// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components
#pragma once

#include "FieldSampler/FieldSamplerActor.h"
#include "JetActor.generated.h"

UCLASS(MinimalAPI, hidecategories=(Input))
class AJetActor : public AFieldSamplerActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadOnly, Category=FieldSampler)
	class UJetComponent* JetComponent;

};

// NVCHANGE_END: JCAO - Add Turbulence actors and components


