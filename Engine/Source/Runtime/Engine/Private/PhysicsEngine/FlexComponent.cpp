// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "PhysXSupport.h"
#include "PhysicsPublic.h"
#include "StaticMeshResources.h"

#if WITH_FLEX

#include "PhysicsEngine/FlexFluidSurfaceComponent.h"
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
	bRequiresCustomLocation = true;

	EnableParticleMode = false;
	ParticleMode = EFlexParticleMode::FillVolume;
	bFlexParticlesSpawned = false;

	VolumeRadius = 42.0f;
	VolumeSeparation = 5.0f;
	VolumeMaxParticles = 10000;
	VolumeMaxAttempts = 10000;

	GridDimensions = FIntVector(10, 10, 10);
	GridRadius = 10.f;
	GridJitter = 0.0f;

	DiffuseParticleScale = 0.5f;
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
	if (GEngine && EnableParticleMode)
	{
		FPhysScene* PhysScene = GetWorld()->GetPhysicsScene();
		if (PhysScene)
		{
			FFlexContainerInstance* Container = PhysScene->GetFlexContainer(ContainerTemplate);
			if (Container)
			{
				ContainerInstance = Container;
				ContainerInstance->Register(this);

				if (FluidSurfaceTemplate != nullptr)
				{
					UFlexFluidSurfaceComponent* FluidSurfaceComponent = GetWorld()->AddFlexFluidSurface(FluidSurfaceTemplate);
					if (FluidSurfaceComponent)
					{
						FluidSurfaceComponent->RegisterFlexComponent(this);
					}
				}
			}
		}
	}

	if (GEngine && StaticMesh && StaticMesh->FlexAsset && !EnableParticleMode)
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
	
	if (ContainerInstance && EnableParticleMode && bFlexParticlesSpawned)
	{
		if (ParticleIndices.Num() > 0)
		{
			FVector4* RESTRICT DstParticles = &SimPositions[0];
			FVector* RESTRICT DstNormals = &SimNormals[0];

			const FVector4* RESTRICT SrcParticles = ContainerInstance->Particles;
			const FVector4* RESTRICT SrcNormals = ContainerInstance->Normals;

			for (int i = 0; i < ParticleIndices.Num(); ++i)
			{
				int ParticleIndex = ParticleIndices[i];

				DstParticles[i] = SrcParticles[ParticleIndex];
				DstNormals[i] = SrcNormals[ParticleIndex];
			}

			// update render transform
			MarkRenderTransformDirty();

			// update render thread data
			MarkRenderDynamicDataDirty();
		}
	}

	else if (ContainerInstance && AssetInstance)
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

	if (SceneProxy && !EnableParticleMode)
	{
		UpdateSceneProxy((FFlexMeshSceneProxy*)SceneProxy);
	}
	else
	{
		FFlexParticleSceneProxy* ParticleSceneProxy = static_cast<FFlexParticleSceneProxy*>(SceneProxy);
		TArray<FVector4> Positions;
		TArray<FVector4> DiffusePositions;
		TArray<FVector4> Anisotropy1;
		TArray<FVector4> Anisotropy2;
		TArray<FVector4> Anisotropy3;

		Positions.AddZeroed(ParticleIndices.Num());
		DiffusePositions.AddZeroed(ContainerInstance->NumDiffuseParticles);

		for (int32 i = 0; i < ParticleIndices.Num(); i++)
		{
			Positions[i] = (ContainerTemplate->PositionSmoothing > 0.0f) ? ContainerInstance->SmoothPositions[ParticleIndices[i]] :ContainerInstance->Particles[ParticleIndices[i]];
		}

		if (ContainerInstance->NumDiffuseParticles > 0)
		{
			for (int32 i = 0; i < ContainerInstance->NumDiffuseParticles; i++)
			{
				DiffusePositions[i] = ContainerInstance->DiffuseParticles[i];
			}
		}

		if (ContainerTemplate->AnisotropyScale > 0.0f)
		{
			Anisotropy1.AddZeroed(ParticleIndices.Num());
			Anisotropy2.AddZeroed(ParticleIndices.Num());
			Anisotropy3.AddZeroed(ParticleIndices.Num());

			for (int32 i = 0; i < ParticleIndices.Num(); i++)
			{
				Anisotropy1[i] = ContainerInstance->Anisotropy1[ParticleIndices[i]];
				Anisotropy2[i] = ContainerInstance->Anisotropy2[ParticleIndices[i]];
				Anisotropy3[i] = ContainerInstance->Anisotropy3[ParticleIndices[i]];
			}
		}

		ENQUEUE_UNIQUE_RENDER_COMMAND_SIXPARAMETER(
			FFlexUpdateParticlePositions,
			FFlexParticleSceneProxy*, SceneProxy, ParticleSceneProxy,
			TArray<FVector4>, ParticlePositions, Positions,
			TArray<FVector4>, DiffuseParticlePositions, DiffusePositions,
			TArray<FVector4>, ParticleAnisotropy1, Anisotropy1,
			TArray<FVector4>, ParticleAnisotropy2, Anisotropy2,
			TArray<FVector4>, ParticleAnisotropy3, Anisotropy3,
			{
				SceneProxy->UpdateParticlePositions(ParticlePositions);
				SceneProxy->UpdateDiffuseParticlePositions(DiffuseParticlePositions);

				if (ParticleAnisotropy1.Num() > 0)
				{
					SceneProxy->UpdateAnisotropy(ParticleAnisotropy1, ParticleAnisotropy2, ParticleAnisotropy3);
				}
			});
	}
}

FBoxSphereBounds UFlexComponent::CalcBounds(const FTransform & LocalToWorld) const
{

#if WITH_FLEX

	if (EnableParticleMode)
	{
#if WITH_EDITOR
		if (GetWorld()->WorldType == EWorldType::Editor)
		{
			FBoxSphereBounds NewBounds = FBoxSphereBounds(FSphere(LocalToWorld.GetLocation(), 65536.f));
			return NewBounds;
		}
		else
#endif
		{
			if (ContainerInstance && bFlexParticlesSpawned)
			{
				//FBoxSphereBounds NewBounds = FBoxSphereBounds(FSphere(LocalToWorld.GetLocation(), 65536.f));
				//return NewBounds;
				TArray<FVector> Positions;
				Positions.SetNum(ParticleIndices.Num());
				
				for (int32 i = 0; i < Positions.Num(); i++)
					Positions[i] = FVector(
						ContainerInstance->Particles[ParticleIndices[i]].X,
						ContainerInstance->Particles[ParticleIndices[i]].Y,
						ContainerInstance->Particles[ParticleIndices[i]].Z
						);

				return FBoxSphereBounds(Positions.GetData(), Positions.Num());
			}
			else if (ContainerInstance && ParticleMode == EFlexParticleMode::Shape)
			{
				return Super::CalcBounds(LocalToWorld);
			}
			else
			{
				return FBoxSphereBounds();
			}
		}
	}
	else
	{
		if (StaticMesh && ContainerInstance && Bounds.SphereRadius > 0.0f && (StaticMesh->FlexAsset->GetClass() == UFlexAssetCloth::StaticClass() ||
			StaticMesh->FlexAsset->GetClass() == UFlexAssetSoft::StaticClass()))
		{
			return LocalBounds.TransformBy(LocalToWorld);
		}
		else
		{
			return Super::CalcBounds(LocalToWorld);
		}
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

	if (EnableParticleMode)
	{
		if (ContainerInstance && !bFlexParticlesSpawned)
		{
			/* Enable dependent components first */
			TArray<UFlexComponent*> Components;
			GetOwner()->GetComponents<UFlexComponent>(Components);

			for (auto It = DependentComponents.CreateIterator(); It; ++It)
			{
				for (auto CompIt = Components.CreateIterator(); CompIt; ++CompIt)
				{
					if ((*CompIt)->GetFName() == (*It))
					{
						(*CompIt)->EnableSim();
						break;
					}
				}
			}

			ComponentPhase = ContainerInstance->GetPhase(Phase);
			float InvMass = (OverrideAsset && Mass > 0.0f) ? (1.f / Mass) : 0.0f;

			if (PreSimPositions.Num() > 0)
			{
				for (int32 i = 0; i < PreSimPositions.Num(); i++)
				{
					ParticleIndices.Add(ContainerInstance->CreateParticle(FVector4(PreSimPositions[i], InvMass), FVector::ZeroVector, ComponentPhase));
				}

				bFlexParticlesSpawned = true;
				MarkRenderStateDirty();
			}
			else
			{
				if (ParticleMode == EFlexParticleMode::Shape)
				{
					if (StaticMesh && StaticMesh->FlexAsset)
					{
						if (!OverrideAsset)
						{
							InvMass = (StaticMesh->FlexAsset->Mass > 0.0f) ? (1.f / StaticMesh->FlexAsset->Mass) : 0.0f;
						}

						int32 ParticlesPerShape = StaticMesh->FlexAsset->Particles.Num();
						for (int32 i = 0; i < ParticlesPerShape; i++)
						{
							ParticleIndices.Add(ContainerInstance->CreateParticle(ComponentToWorld.TransformPosition(
								FVector4(
									StaticMesh->FlexAsset->Particles[i].X,
									StaticMesh->FlexAsset->Particles[i].Y, 
									StaticMesh->FlexAsset->Particles[i].Z,
									InvMass
									)), FVector::ZeroVector, ComponentPhase));
						}
					}
				}
				else if (ParticleMode == EFlexParticleMode::ParticleGrid)
				{
					for (int32 x = -(GridDimensions.X / 2); x < GridDimensions.X / 2; x++)
					{
						for (int32 y = -(GridDimensions.Y / 2); y < GridDimensions.Y / 2; y++)
						{
							for (int32 z = -(GridDimensions.Z / 2); z < GridDimensions.Z / 2; z++)
							{
								FVector Pos = FVector((float)x, (float)y, (float)z) * GridRadius + FMath::VRand() * GridJitter;
								ParticleIndices.Add(ContainerInstance->CreateParticle(FVector4(ComponentToWorld.TransformPosition(Pos), InvMass), FVector::ZeroVector, ComponentPhase));
							}
						}
					}
				}
				else if (ParticleMode == EFlexParticleMode::FillVolume)
				{
					TArray<FVector> Points;
					Points.AddZeroed(VolumeMaxParticles);

					int32 c = 0;
					while (c < VolumeMaxParticles)
					{
						int32 a = 0;
						while (a < VolumeMaxAttempts)
						{
							FVector p = FVector::ZeroVector;
							for (;;)
							{
								FVector v = FVector(
									FMath::FRand() * 2.f - 1.f,
									FMath::FRand() * 2.f - 1.f,
									FMath::FRand() * 2.f - 1.f
									);

								if (FVector::DotProduct(v, v) < 1.f)
								{
									p = v;
									break;
								}
							}
							p *= VolumeRadius;

							int32 i = 0;
							for (; i < c; i++)
							{
								FVector d = p - Points[i];

								if (d.SizeSquared() < VolumeSeparation * VolumeSeparation)
									break;
							}

							if (i == c)
							{
								Points[i] = p;
								ParticleIndices.Add(ContainerInstance->CreateParticle(FVector4(ComponentToWorld.TransformPosition(p), InvMass), FVector::ZeroVector, ComponentPhase));
								++c;
								break;
							}

							++a;
						}

						if (a == VolumeMaxAttempts)
							break;
					}
				}

				bFlexParticlesSpawned = true;
				MarkRenderStateDirty();
			}

			SimPositions.SetNum(ParticleIndices.Num());
			SimNormals.SetNum(ParticleIndices.Num());
		}
	}
	else
	{
		if (ContainerInstance && !AssetInstance)
		{
			/* Enable dependent components first */
			TArray<UFlexComponent*> Components;
			GetOwner()->GetComponents<UFlexComponent>(Components);

			for (auto It = DependentComponents.CreateIterator(); It; ++It)
			{
				for (auto CompIt = Components.CreateIterator(); CompIt; ++CompIt)
				{
					if ((*CompIt)->GetFName() == (*It))
					{
						(*CompIt)->EnableSim();
						break;
					}
				}
			}

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

					for (int ParticleIndex = 0; ParticleIndex < AssetInstance->mNumParticles; ++ParticleIndex)
					{
						FVector4 ParticlePos = SimPositions[ParticleIndex];

						// perform a point check (small sphere)
						FCollisionShape Shape;
						Shape.SetSphere(0.001f);

						// gather overlapping primitives, except owning actor
						TArray<FOverlapResult> Overlaps;
						FCollisionQueryParams QueryParams(false);
						//QueryParams.AddIgnoredActor(GetOwner());
						GetWorld()->OverlapMultiByObjectType(Overlaps, ParticlePos, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects), Shape, QueryParams);

						// pick first non-flex actor, that has a body and is not a trigger
						UPrimitiveComponent* PrimComp = NULL;
						for (int32 OverlapIdx = 0; OverlapIdx < Overlaps.Num() && !PrimComp; ++OverlapIdx)
						{
							FOverlapResult const& O = Overlaps[OverlapIdx];

							if (!O.Component.IsValid() || O.Component.Get() == this)
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
	if (EnableParticleMode)
	{
#if WITH_EDITOR
		if(GetWorld()->WorldType == EWorldType::Editor)
		{
			/* In editor */
			if (PreSimPositions.Num() > 0)
			{
				/* Show saved simulated positions */
				return new FFlexParticleSceneProxy(this, PreSimPositions);
			}
			else
			{
				if (ParticleMode == EFlexParticleMode::Shape)
				{
					if(StaticMesh)
					{
						TArray<FVector> Positions;
						Positions.SetNum(StaticMesh->FlexAsset->Particles.Num());

						for (int32 i = 0; i < StaticMesh->FlexAsset->Particles.Num(); i++)
						{
							Positions[i] = ComponentToWorld.TransformPosition(FVector(
								StaticMesh->FlexAsset->Particles[i].X,
								StaticMesh->FlexAsset->Particles[i].Y,
								StaticMesh->FlexAsset->Particles[i].Z
								));
						}
						return new FFlexParticleSceneProxy(this, Positions);
					}
				}
				else if (ParticleMode == EFlexParticleMode::ParticleGrid)
				{
					TArray<FVector> Positions;
					for (int32 x = -(GridDimensions.X / 2); x < GridDimensions.X / 2; x++)
					{
						for (int32 y = -(GridDimensions.Y / 2); y < GridDimensions.Y / 2; y++)
						{
							for (int32 z = -(GridDimensions.Z / 2); z < GridDimensions.Z / 2; z++)
							{
								FVector Pos = FVector((float)x, (float)y, (float)z) * GridRadius + FMath::VRand() * GridJitter;
								Positions.Add(ComponentToWorld.TransformPosition(Pos));
							}
						}
					}
					return new FFlexParticleSceneProxy(this, Positions);
				}
				else if (ParticleMode == EFlexParticleMode::FillVolume)
				{
					TArray<FVector> Positions;

					TArray<FVector> Points;
					Points.AddZeroed(VolumeMaxParticles);

					int32 c = 0;
					while (c < VolumeMaxParticles)
					{
						int32 a = 0;
						while (a < VolumeMaxAttempts)
						{
							FVector p = FVector::ZeroVector;
							for (;;)
							{
								FVector v = FMath::VRand();
								if (FVector::DotProduct(v, v) < 1.f)
								{
									p = v;
									break;
								}
							}
							p *= VolumeRadius;

							int32 i = 0;
							for (; i < c; i++)
							{
								FVector d = p - Points[i];

								if (d.SizeSquared() < VolumeSeparation * VolumeSeparation)
									break;
							}

							if (i == c)
							{
								Points[i] = p;
								Positions.Add(ComponentToWorld.TransformPosition(p));
								++c;
								break;
							}

							++a;
						}

						if (a == VolumeMaxAttempts)
							break;
					}
					return new FFlexParticleSceneProxy(this, Positions);
				}

				TArray<FVector> Positions;
				Positions.Add(ComponentToWorld.TransformPosition(FVector(0, 0, 0)));

				return new FFlexParticleSceneProxy(this, Positions);
			}
		}
		else
#endif
		{
			return new FFlexParticleSceneProxy(this);
		}
	}

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
	if (EnableParticleMode)
	{
#if WITH_EDITOR
		if (GetWorld()->WorldType == EWorldType::Editor)
		{
			return true;
		}
#endif
	}
	else
	{
		// if this component has a flex asset then don't recreate the proxy
		if (AssetInstance && ContainerInstance && (StaticMesh->FlexAsset->GetClass() == UFlexAssetCloth::StaticClass() ||
			StaticMesh->FlexAsset->GetClass() == UFlexAssetSoft::StaticClass()))
		{
			return false;
		}
	}
#endif
	
	return Super::ShouldRecreateProxyOnUpdateTransform();
}

void UFlexComponent::AddRadialForce(FVector Origin, float Radius, float Strength, enum ERadialImpulseFalloff Falloff, bool bAccelChange)
{
#if WITH_FLEX
	if (ContainerInstance)
	{
		ContainerInstance->AddRadialForce(Origin, Radius, Strength, Falloff);
	}
#endif
}

void UFlexComponent::AddForce(FVector Force, FName BoneName, bool bAccelChange)
{
#if WITH_FLEX
	if (ContainerInstance && EnableParticleMode && bFlexParticlesSpawned)
	{
		for (int32 i = 0; i < ParticleIndices.Num(); i++)
		{
			if (bAccelChange)
			{
				ContainerInstance->Velocities[ParticleIndices[i]] = Force;
			}
			else
			{
				ContainerInstance->Velocities[ParticleIndices[i]] += Force;
			}
		}
	}
	else if (ContainerInstance && AssetInstance)
	{
		for (int32 i = 0; i < AssetInstance->mNumParticles; i++)
		{
			if (bAccelChange)
			{
				ContainerInstance->Velocities[AssetInstance->mParticleIndices[i]] = Force;
			}
			else
			{
				ContainerInstance->Velocities[AssetInstance->mParticleIndices[i]] += Force;
			}
		}
	}
#endif
}

void UFlexComponent::AddRadialImpulse(FVector Origin, float Radius, float Strength, enum ERadialImpulseFalloff Falloff, bool bVelChange)
{
#if WITH_FLEX
	if (ContainerInstance)
	{
		ContainerInstance->AddRadialImpulse(Origin, Radius, Strength, Falloff, bVelChange);
	}
#endif
}

FVector UFlexComponent::GetCustomLocation() const
{
	/*if (ContainerInstance && AssetInstance)
	{
		FVector TotalLocation = FVector::ZeroVector;
		for (int32 i = 0; i < SimPositions.Num(); i++)
		{
			TotalLocation += SimPositions[i];
		}
		return (TotalLocation / SimPositions.Num());
	}*/
	return ComponentToWorld.GetTranslation();
}

void UFlexComponent::CreateParticles(TArray<FVector> InPositions, TArray<float> InMasses, TArray<FVector> InVelocities, TArray<int32>& OutIndices)
{
#if WITH_FLEX
	if (ContainerInstance && EnableParticleMode)
	{
		if (!bFlexParticlesSpawned)
		{
			ComponentPhase = ContainerInstance->GetPhase(Phase);
			bFlexParticlesSpawned = true;
		}

		ContainerInstance->CreateParticles(InPositions, InMasses, InVelocities, ComponentPhase, OutIndices);
		ParticleIndices.Append(OutIndices);

		SimPositions.SetNum(ParticleIndices.Num());
		SimNormals.SetNum(ParticleIndices.Num());

		MarkRenderTransformDirty();
		MarkRenderDynamicDataDirty();
	}
#endif
}

#if WITH_EDITOR
void UFlexComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (EnableParticleMode && !OverrideAsset)
		OverrideAsset = true;

	if (EnableParticleMode && !Phase.Fluid && FluidSurfaceTemplate != nullptr)
		FluidSurfaceTemplate = nullptr;

	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if (PropertyThatChanged)
	{
		if (PropertyThatChanged->GetName().Contains(TEXT("EnableParticleMode")) 
			|| PropertyThatChanged->GetName().Contains(TEXT("ContainerTemplate"))
			|| PropertyThatChanged->GetName().Contains(TEXT("ParticleMode"))
			|| PropertyThatChanged->GetName().Contains(TEXT("VolumeRadius"))
			|| PropertyThatChanged->GetName().Contains(TEXT("VolumeSeparation"))
			|| PropertyThatChanged->GetName().Contains(TEXT("VolumeMaxParticles"))
			|| PropertyThatChanged->GetName().Contains(TEXT("VolumeMaxAttempts"))
			|| PropertyThatChanged->GetName().Contains(TEXT("GridDimensions"))
			|| PropertyThatChanged->GetName().Contains(TEXT("GridRadius"))
			|| PropertyThatChanged->GetName().Contains(TEXT("GridJitter")))
		{
			PreSimPositions.Empty();
		}
	}
}
#endif // WITH_EDITOR

#endif // WITH_FLEX
