// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components
#pragma once

#include "FieldSampler/FieldSamplerActor.h"
#include "AttractorActor.generated.h"

UCLASS(MinimalAPI, hidecategories=(Input))
class AAttractorActor : public AFieldSamplerActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadOnly, Category=FieldSampler)
	class UAttractorComponent* AttractorComponent;

};

// NVCHANGE_END: JCAO - Add Turbulence actors and components


