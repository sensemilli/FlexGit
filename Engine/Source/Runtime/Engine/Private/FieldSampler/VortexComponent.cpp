// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/*=============================================================================
	VortexComponent.cpp: UVortexComponent methods.
=============================================================================*/


#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"


UVortexComponent::UVortexComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UVortexComponent::SetRotationalFieldStrength(float RotationalStrength)
{
	RotationalFieldStrength = RotationalStrength;
	UpdateApexActor();
}

void UVortexComponent::SetRadialFieldStrength(float RadialStrength)
{
	RadialFieldStrength = RadialStrength;
	UpdateApexActor();
}

void UVortexComponent::SetLiftFieldStrength(float LiftStrength)
{
	LiftFieldStrength = LiftStrength;
	UpdateApexActor();
}

void UVortexComponent::SetFieldSamplerAsset( UFieldSamplerAsset* Asset )
{
	UVortexAsset* CastedAsset = Cast<UVortexAsset>(Asset);
	if (CastedAsset)
	{
		VortexAsset = CastedAsset;
	}
}

#if WITH_APEX_TURBULENCE
UFieldSamplerAsset* UVortexComponent::GetFieldSamplerAsset()
{ 
	return VortexAsset; 
} 

void UVortexComponent::UpdateApexActor()
{
	FApexVortexActor* ApexVortexActor = static_cast<FApexVortexActor*>(ApexFieldSamplerActor);

	if (ApexVortexActor)
	{
		ApexVortexActor->bEnabled = bEnabled;
		ApexVortexActor->RotationalFieldStrength = RotationalFieldStrength;
		ApexVortexActor->RadialFieldStrength = RadialFieldStrength;
		ApexVortexActor->LiftFieldStrength = LiftFieldStrength;
		ApexVortexActor->UpdateApexActor();
	}
}

void UVortexComponent::CreateApexActor(FPhysScene* InPhysScene, UFieldSamplerAsset* InFieldSamplerAsset)
{
	RotationalFieldStrength = VortexAsset->RotationalFieldStrength;
	RadialFieldStrength = VortexAsset->RadialFieldStrength;
	LiftFieldStrength = VortexAsset->LiftFieldStrength;

	ApexFieldSamplerActor = new FApexVortexActor(this, InPhysScene, PST_Async, InFieldSamplerAsset);
}

#endif //WITH_APEX_TURBULENCE

// NVCHANGE_END: JCAO - Add Turbulence actors and components