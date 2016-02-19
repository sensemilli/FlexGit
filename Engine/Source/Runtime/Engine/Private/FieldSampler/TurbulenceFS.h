// NVCHANGE_BEGIN: JCAO - Turbulence FS
/*==============================================================================
	TurbulenceFS.h: TurbulenceFS instance
==============================================================================*/

#pragma once

#if WITH_APEX_TURBULENCE
namespace physx
{
	namespace apex
	{
		class NxTurbulenceFSActor;
	}
}
#endif

class FApexTurbulenceActor;

class FFieldSamplerInstance
{
public:
	int32 Index;
	EFieldSamplerAssetType FSType;
	FBox WorldBounds;
	bool bEnabled;

	FFieldSamplerInstance()
		: Index(INDEX_NONE)
		, FSType(EFieldSamplerAssetType::EFSAT_NONE)
		, bEnabled(false)
	{
	}

	virtual ~FFieldSamplerInstance()
	{
	}

	virtual void UpdateTransforms(const FMatrix& LocalToWorld) {}
};

/**
 */
class FTurbulenceFSInstance : public FFieldSamplerInstance
{
public:

#if WITH_APEX_TURBULENCE
	//physx::apex::NxTurbulenceFSActor* TurbulenceFSActor;
	FApexTurbulenceActor* TurbulenceFSActor;
#endif

	FMatrix VolumeToWorldNoScale;
	FMatrix WorldToVolume;
	FMatrix VolumeToWorld;

	FBox LocalBounds;

	float VelocityMultiplier;
	float VelocityWeight;

	/** Default constructor. */
	FTurbulenceFSInstance()
		: VelocityMultiplier(0)
		, VelocityWeight(0)
#if WITH_APEX_TURBULENCE		
		, TurbulenceFSActor(0)
#endif // WITH_APEX_TURBULENCE
	{
	}

	/** Destructor. */
	virtual ~FTurbulenceFSInstance()
	{
#if WITH_APEX_TURBULENCE
		TurbulenceFSActor = 0;
#endif
	}

	virtual void UpdateTransforms(const FMatrix& LocalToWorld) override;

};

/** A list of TurbulenceFS instances. */
typedef TSparseArray<FTurbulenceFSInstance*> FTurbulenceFSInstanceList;


class FAttractorFSInstance : public FFieldSamplerInstance
{
public:
	FVector Origin;
	float	Radius;
	float	ConstFieldStrength;
	float	VariableFieldStrength;

	FAttractorFSInstance()
		: Radius(0)
		, ConstFieldStrength(0)
		, VariableFieldStrength(0)
	{
	}

	virtual ~FAttractorFSInstance()
	{
	}

	virtual void UpdateTransforms(const FMatrix& LocalToWorld) override;
};

typedef TSparseArray<FAttractorFSInstance*> FAttractorFSInstanceList;

class FNoiseFSInstance : public FFieldSamplerInstance
{
public:
	FVector NoiseSpaceFreq;
	FVector NoiseSpaceFreqOctaveMultiplier;
	float NoiseStrength;
	float NoiseTimeFreq;
	float NoiseStrengthOctaveMultiplier;
	float NoiseTimeFreqOctaveMultiplier;
	int32 NoiseOctaves;
	int32 NoiseType;
	int32 NoiseSeed;

	FNoiseFSInstance()
		: NoiseStrength(0)
		, NoiseTimeFreq(0)
		, NoiseStrengthOctaveMultiplier(0)
		, NoiseTimeFreqOctaveMultiplier(0)
		, NoiseOctaves(0)
		, NoiseType(0)
		, NoiseSeed(0)
	{
	}

	virtual ~FNoiseFSInstance()
	{
	}
};

typedef TSparseArray<FNoiseFSInstance*> FNoiseFSInstanceList;
// NVCHANGE_END: JCAO - Turbulence FS