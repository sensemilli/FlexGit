// NVCHANGE_BEGIN: JCAO - Add Field Sampler Module for particles
/*==============================================================================
	ParticleModules_FieldSampler.cpp: Field Sampler module implementations.
==============================================================================*/

#include "EnginePrivate.h"

#include "Particles/FieldSampler/ParticleModuleFieldSamplerBase.h"
#include "Particles/FieldSampler/ParticleModuleFieldSamplerAttractor.h"
#include "Particles/FieldSampler/ParticleModuleFieldSamplerGrid.h"
#include "Particles/FieldSampler/ParticleModuleFieldSamplerHeatSource.h"
#include "Particles/FieldSampler/ParticleModuleFieldSamplerJet.h"
#include "Particles/FieldSampler/ParticleModuleFieldSamplerNoise.h"
#include "Particles/FieldSampler/ParticleModuleFieldSamplerVelocitySource.h"
#include "Particles/FieldSampler/ParticleModuleFieldSamplerVortex.h"

/*------------------------------------------------------------------------------
	Base
------------------------------------------------------------------------------*/
UParticleModuleFieldSamplerBase::UParticleModuleFieldSamplerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

/*------------------------------------------------------------------------------
	Grid
------------------------------------------------------------------------------*/
UParticleModuleFieldSamplerGrid::UParticleModuleFieldSamplerGrid(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UParticleModuleFieldSamplerGrid::CompileModule(FParticleEmitterBuildInfo& EmitterInfo)
{
#if WITH_APEX_TURBULENCE
	FGPUSpriteLocalFieldSamplerInfo LocalFieldSampler;
	LocalFieldSampler.FieldSamplerAsset = GridAsset;
	FTransform LocalTransform;
	LocalTransform.SetTranslation(RelativeTranslation);
	LocalTransform.SetRotation(RelativeRotation.Quaternion());
	LocalFieldSampler.Transform = LocalTransform;
	EmitterInfo.LocalFieldSamplers.Add(LocalFieldSampler);
#endif
}


/*------------------------------------------------------------------------------
	Jet
------------------------------------------------------------------------------*/
UParticleModuleFieldSamplerJet::UParticleModuleFieldSamplerJet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UParticleModuleFieldSamplerJet::CompileModule(FParticleEmitterBuildInfo& EmitterInfo)
{
#if WITH_APEX_TURBULENCE
	FGPUSpriteLocalFieldSamplerInfo LocalFieldSampler;
	LocalFieldSampler.FieldSamplerAsset = JetAsset;
	FTransform LocalTransform;
	LocalTransform.SetTranslation(RelativeTranslation);
	LocalTransform.SetRotation(RelativeRotation.Quaternion());
	LocalFieldSampler.Transform = LocalTransform;
	EmitterInfo.LocalFieldSamplers.Add(LocalFieldSampler);
#endif
}
/*------------------------------------------------------------------------------
	Attractor
------------------------------------------------------------------------------*/
UParticleModuleFieldSamplerAttractor::UParticleModuleFieldSamplerAttractor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UParticleModuleFieldSamplerAttractor::CompileModule(FParticleEmitterBuildInfo& EmitterInfo)
{
#if WITH_APEX_TURBULENCE
	FGPUSpriteLocalFieldSamplerInfo LocalFieldSampler;
	LocalFieldSampler.FieldSamplerAsset = AttractorAsset;
	FTransform LocalTransform;
	LocalTransform.SetTranslation(RelativeTranslation);
	LocalTransform.SetRotation(RelativeRotation.Quaternion());
	LocalFieldSampler.Transform = LocalTransform;
	EmitterInfo.LocalFieldSamplers.Add(LocalFieldSampler);
#endif
}
/*------------------------------------------------------------------------------
	Noise
------------------------------------------------------------------------------*/
UParticleModuleFieldSamplerNoise::UParticleModuleFieldSamplerNoise(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UParticleModuleFieldSamplerNoise::CompileModule(FParticleEmitterBuildInfo& EmitterInfo)
{
#if WITH_APEX_TURBULENCE
	FGPUSpriteLocalFieldSamplerInfo LocalFieldSampler;
	LocalFieldSampler.FieldSamplerAsset = NoiseAsset;
	FTransform LocalTransform;
	LocalTransform.SetTranslation(RelativeTranslation);
	LocalTransform.SetRotation(RelativeRotation.Quaternion());
	LocalFieldSampler.Transform = LocalTransform;
	EmitterInfo.LocalFieldSamplers.Add(LocalFieldSampler);
#endif
}
/*------------------------------------------------------------------------------
	Vortex
------------------------------------------------------------------------------*/
UParticleModuleFieldSamplerVortex::UParticleModuleFieldSamplerVortex(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UParticleModuleFieldSamplerVortex::CompileModule(FParticleEmitterBuildInfo& EmitterInfo)
{
#if WITH_APEX_TURBULENCE
	FGPUSpriteLocalFieldSamplerInfo LocalFieldSampler;
	LocalFieldSampler.FieldSamplerAsset = VortexAsset;
	FTransform LocalTransform;
	LocalTransform.SetTranslation(RelativeTranslation);
	LocalTransform.SetRotation(RelativeRotation.Quaternion());
	LocalFieldSampler.Transform = LocalTransform;
	EmitterInfo.LocalFieldSamplers.Add(LocalFieldSampler);
#endif
}
/*------------------------------------------------------------------------------
	Heat Source
------------------------------------------------------------------------------*/
UParticleModuleFieldSamplerHeatSource::UParticleModuleFieldSamplerHeatSource(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UParticleModuleFieldSamplerHeatSource::CompileModule(FParticleEmitterBuildInfo& EmitterInfo)
{
#if WITH_APEX_TURBULENCE
	FGPUSpriteLocalFieldSamplerInfo LocalFieldSampler;
	LocalFieldSampler.FieldSamplerAsset = HeatSourceAsset;
	FTransform LocalTransform;
	LocalTransform.SetTranslation(RelativeTranslation);
	LocalTransform.SetRotation(RelativeRotation.Quaternion());
	LocalFieldSampler.Transform = LocalTransform;
	EmitterInfo.LocalFieldSamplers.Add(LocalFieldSampler);
#endif
}
/*------------------------------------------------------------------------------
	Velocity Source
------------------------------------------------------------------------------*/
UParticleModuleFieldSamplerVelocitySource::UParticleModuleFieldSamplerVelocitySource(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UParticleModuleFieldSamplerVelocitySource::CompileModule(FParticleEmitterBuildInfo& EmitterInfo)
{
#if WITH_APEX_TURBULENCE
	FGPUSpriteLocalFieldSamplerInfo LocalFieldSampler;
	LocalFieldSampler.FieldSamplerAsset = VelocitySourceAsset;
	FTransform LocalTransform;
	LocalTransform.SetTranslation(RelativeTranslation);
	LocalTransform.SetRotation(RelativeRotation.Quaternion());
	LocalFieldSampler.Transform = LocalTransform;
	EmitterInfo.LocalFieldSamplers.Add(LocalFieldSampler);
#endif
}
// NVCHANGE_END: JCAO - Add Field Sampler Module for particles
