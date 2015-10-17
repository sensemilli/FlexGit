// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/**
 *	APEX Turbulence - Vortex Component
 */
#pragma once

#include "FieldSampler/FieldSamplerComponent.h"
#include "VortexComponent.generated.h"

UCLASS(ClassGroup=Physics, config=Engine, editinlinenew, meta=(BlueprintSpawnableComponent),MinimalAPI)
class UVortexComponent : public UFieldSamplerComponent
{
	GENERATED_UCLASS_BODY()

	/** The field sampler asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=VortexFS)
	class UVortexAsset* VortexAsset;

	/** Rotational field strength */
	UPROPERTY(BlueprintReadOnly, Category=VortexFS)
	float	RotationalFieldStrength;

	/** Radial field strength */
	UPROPERTY(BlueprintReadOnly, Category=VortexFS)
	float	RadialFieldStrength;

	/** Lift field strength */
	UPROPERTY(BlueprintReadOnly, Category=VortexFS)
	float	LiftFieldStrength;

	UFUNCTION(BlueprintCallable, Category=VortexFS)
	void SetRotationalFieldStrength(float RotationalStrength);

	UFUNCTION(BlueprintCallable, Category=VortexFS)
	void SetRadialFieldStrength(float RadialStrength);

	UFUNCTION(BlueprintCallable, Category=VortexFS)
	void SetLiftFieldStrength(float LiftStrength);

	virtual void SetFieldSamplerAsset( class UFieldSamplerAsset* Asset ) override;

protected:
#if WITH_APEX_TURBULENCE
	virtual void UpdateApexActor() override;
	virtual class UFieldSamplerAsset* GetFieldSamplerAsset() override;
	virtual void CreateApexActor(FPhysScene* InPhysScene, class UFieldSamplerAsset* InFieldSamplerAsset) override;
#endif

};



// NVCHANGE_END: JCAO - Add Turbulence actors and components