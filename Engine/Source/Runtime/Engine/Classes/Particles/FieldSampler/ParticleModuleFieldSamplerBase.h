// NVCHANGE_BEGIN: JCAO - Add Field Sampler Module for particles
/*==============================================================================
	ParticleModuleFieldSamplerBase: Base class for organizing field sampler
		related modules.
==============================================================================*/

#pragma once
#include "ParticleModuleFieldSamplerBase.generated.h"

UCLASS(editinlinenew, hidecategories=Object, abstract, meta=(DisplayName = "Field Sampler"))
class UParticleModuleFieldSamplerBase : public UParticleModule
{
	GENERATED_UCLASS_BODY()

	/** Translation of the field sampler relative to the emitter. */
	UPROPERTY(EditAnywhere, Category=FieldSampler)
	FVector RelativeTranslation;

	/** Rotation of the field sampler relative to the emitter. */
	UPROPERTY(EditAnywhere, Category=FieldSampler)
	FRotator RelativeRotation;
};
// NVCHANGE_END: JCAO - Add Field Sampler Module for particles

