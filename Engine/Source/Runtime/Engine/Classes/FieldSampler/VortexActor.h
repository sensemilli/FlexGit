// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components
#pragma once

#include "FieldSampler/FieldSamplerActor.h"
#include "VortexActor.generated.h"

UCLASS(MinimalAPI, hidecategories=(Input))
class AVortexActor : public AFieldSamplerActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadOnly, Category=FieldSampler)
	class UVortexComponent* VortexComponent;

};

// NVCHANGE_END: JCAO - Add Turbulence actors and components


