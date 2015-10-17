// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/*=============================================================================
	HeatSourceComponent.cpp: UHeatSourceComponent methods.
=============================================================================*/


#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"


UHeatSourceComponent::UHeatSourceComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UHeatSourceComponent::SetAverageTemperature(float InAverageTemperature)
{
	AverageTemperature = InAverageTemperature;
	UpdateApexActor();
}

void UHeatSourceComponent::SetStandardTemperature(float InStandardTemperature)
{
	StandardTemperature = InStandardTemperature;
	UpdateApexActor();
}

void UHeatSourceComponent::SetFieldSamplerAsset( UFieldSamplerAsset* Asset )
{
	UHeatSourceAsset* CastedAsset = Cast<UHeatSourceAsset>(Asset);
	if (CastedAsset)
	{
		HeatSourceAsset = CastedAsset;
	}
}

#if WITH_APEX_TURBULENCE
UFieldSamplerAsset* UHeatSourceComponent::GetFieldSamplerAsset()
{ 
	return HeatSourceAsset; 
} 

void UHeatSourceComponent::UpdateApexActor()
{
	FApexHeatSourceActor* ApexHeatSourceActor = static_cast<FApexHeatSourceActor*>(ApexFieldSamplerActor);

	if (ApexHeatSourceActor)
	{
		ApexHeatSourceActor->bEnabled = bEnabled;
		ApexHeatSourceActor->AverageTemperature = AverageTemperature;
		ApexHeatSourceActor->StandardTemperature = StandardTemperature;
		ApexHeatSourceActor->UpdateApexActor();
	}
}

void UHeatSourceComponent::CreateApexActor(FPhysScene* InPhysScene, UFieldSamplerAsset* InFieldSamplerAsset)
{
	AverageTemperature = HeatSourceAsset->AverageTemperature;
	StandardTemperature = HeatSourceAsset->StandardTemperature;

	ApexFieldSamplerActor = new FApexHeatSourceActor(this, InPhysScene, PST_Async, InFieldSamplerAsset);
}
#endif //WITH_APEX_TURBULENCE

// NVCHANGE_END: JCAO - Add Turbulence actors and components