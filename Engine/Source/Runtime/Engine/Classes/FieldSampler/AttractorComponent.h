// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/**
 *	APEX Turbulence - Attractor Component
 */
#pragma once

#include "FieldSampler/FieldSamplerComponent.h"
#include "AttractorComponent.generated.h"

UCLASS(ClassGroup=Physics, config=Engine, editinlinenew, meta=(BlueprintSpawnableComponent),MinimalAPI)
class UAttractorComponent : public UFieldSamplerComponent
{
	GENERATED_UCLASS_BODY()

	/** The field sampler asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AttractorFS)
	class UAttractorAsset* AttractorAsset;

	UPROPERTY(BlueprintReadOnly, Category=AttractorFS)
	float	ConstFieldStrength;

	UPROPERTY(BlueprintReadOnly, Category=AttractorFS)
	float	VariableFieldStrength;

	UFUNCTION(BlueprintCallable, Category=AttractorFS)
	void SetConstFieldStrength(float ConstStrength);

	UFUNCTION(BlueprintCallable, Category=AttractorFS)
	void SetVariableFieldStrength(float VariableStrength);

	virtual void SetFieldSamplerAsset( class UFieldSamplerAsset* Asset ) override;


protected:
#if WITH_APEX_TURBULENCE
	virtual class UFieldSamplerAsset* GetFieldSamplerAsset() override;
	virtual bool CreateFieldSamplerInstance() override;
	virtual void CreateApexActor(FPhysScene* InPhysScene, class UFieldSamplerAsset* InFieldSamplerAsset) override;
#endif

	// Begin UActorComponent Interface
	virtual void SendRenderTransform_Concurrent() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const override;
	// End UActorComponent Interface
};



// NVCHANGE_END: JCAO - Add Turbulence actors and components