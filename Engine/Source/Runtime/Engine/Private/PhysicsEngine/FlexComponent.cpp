// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "PhysXSupport.h"
#include "PhysicsPublic.h"
#include "StaticMeshResources.h"

#if WITH_FLEX

#include "FlexContainerInstance.h"
#include "FlexRender.h"

#if STATS

DECLARE_CYCLE_STAT(TEXT("Update Bounds (CPU)"), STAT_Flex_UpdateBoundsCpu, STATGROUP_Flex);

DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Active Mesh Particle Count"), STAT_Flex_ActiveParticleCount, STATGROUP_Flex);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Active Mesh Actor Count"), STAT_Flex_ActiveMeshActorCount, STATGROUP_Flex);

#endif


UFlexComponent::UFlexComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	// tick driven through container
	PrimaryComponentTick.bCanEverTick = false;

	OverrideAsset = false;
	AttachToRigids = false;
	ContainerTemplate = NULL;
	Mass = 1.0f;
	Mobility = EComponentMobility::Movable;
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetSimulatePhysics(false);
	SetViewOwnerDepthPriorityGroup(true, SDPG_World);

	AssetInstance = NULL;
	ContainerInstance = NULL;
}

void UFlexComponent::OnRegister()
{
	Super::OnRegister();

#if WITH_EDITOR
	if (GIsEditor && !GIsPlayInEditorWorld)
	{
		//this is executed on actor conversion and restores the collision and simulation settings
		SetSimulatePhysics(false);
		Mobility = EComponentMobility::Movable;
	}
#endif

#if WITH_FLEX
	if (GEngine && StaticMesh && StaticMesh->FlexAsset)
	{
		// use the actor's settings instead of the defaults from the asset
		if (!OverrideAsset)
		{
			ContainerTemplate = StaticMesh->FlexAsset->ContainerTemplate;
			Phase = StaticMesh->FlexAsset->Phase;
			Mass = StaticMesh->FlexAsset->Mass;
			AttachToRigids = StaticMesh->FlexAsset->AttachToRigids;
		}

		// request attach with the FlexContainer
		if (ContainerTemplate && (!GIsEditor  || GIsPlayInEditorWorld) && !AssetInstance)
		{
			FPhysScene* PhysScene = GetWorld()->GetPhysicsScene();

			if (PhysScene)
			{
				FFlexContainerInstance* Container = PhysScene->GetFlexContainer(ContainerTemplate);
				if (Container)
				{
					ContainerInstance = Container;
					ContainerInstance->Register(this);

					const int NumParticles = StaticMesh->FlexAsset->Particles.Num();

					SimPositions.SetNum(NumParticles);
					SimNormals.SetNum(NumParticles);

					float InvMassScale = 1.0f;
					if (OverrideAsset)
					{
						InvMassScale = (Mass > 0.0f) ? (StaticMesh->FlexAsset->Mass / Mass) : 0.0f;
					}

					if (NumParticles == PreSimPositions.Num())
					{
						// if pre-sim state still matches the static mesh apply any pre-simulated positions to the particles
						for (int i=0; i < NumParticles; ++i)
						{
							float mass = StaticMesh->FlexAsset->Particles[i].W*InvMassScale;

							SimPositions[i] = FVector4(PreSimPositions[i], mass);
						}
					}
					else
					{
						// particles are static mesh positions transformed by actor position
						for (int i=0; i < NumParticles; ++i)
						{
							FVector LocalPos = StaticMesh->FlexAsset->Particles[i];
							float mass = StaticMesh->FlexAsset->Particles[i].W*InvMassScale;

							SimPositions[i] = FVector4(FVector(ComponentToWorld.TransformPosition(LocalPos)), mass);
						}
					}

					// calculate normals for initial particle positions, this is necessary because otherwise 
					// the mesh will be rendered incorrectly if it is visible before it is first simulated
					// todo: serialize these initial normals
					if (StaticMesh->FlexAsset->GetClass() == UFlexAssetCloth::StaticClass())
					{
						const TArray<int>& TriIndices = StaticMesh->FlexAsset->Triangles;
						int NumTriangles = TriIndices.Num()/3;

						const FVector4* RESTRICT Particles = &SimPositions[0];
						FVector* RESTRICT Normals = &SimNormals[0];
				
						// iterate over triangles updating vertex normals
						for (int i=0; i < NumTriangles; ++i)
						{
							const int a = TriIndices[i*3+0];
							const int b = TriIndices[i*3+1];
							const int c = TriIndices[i*3+2];

							FVector Vertex0 = Particles[a];
							FVector Vertex1 = Particles[b];
							FVector Vertex2 = Particles[c];

							FVector TriNormal = (Vertex1-Vertex0)^(Vertex2-Vertex0);

							Normals[a] += TriNormal;
							Normals[b] += TriNormal;
							Normals[c] += TriNormal;
						}

						// normalize normals
						for (int i=0; i < NumParticles; ++i)
							Normals[i] = Normals[i].GetSafeNormal();					
					}
				}

				// ensure valid initial bounds for LOD
				UpdateBounds();
			}
		}
	}

#endif // WITH_FLEX
}

void UFlexComponent::OnUnregister()
{
	Super::OnUnregister();

#if WITH_FLEX

	if (ContainerInstance && AssetInstance)
	{
		DEC_DWORD_STAT_BY(STAT_Flex_ActiveParticleCount, AssetInstance->mNumParticles);
		DEC_DWORD_STAT(STAT_Flex_ActiveMeshActorCount);

		ContainerInstance->DestroyInstance(AssetInstance);
		AssetInstance = NULL;

	}

	if (ContainerInstance)
	{
		ContainerInstance->Unregister(this);
		ContainerInstance = NULL;
	}

#endif // WITH_FLEX

}

// called during the sychronous portion of the FlexContainer update
// i.e.: at this point there is no GPU work outstanding, so we may 
// modify particles freely, create instances, etc
void UFlexComponent::Synchronize()
{
#if WITH_FLEX
	if (!IsRegistered())
		return;
	
	if (ContainerInstance && AssetInstance)
	{
		// process attachments
		for (int AttachmentIndex=0; AttachmentIndex < Attachments.Num(); )
		{
			const FlexParticleAttachment& Attachment = Attachments[AttachmentIndex];
			const UPrimitiveComponent* PrimComp = Attachment.Primitive.Get();
		
			// index into the simulation data, we need to modify the container's copy
			// of the data so that the new positions get sent back to the sim
			const int ParticleIndex = AssetInstance->mParticleIndices[Attachment.ParticleIndex];

			if(PrimComp)
			{
				// calculate world position of attached particle, and zero mass
				const FTransform PrimTransform = PrimComp->GetComponentToWorld();
				const FVector AttachedPos = PrimTransform.TransformPosition(Attachment.LocalPos);

				ContainerInstance->Particles[ParticleIndex] = FVector4(AttachedPos, 0.0f);
				ContainerInstance->Velocities[ParticleIndex] = FVector(0.0f);

				++AttachmentIndex;
			}
			else // process detachments
			{
				ContainerInstance->Particles[ParticleIndex].W = Attachment.OldMass;
				ContainerInstance->Velocities[ParticleIndex] = FVector(0.0f);
				
				Attachments.RemoveAt(AttachmentIndex);
			}
		}

		// if sim is enabled, then read back latest position and normal data for rendering
		int NumParticles = AssetInstance->mNumParticles;
		const int* RESTRICT Indices = AssetInstance->mParticleIndices;

		FVector4* RESTRICT DstParticles = &SimPositions[0];
		FVector* RESTRICT DstNormals = &SimNormals[0];

		const FVector4* RESTRICT SrcParticles = ContainerInstance->Particles;
		const FVector4* RESTRICT SrcNormals = ContainerInstance->Normals;

		for (int i=0; i < NumParticles; ++i)
		{
			int ParticleIndex = Indices[i];

			DstParticles[i] = SrcParticles[ParticleIndex];
			DstNormals[i] = SrcNormals[ParticleIndex];
		}

		if (StaticMesh->FlexAsset->GetClass() == UFlexAssetSolid::StaticClass())
		{
			const int ShapeIndex = AssetInstance->mShapeIndex;

			if (ShapeIndex != -1)
			{
				const FQuat Rotation = *(FQuat*)AssetInstance->mShapeRotations;
				const FVector Translation = *(FVector*)AssetInstance->mShapeTranslations;

				const FTransform NewTransform(Rotation, Translation);

				// offset to handle case where object's pivot is not aligned with the object center of mass
                const FVector Offset = ComponentToWorld.TransformVector(FVector(AssetInstance->mAsset->mShapeCenters[0], AssetInstance->mAsset->mShapeCenters[1], AssetInstance->mAsset->mShapeCenters[2]));
				const FVector MoveBy = NewTransform.GetLocation() - ComponentToWorld.GetLocation() - Offset;
				const FRotator NewRotation = NewTransform.Rotator();

				EMoveComponentFlags MoveFlags = IsCollisionEnabled() ? MOVECOMP_NoFlags : MOVECOMP_SkipPhysicsMove;
				MoveComponent(MoveBy, NewRotation, false, NULL, MoveFlags);
			}

			UpdateComponentToWorld();
		}
		else if (StaticMesh->FlexAsset->GetClass() == UFlexAssetCloth::StaticClass() || 
				 StaticMesh->FlexAsset->GetClass() == UFlexAssetSoft::StaticClass())
		{
			if (IsCollisionEnabled())
			{
				// move collision shapes according center of Bounds
				const FVector MoveBy = Bounds.Origin - ComponentToWorld.GetLocation();
				MoveComponent(MoveBy, FRotator::ZeroRotator, false, NULL, MOVECOMP_NoFlags);
				UpdateComponentToWorld();
			}
		}
		
		// update render transform
		MarkRenderTransformDirty();

		// update render thread data
		MarkRenderDynamicDataDirty();
	}

	EnableSim();

	// update bounds for clothing
	if (StaticMesh && ContainerInstance && Bounds.SphereRadius > 0.0f && 
		(StaticMesh->FlexAsset->GetClass() == UFlexAssetCloth::StaticClass() || 
		 StaticMesh->FlexAsset->GetClass() == UFlexAssetSoft::StaticClass()))
	{
		SCOPE_CYCLE_COUNTER(STAT_Flex_UpdateBoundsCpu);

		// calculate bounding box in world space
		const int NumParticles = SimPositions.Num();
		const FVector4* RESTRICT Particles = &SimPositions[0];

		FBox WorldBounds(0);

		for (int i = 0; i < NumParticles; ++i)
			WorldBounds += FVector(Particles[i]);

		LocalBounds = FBoxSphereBounds(WorldBounds).TransformBy(ComponentToWorld.Inverse());

		// Clamp bounds in case of instability
		const float MaxRadius = 1000000.0f;

		if (LocalBounds.SphereRadius > MaxRadius)
			LocalBounds = FBoxSphereBounds(EForceInit::ForceInitToZero);
	}

	/*
	// check LOD conditions and enable/disable simulation	
	if (SceneProxy)
	{
		FVector PlayerLocation;
		if (GetPlayerPosition(PlayerLocation))
		{
			const float DistanceBeforeSleep = GSystemSettings.FlexDistanceBeforeSleep;
			if (FVector(PlayerLocation - Bounds.Origin).Size() - Bounds.SphereRadius > DistanceBeforeSleep && DistanceBeforeSleep > 0.0f)
				DisableSim();
			else		
			{
				const int LastRenderedFrame = SceneProxy->GetLastVisibleFrame();
				const int NumFramesBeforeSleep = GSystemSettings.FlexInvisibleFramesBeforeSleep;

				// if not rendered in the last N frames then remove this actor's
				// particles and constraints from the simulation
				if(int(GFrameNumber - LastRenderedFrame) > NumFramesBeforeSleep && NumFramesBeforeSleep > 0)
					DisableSim();
				else
					EnableSim();
			}
		}
	}
	*/
#endif // WITH_FLEX
}

void UFlexComponent::UpdateSceneProxy(FFlexMeshSceneProxy* Proxy)
{	
	if (Proxy && StaticMesh->FlexAsset->GetClass() == UFlexAssetSoft::StaticClass())
	{
		// copy transforms to render thread
		const int NumShapes = StaticMesh->FlexAsset->ShapeCenters.Num();
		
		FFlexShapeTransform* NewTransforms = new FFlexShapeTransform[NumShapes];

		if (AssetInstance)
		{
			// set transforms based on the simulation object	
			for (int i=0; i < NumShapes; ++i)
			{
				NewTransforms[i].Translation = *(FVector*)&AssetInstance->mShapeTranslations[i*3];
				NewTransforms[i].Rotation = *(FQuat*)&AssetInstance->mShapeRotations[i*4];
			}
		}
		else
		{
			// if the simulation object isn't valid yet then set transforms 
			// based on the component transform and asset rest poses
			for (int i=0; i < NumShapes; ++i)
			{
				NewTransforms[i].Translation = ComponentToWorld.TransformPosition(StaticMesh->FlexAsset->ShapeCenters[i]);
				NewTransforms[i].Rotation = ComponentToWorld.GetRotation();
			}

			// todo: apply presimulation transforms here
		}

		// the proxy is only a FFlexMeshSceneProxy in the game, in the editor it is the static mesh proxy
		// the container instance is only valid in game so we should never fail this assertion
		check(!GIsEditor || GIsPlayInEditorWorld);

		// Enqueue command to send to render thread
		ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
			FSendFlexShapeTransforms,
			FFlexMeshSceneProxy*, Proxy, (FFlexMeshSceneProxy*)Proxy,
			FFlexShapeTransform*, ShapeTransforms, NewTransforms,			
			int32, NumShapes, NumShapes,
			{
				Proxy->UpdateSoftTransforms(ShapeTransforms, NumShapes);
			});
	}

	// cloth
	if (Proxy && StaticMesh->FlexAsset->GetClass() == UFlexAssetCloth::StaticClass())
	{
		// Enqueue command to send to render thread
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FSendFlexClothTransforms,
			FFlexMeshSceneProxy*, Proxy, (FFlexMeshSceneProxy*)Proxy,
			{
				Proxy->UpdateClothTransforms();
			});
	}
}

void UFlexComponent::SendRenderDynamicData_Concurrent()
{
	Super::SendRenderDynamicData_Concurrent();

	if (SceneProxy)
		UpdateSceneProxy((FFlexMeshSceneProxy*)SceneProxy);
}

FBoxSphereBounds UFlexComponent::CalcBounds(const FTransform & LocalToWorld) const
{

#if WITH_FLEX

	if (StaticMesh && ContainerInstance && Bounds.SphereRadius > 0.0f && (StaticMesh->FlexAsset->GetClass() == UFlexAssetCloth::StaticClass() || 
																		  StaticMesh->FlexAsset->GetClass() == UFlexAssetSoft::StaticClass()))
	{
		return LocalBounds.TransformBy(LocalToWorld);
	}
	else
	{
		return Super::CalcBounds(LocalToWorld);
	}
#else

	return Super::CalcBounds(LocalToWorld);

#endif

}

void UFlexComponent::DisableSim()
{
#if WITH_FLEX
	if (ContainerInstance && AssetInstance)
	{
		DEC_DWORD_STAT_BY(STAT_Flex_ActiveParticleCount, AssetInstance->mNumParticles);
		DEC_DWORD_STAT(STAT_Flex_ActiveMeshActorCount);

		ContainerInstance->DestroyInstance(AssetInstance);
		AssetInstance = NULL;
	}
#endif // WITH_FLEX
}

void UFlexComponent::EnableSim()
{
#if WITH_FLEX
	if (ContainerInstance && !AssetInstance)
	{
		// SimPositions count can be zero if asset internal FlexExtObject creation failed.
		if (SimPositions.Num() == 0)
			return;

		AssetInstance = ContainerInstance->CreateInstance(StaticMesh->FlexAsset->GetFlexAsset(), ComponentToWorld.ToMatrixNoScale(), FVector(0.0f), ContainerInstance->GetPhase(Phase));

		if (AssetInstance)
		{
			INC_DWORD_STAT_BY(STAT_Flex_ActiveParticleCount, StaticMesh->FlexAsset->Particles.Num());
			INC_DWORD_STAT(STAT_Flex_ActiveMeshActorCount);
		
			// if attach requested then generate attachment points for overlapping shapes
			if (AttachToRigids)
			{
				// clear out any previous attachments
				Attachments.SetNum(0);

				for (int ParticleIndex=0; ParticleIndex < AssetInstance->mNumParticles; ++ParticleIndex)
				{
					FVector4 ParticlePos = SimPositions[ParticleIndex];

					// perform a point check (small sphere)
					FCollisionShape Shape;
					Shape.SetSphere(0.001f);

					// gather overlapping primitives, except owning actor
					TArray<FOverlapResult> Overlaps;
					FCollisionQueryParams QueryParams(false);
					QueryParams.AddIgnoredActor(GetOwner());
					GetWorld()->OverlapMultiByObjectType(Overlaps, ParticlePos, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects), Shape, QueryParams);

					// pick first non-flex actor, that has a body and is not a trigger
					UPrimitiveComponent* PrimComp = NULL;
					for (int32 OverlapIdx = 0; OverlapIdx<Overlaps.Num() && !PrimComp; ++OverlapIdx)
					{
						FOverlapResult const& O = Overlaps[OverlapIdx];
						
						if (!O.Component.IsValid() || O.Component.Get()->IsA(UFlexComponent::StaticClass()))
							continue;

						UPrimitiveComponent* TmpPrimComp = O.Component.Get();

						if (TmpPrimComp->GetBodyInstance() == NULL)
							continue;

						ECollisionResponse Response = TmpPrimComp->GetCollisionResponseToChannel(ContainerInstance->Template->ObjectType);
						if (Response == ECollisionResponse::ECR_Ignore)
							continue; 
						
						PrimComp = TmpPrimComp;
					}

					if (PrimComp)
					{
						// calculate local space position of particle in component
						FTransform LocalToWorld = PrimComp->GetComponentToWorld();
						FVector LocalPos = LocalToWorld.InverseTransformPosition(ParticlePos);

						FlexParticleAttachment Attachment;
						Attachment.Primitive = PrimComp;
						Attachment.ParticleIndex = ParticleIndex;
						Attachment.OldMass = ParticlePos.W;
						Attachment.LocalPos = LocalPos;
						Attachment.ShapeIndex = 0;	// don't currently support shape indices

						Attachments.Add(Attachment);
					}
				}
			}

			// apply any existing positions (pre-simulated particles)
			for (int32 i = 0; i < AssetInstance->mNumParticles; ++i)
				ContainerInstance->Particles[AssetInstance->mParticleIndices[i]] = SimPositions[i];
		}
	}
#endif // WITH_FLEX
}

FMatrix UFlexComponent::GetRenderMatrix() const
{
#if WITH_FLEX
	if (ContainerInstance && StaticMesh && (StaticMesh->FlexAsset->GetClass() == UFlexAssetCloth::StaticClass() || 
										    StaticMesh->FlexAsset->GetClass() == UFlexAssetSoft::StaticClass()))
	{
		// particles are simulated in world space
		return FMatrix::Identity;
	}
#endif

	return Super::GetRenderMatrix();
}


FPrimitiveSceneProxy* UFlexComponent::CreateSceneProxy()
{
#if WITH_FLEX
	// if this component has a flex asset then use the subtitute scene proxy for rendering (cloth and soft bodies only)
	if (ContainerInstance && StaticMesh && (StaticMesh->FlexAsset->GetClass() == UFlexAssetCloth::StaticClass() ||
											StaticMesh->FlexAsset->GetClass() == UFlexAssetSoft::StaticClass()))
	{
		FFlexMeshSceneProxy* Proxy = new FFlexMeshSceneProxy(this);
		
		// send initial render data to the proxy
		UpdateSceneProxy(Proxy);
		return Proxy;
	}
#endif // WITH_FLEX

	//@todo: figure out why i need a ::new (gcc3-specific)
	return Super::CreateSceneProxy();

}
bool UFlexComponent::ShouldRecreateProxyOnUpdateTransform() const
{
#if WITH_FLEX
	// if this component has a flex asset then don't recreate the proxy
	if (AssetInstance && ContainerInstance && (StaticMesh->FlexAsset->GetClass() == UFlexAssetCloth::StaticClass() ||
											   StaticMesh->FlexAsset->GetClass() == UFlexAssetSoft::StaticClass()))
	{
		return false;
	}
#endif
	
	return Super::ShouldRecreateProxyOnUpdateTransform();
}

#endif // WITH_FLEX
