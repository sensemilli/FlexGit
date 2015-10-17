// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/*=============================================================================
	VelocitySourceComponent.cpp: UVelocitySourceComponent methods.
=============================================================================*/


#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"


UVelocitySourceComponent::UVelocitySourceComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UVelocitySourceComponent::SetAverageVelocity(float InAverageVelocity)
{
	AverageVelocity = InAverageVelocity;
	UpdateApexActor();
}

void UVelocitySourceComponent::SetStandardVelocity(float InStandardVelocity)
{
	StandardVelocity = InStandardVelocity;
	UpdateApexActor();
}

void UVelocitySourceComponent::SetFieldSamplerAsset( UFieldSamplerAsset* Asset )
{
	UVelocitySourceAsset* CastedAsset = Cast<UVelocitySourceAsset>(Asset);
	if (CastedAsset)
	{
		VelocitySourceAsset = CastedAsset;
	}
}

#if WITH_APEX_TURBULENCE
UFieldSamplerAsset* UVelocitySourceComponent::GetFieldSamplerAsset()
{ 
	return VelocitySourceAsset; 
} 

void UVelocitySourceComponent::UpdateApexActor()
{
	FApexVelocitySourceActor* ApexVelocitySourceActor = static_cast<FApexVelocitySourceActor*>(ApexFieldSamplerActor);

	if (ApexVelocitySourceActor)
	{
		ApexVelocitySourceActor->bEnabled = bEnabled;
		ApexVelocitySourceActor->AverageVelocity = AverageVelocity;
		ApexVelocitySourceActor->StandardVelocity = StandardVelocity;
		ApexVelocitySourceActor->UpdateApexActor();
	}
}

void UVelocitySourceComponent::CreateApexActor(FPhysScene* InPhysScene, UFieldSamplerAsset* InFieldSamplerAsset)
{
	AverageVelocity = VelocitySourceAsset->AverageVelocity;
	StandardVelocity = VelocitySourceAsset->StandardVelocity;

	ApexFieldSamplerActor = new FApexVelocitySourceActor(this, InPhysScene, PST_Async, InFieldSamplerAsset);
}
#endif //WITH_APEX_TURBULENCE

// NVCHANGE_END: JCAO - Add Turbulence actors and components