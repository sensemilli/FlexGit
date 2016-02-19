// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/**
 *	APEX Turbulence - Jet Component
 */
#pragma once

#include "FieldSampler/FieldSamplerComponent.h"
#include "JetComponent.generated.h"

UCLASS(ClassGroup=Physics, config=Engine, editinlinenew, meta=(BlueprintSpawnableComponent),MinimalAPI)
class UJetComponent : public UFieldSamplerComponent
{
	GENERATED_UCLASS_BODY()

	/** The field sampler asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=JetFS)
	class UJetAsset* JetAsset;

	UPROPERTY(BlueprintReadOnly, Category=JetFS)
	float		FieldStrength;

	UFUNCTION(BlueprintCallable, Category=JetFS)
	void SetFieldStrength(float Field);

	virtual void SetFieldSamplerAsset( class UFieldSamplerAsset* Asset ) override;

protected:
#if WITH_APEX_TURBULENCE
	virtual void UpdateApexActor() override;
	virtual class UFieldSamplerAsset* GetFieldSamplerAsset() override;
	virtual void CreateApexActor(FPhysScene* InPhysScene, class UFieldSamplerAsset* InFieldSamplerAsset) override;
#endif
};



// NVCHANGE_END: JCAO - Add Turbulence actors and components