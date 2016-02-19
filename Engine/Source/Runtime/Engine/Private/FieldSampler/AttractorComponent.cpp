// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/*=============================================================================
	AttractorComponent.cpp: UAttractorComponent methods.
=============================================================================*/


#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"
#include "../FXSystem.h"
#include "TurbulenceFS.h"


UAttractorComponent::UAttractorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FBoxSphereBounds UAttractorComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds NewBounds;
	NewBounds.Origin = FVector(0.0f);
	NewBounds.BoxExtent = FVector(0.0f);
	NewBounds.SphereRadius = 0.0f;

#if WITH_APEX_TURBULENCE
	if ( AttractorAsset )
	{
		NewBounds.BoxExtent = FVector(AttractorAsset->Radius);
		NewBounds.SphereRadius = AttractorAsset->Radius;
	}
#endif

	return NewBounds.TransformBy( LocalToWorld );
}

void UAttractorComponent::SendRenderTransform_Concurrent()
{
	Super::SendRenderTransform_Concurrent();
	if (FXSystem)
	{
		FXSystem->UpdateFieldSampler(this);
	}
}


void UAttractorComponent::SetConstFieldStrength(float ConstStrength)
{
	ConstFieldStrength = ConstStrength;

	if (FXSystem)
	{
		FXSystem->UpdateFieldSampler(this);
	}
}

void UAttractorComponent::SetVariableFieldStrength(float VariableStrength)
{
	VariableFieldStrength = VariableStrength;

	if (FXSystem)
	{
		FXSystem->UpdateFieldSampler(this);
	}
}

void UAttractorComponent::SetFieldSamplerAsset( UFieldSamplerAsset* Asset )
{
	UAttractorAsset* CastedAsset = Cast<UAttractorAsset>(Asset);
	if (CastedAsset)
	{
		AttractorAsset = CastedAsset;
	}
}

#if WITH_APEX_TURBULENCE
UFieldSamplerAsset* UAttractorComponent::GetFieldSamplerAsset()
{ 
	return AttractorAsset; 
} 

bool UAttractorComponent::CreateFieldSamplerInstance()
{
	if( AttractorAsset )
	{
		ConstFieldStrength = AttractorAsset->ConstFieldStrength;
		VariableFieldStrength = AttractorAsset->VariableFieldStrength;

		FAttractorFSInstance* Instance = new FAttractorFSInstance();
		FieldSamplerInstance = Instance;

		Instance->FSType = EFieldSamplerAssetType::EFSAT_ATTRACTOR;
		Instance->WorldBounds = Bounds.GetBox();
		Instance->Origin = ComponentToWorld.ToMatrixWithScale().GetOrigin();
		Instance->Radius = AttractorAsset->Radius;
		Instance->ConstFieldStrength = ConstFieldStrength;
		Instance->VariableFieldStrength = VariableFieldStrength;
		Instance->bEnabled = bEnabled;
		return true;
	}
	return false;
}

void UAttractorComponent::CreateApexActor(FPhysScene* InPhysScene, UFieldSamplerAsset* InFieldSamplerAsset)
{
	if (World && World->Scene && FieldSamplerInstance == NULL)
	{
		// Store the FX system for the world in which this component is registered.
		check(FXSystem == NULL);
		FXSystem = World->Scene->GetFXSystem();
		check(FXSystem != NULL);

		// Add this component to the FX system.
		FXSystem->AddFieldSampler(this);
	}
}
#endif //WITH_APEX_TURBULENCE

// NVCHANGE_END: JCAO - Add Turbulence actors and components