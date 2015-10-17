// NVCHANGE_BEGIN: JCAO - Add Field Sampler Module for particles
/*==============================================================================
	ParticleModuleFieldSamplerHeatSource: HeatSource component
==============================================================================*/

#pragma once
#include "Particles/FieldSampler/ParticleModuleFieldSamplerBase.h"
#include "ParticleModuleFieldSamplerHeatSource.generated.h"

UCLASS(editinlinenew, hidecategories=Object, meta=(DisplayName = "Heat Source"))
class UParticleModuleFieldSamplerHeatSource : public UParticleModuleFieldSamplerBase
{
	GENERATED_UCLASS_BODY()

	/** HeatSource Asset */
	UPROPERTY(EditAnywhere, Category=FieldSampler)
	class UHeatSourceAsset* HeatSourceAsset;

	virtual void CompileModule(struct FParticleEmitterBuildInfo& EmitterInfo) override;
};
// NVCHANGE_END: JCAO - Add Field Sampler Module for particles
