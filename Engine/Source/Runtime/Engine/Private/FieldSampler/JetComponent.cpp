// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/*=============================================================================
	JetComponent.cpp: UJetComponent methods.
=============================================================================*/


#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"


UJetComponent::UJetComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UJetComponent::SetFieldStrength(float Field)
{
	FieldStrength = Field;
	UpdateApexActor();
}

void UJetComponent::SetFieldSamplerAsset( UFieldSamplerAsset* Asset )
{
	UJetAsset* CastedAsset = Cast<UJetAsset>(Asset);
	if (CastedAsset)
	{
		JetAsset = CastedAsset;
	}
}

#if WITH_APEX_TURBULENCE
UFieldSamplerAsset* UJetComponent::GetFieldSamplerAsset()
{ 
	return JetAsset; 
} 

void UJetComponent::UpdateApexActor()
{
	FApexJetActor* ApexJetActor = static_cast<FApexJetActor*>(ApexFieldSamplerActor);

	if (ApexJetActor)
	{
		ApexJetActor->bEnabled = bEnabled;
		ApexJetActor->FieldStrength = FieldStrength;
		ApexJetActor->UpdateApexActor();
	}
}

void UJetComponent::CreateApexActor(FPhysScene* InPhysScene, UFieldSamplerAsset* InFieldSamplerAsset)
{
	FieldStrength = JetAsset->FieldStrength;

	ApexFieldSamplerActor = new FApexJetActor(this, InPhysScene, PST_Async, InFieldSamplerAsset);
}
#endif //WITH_APEX_TURBULENCE

// NVCHANGE_END: JCAO - Add Turbulence actors and components