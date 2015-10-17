// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components
#pragma once

#include "FieldSamplerActor.generated.h"

UCLASS(abstract, MinimalAPI, hidecategories=(Input))
class AFieldSamplerActor : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=FieldSampler)
	class UFieldSamplerComponent* FieldSamplerComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UBillboardComponent* SpriteComponent;
#endif
};

// NVCHANGE_END: JCAO - Add Turbulence actors and components


