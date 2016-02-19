// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/*=============================================================================
	FieldSamplerComponent.cpp: UFieldSamplerComponent methods.
=============================================================================*/


#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"
#include "../Collision/PhysXCollision.h"

UFieldSamplerComponent::UFieldSamplerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BodyInstance.SetUseAsyncScene(true);

	static FName CollisionProfileName(TEXT("Turbulence"));
	SetCollisionProfileName(CollisionProfileName);

	bAlwaysCreatePhysicsState = true;
	bIsActive = true;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	// Field Sampler Parameters
	bEnabled = true;

#if WITH_APEX_TURBULENCE
	ApexFieldSamplerActor = NULL;
#endif
	FieldSamplerInstance = NULL;
	FXSystem = NULL;
}

void UFieldSamplerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if WITH_APEX_TURBULENCE
	bool bEnabledByDistance = true;
	if (GEngine->ApexTurbulenceDisabledDistance > 0.0f)
	{
		UWorld* World = GetWorld();
		if(World != NULL)
		{
			FVector	TurbulenceLocation	= ComponentToWorld.GetLocation();

			float DistanceToTurbulence = 0.0f;
			float DisabledDistance = World->GetPlayerControllerIterator() ? WORLD_MAX : 0.0f;
			//bool bBehindPlayer = true;
			for( FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator )
			{
				APlayerController* PlayerController = *Iterator;
				if(PlayerController->IsLocalPlayerController())
				{
					FVector POVLoc;
					FRotator POVRotation;
					PlayerController->GetPlayerViewPoint(POVLoc, POVRotation);

					//if ( FVector::DotProduct(TurbulenceLocation - POVLoc, POVRotation.Vector()) > 0 )
					//{
					//	bBehindPlayer = false;
					//}

					DistanceToTurbulence = FVector(POVLoc - TurbulenceLocation).Size();

					// Take closest
					if ((DisabledDistance == 0.f) || (DistanceToTurbulence < DisabledDistance))
					{
						DisabledDistance = DistanceToTurbulence;
					}
				}
			}
			bEnabledByDistance = (DisabledDistance <= GEngine->ApexTurbulenceDisabledDistance) /*&& !bBehindPlayer*/;
		}
	}

	if(bEnabledByDistance)
	{
		if (bEnabled)
		{
			CreateApexObjects();
		}
	}
	else
	{
		DestroyApexObjects();
	}

	if (bEnabled)
	{
		if( Duration > 0.0f )   
		{
			if( ElapsedTime >= Duration )
			{
				ElapsedTime = 0.0f;
				bEnabled = false;

				AActor* MyOwner = GetOwner();
				if(MyOwner)
				{
					AFieldSamplerActor* SpawnableActor = Cast<AFieldSamplerActor>(MyOwner);
					if(SpawnableActor)
					{
						World->DestroyActor(SpawnableActor);
					}
					else
					{
						DetachFromParent();
					}
				}
			}

			ElapsedTime += DeltaTime;
		}
	}
#endif
}

void UFieldSamplerComponent::CreatePhysicsState()
{
	UActorComponent::CreatePhysicsState();

	//CreateApexObjects();
}


void UFieldSamplerComponent::DestroyPhysicsState()
{
	DestroyApexObjects();

	UActorComponent::DestroyPhysicsState();
}

#if WITH_EDITOR	
void UFieldSamplerComponent::OnRegister()
{
	Super::OnRegister();

	if (GetFieldSamplerAsset())
	{
		GetFieldSamplerAsset()->RegisterFSComponent(this);
	}
}

void UFieldSamplerComponent::OnUnregister()
{
	if (GetFieldSamplerAsset())
	{
		GetFieldSamplerAsset()->UnregisterFSComponent(this);
	}

	Super::OnUnregister();
}
#endif

void UFieldSamplerComponent::OnUpdateTransform(bool bSkipPhysicsMove, ETeleportType Teleport)
{
	// We are handling the physics move below, so don't handle it at higher levels
	Super::OnUpdateTransform(true, Teleport);

#if WITH_APEX_TURBULENCE
	if (ApexFieldSamplerActor)
	{
		FMatrix UGlobalPose = ComponentToWorld.ToMatrixWithScale();
		ApexFieldSamplerActor->UpdatePosition(UGlobalPose);
	}
#endif
}

void UFieldSamplerComponent::CreateApexObjects()
{
	if (GEngine->GetPhysXLevel() < 2)
	{
		return;
	}

#if WITH_APEX_TURBULENCE
	if (ApexFieldSamplerActor == NULL)
	{
		FPhysScene* PhysScene = World->GetPhysicsScene();
		check(PhysScene);

		UFieldSamplerAsset* FieldSamplerAsset = GetFieldSamplerAsset();

		if (FieldSamplerAsset && FieldSamplerAsset->ApexFieldSamplerAsset)
		{
			CreateApexActor(PhysScene, FieldSamplerAsset);
			UpdateApexActor();
		}
	}

#endif //WITH_APEX_TURBULENCE
}

void UFieldSamplerComponent::DestroyApexObjects()
{
#if WITH_APEX_TURBULENCE
	RemoveFieldSamplerInstance();
	FXSystem = NULL;
	if(ApexFieldSamplerActor)
	{
		ApexFieldSamplerActor->DeferredRelease();
		ApexFieldSamplerActor = NULL;
	}
#endif //WITH_APEX_TURBULENCE
}

void UFieldSamplerComponent::SetEnabled(bool Enabled)
{
	bEnabled = Enabled;
	UpdateApexActor();
	if (FXSystem)
	{
		FXSystem->UpdateFieldSampler(this);
	}
}

void UFieldSamplerComponent::RemoveFieldSamplerInstance()
{
	if (FieldSamplerInstance)
	{
		check(FXSystem != NULL);
		// Remove the component from the FX system.
		FXSystem->RemoveFieldSampler(this);

		FieldSamplerInstance = NULL;
	}
}

#if WITH_APEX_TURBULENCE
void UFieldSamplerComponent::SetupApexActorParams(NxParameterized::Interface* ActorParams, PxFilterData& PQueryFilterData)
{
	if (ActorParams != NULL)
	{
		FMatrix UGlobalPose = ComponentToWorld.ToMatrixWithScale();
		physx::PxMat44 Pose = U2PMatrix(UGlobalPose);
		verify(NxParameterized::setParamMat34(*ActorParams, "initialPose", Pose));
		verify(NxParameterized::setParamString(*ActorParams, "fieldSamplerFilterDataName", FIELD_SAMPLER_FILTER_DATA_NAME));
		verify(NxParameterized::setParamString(*ActorParams, "fieldBoundaryFilterDataName", FIELD_BOUNDARY_FILTER_DATA_NAME));

		// Setup collision filter data
		PxFilterData PSimFilterData;
		AActor* Owner = GetOwner();
		CreateShapeFilterData(GetCollisionObjectType(), (Owner ? Owner->GetUniqueID() : 0), GetCollisionResponseToChannels(), 0, 0, PQueryFilterData, PSimFilterData, false, false, false);		
	}
}
#endif // WITH_APEX_TURBULENCE
// NVCHANGE_END: JCAO - Add Turbulence actors and components