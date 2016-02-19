// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/**
 *	APEX Turbulence - Noise Component
 */
#pragma once

#include "FieldSampler/FieldSamplerComponent.h"
#include "NoiseComponent.generated.h"

UCLASS(ClassGroup=Physics, config=Engine, editinlinenew, meta=(BlueprintSpawnableComponent),MinimalAPI)
class UNoiseComponent : public UFieldSamplerComponent
{
	GENERATED_UCLASS_BODY()

	/** The field sampler asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=NoiseFS)
	class UNoiseAsset* NoiseAsset;

	UPROPERTY(BlueprintReadOnly, Category=NoiseFS)
	float		NoiseStrength;

	UFUNCTION(BlueprintCallable, Category=NoiseFS)
	void SetNoiseStrength(float Noise);

	virtual void SetFieldSamplerAsset( class UFieldSamplerAsset* Asset ) override;

protected:
#if WITH_APEX_TURBULENCE
	virtual void UpdateApexActor() override;
	virtual class UFieldSamplerAsset* GetFieldSamplerAsset() override;
	virtual void CreateApexActor(FPhysScene* InPhysScene, class UFieldSamplerAsset* InFieldSamplerAsset) override;
	virtual bool CreateFieldSamplerInstance() override;
#endif

	// Begin UActorComponent Interface
	virtual void SendRenderTransform_Concurrent() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const override;
	// End UActorComponent Interface

};



// NVCHANGE_END: JCAO - Add Turbulence actors and components