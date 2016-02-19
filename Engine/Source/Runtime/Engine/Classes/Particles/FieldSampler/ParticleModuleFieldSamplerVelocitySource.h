// NVCHANGE_BEGIN: JCAO - Add Field Sampler Module for particles
/*==============================================================================
	ParticleModuleFieldSamplerVelocitySource: VelocitySource component
==============================================================================*/

#pragma once
#include "Particles/FieldSampler/ParticleModuleFieldSamplerBase.h"
#include "ParticleModuleFieldSamplerVelocitySource.generated.h"

UCLASS(editinlinenew, hidecategories=Object, meta=(DisplayName = "Velocity Source"))
class UParticleModuleFieldSamplerVelocitySource : public UParticleModuleFieldSamplerBase
{
	GENERATED_UCLASS_BODY()

	/** VelocitySource Asset */
	UPROPERTY(EditAnywhere, Category=FieldSampler)
	class UVelocitySourceAsset* VelocitySourceAsset;

	virtual void CompileModule(struct FParticleEmitterBuildInfo& EmitterInfo) override;
};
// NVCHANGE_END: JCAO - Add Field Sampler Module for particles
