// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/*=============================================================================
	GridComponent.cpp: UGridComponent methods.
=============================================================================*/


#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"
#include "../FXSystem.h"
#include "TurbulenceFS.h"
#include "ApexFieldSamplerActor.h"


UGridComponent::UGridComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FBoxSphereBounds UGridComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds NewBounds;
	NewBounds.Origin = FVector(0.0f);
	NewBounds.BoxExtent = FVector(0.0f);
	NewBounds.SphereRadius = 0.0f;

#if WITH_APEX_TURBULENCE
	if ( GridAsset )
	{
		NewBounds.BoxExtent = FVector(GridAsset->GridSize3D * (GridAsset->GridScale * 0.5f));
		NewBounds.SphereRadius = NewBounds.BoxExtent.Size();
	}
#endif

	return NewBounds.TransformBy( LocalToWorld );
}

void UGridComponent::SendRenderTransform_Concurrent()
{
	Super::SendRenderTransform_Concurrent();
	if (FXSystem)
	{
		FXSystem->UpdateFieldSampler(this);
	}
}

void UGridComponent::OnDeferredCreated()
{
	if (World && World->Scene)
	{
		// Store the FX system for the world in which this component is registered.
		check(FXSystem == NULL);
		FXSystem = World->Scene->GetFXSystem();
		check(FXSystem != NULL);

		// Add this component to the FX system.
		FXSystem->AddFieldSampler(this);
	}
}

void UGridComponent::SetFieldSamplerAsset( UFieldSamplerAsset* Asset )
{
	UGridAsset* CastedAsset = Cast<UGridAsset>(Asset);
	if (CastedAsset)
	{
		GridAsset = CastedAsset;
	}
}

#if WITH_APEX_TURBULENCE

UFieldSamplerAsset* UGridComponent::GetFieldSamplerAsset()
{ 
	return GridAsset; 
} 

bool UGridComponent::CreateFieldSamplerInstance()
{
	if( GridAsset )
	{
		FTurbulenceFSInstance* Instance = new FTurbulenceFSInstance();
		FieldSamplerInstance = Instance;
		check(ApexFieldSamplerActor);
		Instance->TurbulenceFSActor = static_cast<FApexTurbulenceActor*>(ApexFieldSamplerActor);//static_cast<physx::apex::NxTurbulenceFSActor*>(ApexFieldSamplerActor->GetApexActor());
		Instance->FSType = EFieldSamplerAssetType::EFSAT_GRID;
		Instance->WorldBounds = Bounds.GetBox();
		Instance->LocalBounds = FBox::BuildAABB(FVector::ZeroVector, GridAsset->GridSize3D * (GridAsset->GridScale * 0.5f));
		Instance->VelocityMultiplier = GridAsset->FieldVelocityMultiplier;
		Instance->VelocityWeight = GridAsset->FieldVelocityWeight;
		Instance->bEnabled = bEnabled;
		return true;
	}
	return false;
}

void UGridComponent::UpdateApexActor()
{
	if (ApexFieldSamplerActor)
	{
		ApexFieldSamplerActor->bEnabled = bEnabled;
		ApexFieldSamplerActor->UpdateApexActor();
	}
}

void UGridComponent::SetupApexActorParams(NxParameterized::Interface* ActorParams, PxFilterData& PQueryFilterData)
{
	if (ActorParams != NULL)
	{
		Super::SetupApexActorParams(ActorParams, PQueryFilterData);
		verify(NxParameterized::setParamString(*ActorParams, "collisionFilterDataName", COLLISION_FILTER_DATA_NAME));
	}
}

void UGridComponent::CreateApexActor(FPhysScene* InPhysScene, UFieldSamplerAsset* InFieldSamplerAsset)
{
	ApexFieldSamplerActor = new FApexTurbulenceActor(this, InPhysScene, PST_Async, InFieldSamplerAsset);
}
#endif // WITH_APEX_TURBULENCE


// NVCHANGE_END: JCAO - Add Turbulence actors and components