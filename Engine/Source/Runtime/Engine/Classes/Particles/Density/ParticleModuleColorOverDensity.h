// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles

#pragma once
#include "Particles/Density/ParticleModuleDensityBase.h"
#include "ParticleModuleColorOverDensity.generated.h"

UCLASS(editinlinenew, hidecategories=Object, MinimalAPI, meta=(DisplayName = "Color Over Density"))
class UParticleModuleColorOverDensity : public UParticleModuleDensityBase
{
	GENERATED_UCLASS_BODY()

	/** The color to apply to the particle, as a function of the particle Density. */
	UPROPERTY(EditAnywhere, Category=Color)
	struct FRawDistributionVector ColorOverDensity;

	/** The alpha to apply to the particle, as a function of the particle Density. */
	UPROPERTY(EditAnywhere, Category=Color)
	struct FRawDistributionFloat AlphaOverDensity;

	/** If true, the alpha value will be clamped to the [0..1] range. */
	UPROPERTY(EditAnywhere, Category=Color)
	uint32 bClampAlpha:1;

	/** Initializes the default values for this property */
	void InitializeDefaults();

	// Begin UObject Interface
#if WITH_EDITOR
	virtual	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	virtual void PostInitProperties() override;
	// End UObject Interface


	//Begin UParticleModule Interface
	virtual	bool AddModuleCurvesToEditor(UInterpCurveEdSetup* EdSetup, TArray<const FCurveEdEntry*>& OutCurveEntries) override;
	virtual void CompileModule( struct FParticleEmitterBuildInfo& EmitterInfo ) override;
	virtual void SetToSensibleDefaults(UParticleEmitter* Owner) override;

#if WITH_EDITOR
	virtual bool IsValidForLODLevel(UParticleLODLevel* LODLevel, FString& OutErrorString) override;
#endif
	//End UParticleModule Interface

protected:
	friend class FParticleModuleColorOverDensityDetails;
};
// NVCHANGE_END: JCAO - Grid Density with GPU particles

