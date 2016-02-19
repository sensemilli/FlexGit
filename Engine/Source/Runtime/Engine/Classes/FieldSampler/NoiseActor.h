// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components
#pragma once

#include "FieldSampler/FieldSamplerActor.h"
#include "NoiseActor.generated.h"

UCLASS(MinimalAPI, hidecategories=(Input))
class ANoiseActor : public AFieldSamplerActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadOnly, Category=FieldSampler)
	class UNoiseComponent* NoiseComponent;

};

// NVCHANGE_END: JCAO - Add Turbulence actors and components


