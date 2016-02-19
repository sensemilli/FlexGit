// NVCHANGE_BEGIN: JCAO - Add Field Sampler Module for particles
/*==============================================================================
	ParticleModuleFieldSamplerGrid: Grid component
==============================================================================*/

#pragma once
#include "Particles/FieldSampler/ParticleModuleFieldSamplerBase.h"
#include "ParticleModuleFieldSamplerGrid.generated.h"

UCLASS(editinlinenew, hidecategories=Object, meta=(DisplayName = "Grid FS"))
class UParticleModuleFieldSamplerGrid : public UParticleModuleFieldSamplerBase
{
	GENERATED_UCLASS_BODY()

	/** Grid Asset */
	UPROPERTY(EditAnywhere, Category=FieldSampler)
	class UGridAsset* GridAsset;

	virtual void CompileModule(struct FParticleEmitterBuildInfo& EmitterInfo) override;
};
// NVCHANGE_END: JCAO - Add Field Sampler Module for particles

