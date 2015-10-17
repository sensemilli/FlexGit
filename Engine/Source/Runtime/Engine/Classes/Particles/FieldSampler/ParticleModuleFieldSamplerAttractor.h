// NVCHANGE_BEGIN: JCAO - Add Field Sampler Module for particles
/*==============================================================================
	ParticleModuleFieldSamplerAttractor: Attractor component
==============================================================================*/

#pragma once
#include "Particles/FieldSampler/ParticleModuleFieldSamplerBase.h"
#include "ParticleModuleFieldSamplerAttractor.generated.h"

UCLASS(editinlinenew, hidecategories=Object, meta=(DisplayName = "Attractor FS"))
class UParticleModuleFieldSamplerAttractor : public UParticleModuleFieldSamplerBase
{
	GENERATED_UCLASS_BODY()

	/** Attractor Asset */
	UPROPERTY(EditAnywhere, Category=FieldSampler)
	class UAttractorAsset* AttractorAsset;

	virtual void CompileModule(struct FParticleEmitterBuildInfo& EmitterInfo) override;
};
// NVCHANGE_END: JCAO - Add Field Sampler Module for particles
