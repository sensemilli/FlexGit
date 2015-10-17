// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components
#pragma once

#include "FieldSampler/FieldSamplerActor.h"
#include "GridActor.generated.h"

UCLASS(MinimalAPI, hidecategories=(Input))
class AGridActor : public AFieldSamplerActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadOnly, Category=FieldSampler)
	class UGridComponent* GridComponent;

};

// NVCHANGE_END: JCAO - Add Turbulence actors and components


