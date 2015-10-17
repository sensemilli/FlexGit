// NVCHANGE_BEGIN: JCAO - Add Field Sampler Module for particles
/*==============================================================================
	ParticleModuleFieldSamplerJet: Jet component
==============================================================================*/

#pragma once
#include "Particles/FieldSampler/ParticleModuleFieldSamplerBase.h"
#include "ParticleModuleFieldSamplerJet.generated.h"

UCLASS(editinlinenew, hidecategories=Object, meta=(DisplayName = "Jet FS"))
class UParticleModuleFieldSamplerJet : public UParticleModuleFieldSamplerBase
{
	GENERATED_UCLASS_BODY()

	/** Jet Asset */
	UPROPERTY(EditAnywhere, Category=FieldSampler)
	class UJetAsset* JetAsset;

	virtual void CompileModule(struct FParticleEmitterBuildInfo& EmitterInfo) override;
};
// NVCHANGE_END: JCAO - Add Field Sampler Module for particles

