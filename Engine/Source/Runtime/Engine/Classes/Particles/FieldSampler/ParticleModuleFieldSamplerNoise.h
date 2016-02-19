// NVCHANGE_BEGIN: JCAO - Add Field Sampler Module for particles
/*==============================================================================
	ParticleModuleFieldSamplerNoise: Noise component
==============================================================================*/

#pragma once
#include "Particles/FieldSampler/ParticleModuleFieldSamplerBase.h"
#include "ParticleModuleFieldSamplerNoise.generated.h"

UCLASS(editinlinenew, hidecategories=Object, meta=(DisplayName = "Noise FS"))
class UParticleModuleFieldSamplerNoise : public UParticleModuleFieldSamplerBase
{
	GENERATED_UCLASS_BODY()

	/** Noise Asset */
	UPROPERTY(EditAnywhere, Category=FieldSampler)
	class UNoiseAsset* NoiseAsset;

	virtual void CompileModule(struct FParticleEmitterBuildInfo& EmitterInfo) override;
};
// NVCHANGE_END: JCAO - Add Field Sampler Module for particles
