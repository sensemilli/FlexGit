// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/**
 *	APEX Turbulence - VelocitySource Component
 */
#pragma once

#include "FieldSampler/FieldSamplerComponent.h"
#include "VelocitySourceComponent.generated.h"

UCLASS(ClassGroup=Physics, config=Engine, editinlinenew, meta=(BlueprintSpawnableComponent),MinimalAPI)
class UVelocitySourceComponent : public UFieldSamplerComponent
{
	GENERATED_UCLASS_BODY()

	/** The field sampler asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=VelocitySource)
	class UVelocitySourceAsset* VelocitySourceAsset;

	UPROPERTY(BlueprintReadOnly, Category=VelocitySource)
	float	AverageVelocity;

	UPROPERTY(BlueprintReadOnly, Category=VelocitySource)
	float	StandardVelocity;

	UFUNCTION(BlueprintCallable, Category=VelocitySource)
	void SetAverageVelocity(float InAverageVelocity);

	UFUNCTION(BlueprintCallable, Category=VelocitySource)
	void SetStandardVelocity(float InStandardVelocity);

	virtual void SetFieldSamplerAsset( class UFieldSamplerAsset* Asset ) override;

protected:
#if WITH_APEX_TURBULENCE
	virtual void UpdateApexActor() override;
	virtual class UFieldSamplerAsset* GetFieldSamplerAsset() override;
	virtual void CreateApexActor(FPhysScene* InPhysScene, class UFieldSamplerAsset* InFieldSamplerAsset) override;
#endif

};



// NVCHANGE_END: JCAO - Add Turbulence actors and components