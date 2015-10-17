// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/*=============================================================================
	NoiseComponent.cpp: UNoiseComponent methods.
=============================================================================*/


#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"
#include "ApexFieldSamplerActor.h"
#include "TurbulenceFS.h"


UNoiseComponent::UNoiseComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FBoxSphereBounds UNoiseComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds NewBounds;
	NewBounds.Origin = FVector(0.0f);
	NewBounds.BoxExtent = FVector(0.0f);
	NewBounds.SphereRadius = 0.0f;

#if WITH_APEX_TURBULENCE
	if ( NoiseAsset )
	{
		NewBounds.BoxExtent = FVector(NoiseAsset->BoundarySize * (NoiseAsset->BoundaryScale * 0.5f));
		NewBounds.SphereRadius = NewBounds.BoxExtent.Size();
	}
#endif

	return NewBounds.TransformBy( LocalToWorld );
}

void UNoiseComponent::SendRenderTransform_Concurrent()
{
	Super::SendRenderTransform_Concurrent();
	if (FXSystem)
	{
		FXSystem->UpdateFieldSampler(this);
	}
}

void UNoiseComponent::SetNoiseStrength(float Noise)
{
	NoiseStrength = Noise;
	UpdateApexActor();
	if (FXSystem)
	{
		FXSystem->UpdateFieldSampler(this);
	}
}

void UNoiseComponent::SetFieldSamplerAsset( UFieldSamplerAsset* Asset )
{
	UNoiseAsset* CastedAsset = Cast<UNoiseAsset>(Asset);
	if (CastedAsset)
	{
		NoiseAsset = CastedAsset;
	}
}

#if WITH_APEX_TURBULENCE
UFieldSamplerAsset* UNoiseComponent::GetFieldSamplerAsset()
{ 
	return NoiseAsset; 
} 

bool UNoiseComponent::CreateFieldSamplerInstance()
{
	if( NoiseAsset )
	{
		NoiseStrength = NoiseAsset->NoiseStrength;

		FNoiseFSInstance* Instance = new FNoiseFSInstance();
		FieldSamplerInstance = Instance;

		Instance->FSType = EFieldSamplerAssetType::EFSAT_NOISE;
		Instance->WorldBounds = Bounds.GetBox();

		Instance->NoiseSpaceFreq = FVector(1.0f / NoiseAsset->NoiseSpacePeriod.X, 1.0f / NoiseAsset->NoiseSpacePeriod.Y, 1.0f / NoiseAsset->NoiseSpacePeriod.Z);
		Instance->NoiseSpaceFreqOctaveMultiplier = FVector(1.0f / NoiseAsset->NoiseSpacePeriodOctaveMultiplier.X, 1.0f / NoiseAsset->NoiseSpacePeriodOctaveMultiplier.Y, 1.0f / NoiseAsset->NoiseSpacePeriodOctaveMultiplier.Z);
		Instance->NoiseStrength = NoiseAsset->NoiseStrength;
		Instance->NoiseTimeFreq = 1.0f / NoiseAsset->NoiseTimePeriod;
		Instance->NoiseStrengthOctaveMultiplier = NoiseAsset->NoiseStrengthOctaveMultiplier;
		Instance->NoiseTimeFreqOctaveMultiplier = 1.0f / NoiseAsset->NoiseTimePeriodOctaveMultiplier;
		Instance->NoiseOctaves = NoiseAsset->NoiseOctaves;
		Instance->NoiseType = NoiseAsset->NoiseType;
		Instance->NoiseSeed = NoiseAsset->NoiseSeed;
		Instance->bEnabled = bEnabled;

		return true;
	}
	return false;
}

void UNoiseComponent::UpdateApexActor()
{
	FApexNoiseActor* ApexNoiseActor = static_cast<FApexNoiseActor*>(ApexFieldSamplerActor);

	if (ApexNoiseActor)
	{
		ApexNoiseActor->bEnabled = bEnabled;
		ApexNoiseActor->NoiseStrength = NoiseStrength;
		ApexNoiseActor->UpdateApexActor();
	}
}

void UNoiseComponent::CreateApexActor(FPhysScene* InPhysScene, UFieldSamplerAsset* InFieldSamplerAsset)
{
	if(NoiseAsset && NoiseAsset->FieldType == EFieldType::FORCE)
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
		return;
	}

	NoiseStrength = NoiseAsset->NoiseStrength;

	ApexFieldSamplerActor = new FApexNoiseActor(this, InPhysScene, PST_Async, InFieldSamplerAsset);
}
#endif //WITH_APEX_TURBULENCE

// NVCHANGE_END: JCAO - Add Turbulence actors and components