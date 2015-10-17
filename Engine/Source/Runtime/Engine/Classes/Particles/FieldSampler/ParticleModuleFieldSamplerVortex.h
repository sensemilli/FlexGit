// NVCHANGE_BEGIN: JCAO - Add Field Sampler Module for particles
/*==============================================================================
	ParticleModuleFieldSamplerVortex: Vortex component
==============================================================================*/

#pragma once
#include "Particles/FieldSampler/ParticleModuleFieldSamplerBase.h"
#include "ParticleModuleFieldSamplerVortex.generated.h"

UCLASS(editinlinenew, hidecategories=Object, meta=(DisplayName = "Vortex FS"))
class UParticleModuleFieldSamplerVortex : public UParticleModuleFieldSamplerBase
{
	GENERATED_UCLASS_BODY()

	/** Vortex Asset */
	UPROPERTY(EditAnywhere, Category=FieldSampler)
	class UVortexAsset* VortexAsset;

	virtual void CompileModule(struct FParticleEmitterBuildInfo& EmitterInfo) override;
};
// NVCHANGE_END: JCAO - Add Field Sampler Module for particles
