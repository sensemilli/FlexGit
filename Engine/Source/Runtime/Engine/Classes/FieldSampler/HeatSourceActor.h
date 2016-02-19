// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components
#pragma once

#include "FieldSampler/FieldSamplerActor.h"
#include "HeatSourceActor.generated.h"

UCLASS(MinimalAPI, hidecategories=(Input))
class AHeatSourceActor : public AFieldSamplerActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadOnly, Category=FieldSampler)
	class UHeatSourceComponent* HeatSourceComponent;

};

// NVCHANGE_END: JCAO - Add Turbulence actors and components


