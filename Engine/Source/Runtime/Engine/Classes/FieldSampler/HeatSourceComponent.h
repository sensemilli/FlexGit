// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/**
 *	APEX Turbulence - HeatSource Component
 */
#pragma once

#include "FieldSampler/FieldSamplerComponent.h"
#include "HeatSourceComponent.generated.h"

UCLASS(ClassGroup=Physics, config=Engine, editinlinenew, meta=(BlueprintSpawnableComponent),MinimalAPI)
class UHeatSourceComponent : public UFieldSamplerComponent
{
	GENERATED_UCLASS_BODY()

	/** The field sampler asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=HeatSource)
	class UHeatSourceAsset* HeatSourceAsset;

	UPROPERTY(BlueprintReadOnly, Category=HeatSource)
	float	AverageTemperature;

	UPROPERTY(BlueprintReadOnly, Category=HeatSource)
	float	StandardTemperature;

	UFUNCTION(BlueprintCallable, Category=HeatSource)
	void SetAverageTemperature(float InAverageTemperature);

	UFUNCTION(BlueprintCallable, Category=HeatSource)
	void SetStandardTemperature(float InStandardTemperature);

	virtual void SetFieldSamplerAsset( class UFieldSamplerAsset* Asset ) override;

protected:
#if WITH_APEX_TURBULENCE
	virtual void UpdateApexActor() override;
	virtual class UFieldSamplerAsset* GetFieldSamplerAsset() override;
	virtual void CreateApexActor(FPhysScene* InPhysScene, class UFieldSamplerAsset* InFieldSamplerAsset) override;
#endif

};



// NVCHANGE_END: JCAO - Add Turbulence actors and components