// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components
#pragma once

#include "FieldSampler/FieldSamplerActor.h"
#include "VelocitySourceActor.generated.h"

UCLASS(MinimalAPI, hidecategories=(Input))
class AVelocitySourceActor : public AFieldSamplerActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadOnly, Category=FieldSampler)
	class UVelocitySourceComponent* VelocitySourceComponent;

};

// NVCHANGE_END: JCAO - Add Turbulence actors and components


