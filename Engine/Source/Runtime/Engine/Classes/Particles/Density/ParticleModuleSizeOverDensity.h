// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles

#pragma once
#include "Particles/Density/ParticleModuleDensityBase.h"
#include "ParticleModuleSizeOverDensity.generated.h"

UCLASS(editinlinenew, hidecategories=Object, MinimalAPI, meta=(DisplayName = "Size Over Density"))
class UParticleModuleSizeOverDensity : public UParticleModuleDensityBase
{
	GENERATED_UCLASS_BODY()

	/**
	 *	The scale factor for the size that should be used for a particle.
	 *	The value is retrieved using the density of the particle during its update.
	 */
	UPROPERTY(EditAnywhere, Category=Size)
	struct FRawDistributionVector SizeOverDensity;

	/** Initializes the default values for this property */
	void InitializeDefaults();

	//Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	virtual void PostInitProperties() override;
	// End UObject Interface

	// Begin UParticleModule Interface
	virtual void CompileModule( struct FParticleEmitterBuildInfo& EmitterInfo ) override;
	virtual void SetToSensibleDefaults(UParticleEmitter* Owner) override;

#if WITH_EDITOR
	virtual bool IsValidForLODLevel(UParticleLODLevel* LODLevel, FString& OutErrorString) override;
#endif

	// End UParticleModule Interface

protected:
	friend class FParticleModuleSizeOverDensityDetails;
};

// NVCHANGE_END: JCAO - Grid Density with GPU particles


