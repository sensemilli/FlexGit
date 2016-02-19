// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset
#pragma once

#include "FieldSampler/FieldSamplerAsset.h"
#include "NoiseAsset.generated.h"

UENUM()
enum EFieldType
{
	FORCE,
	VELOCITY_DRAG,
	VELOCITY_DIRECT
};

UENUM()
enum ENoiseType
{
	SIMPLEX,
	CURL
};

UCLASS(BlueprintType, hidecategories=object, MinimalAPI)
class UNoiseAsset : public UFieldSamplerAsset
{
	GENERATED_UCLASS_BODY()


/** Boundary scale */
	UPROPERTY(EditAnywhere, Category=NoiseFS)
	float		BoundaryScale;
/** Boundary size */
	UPROPERTY(EditAnywhere, Category=NoiseFS)
	FVector		BoundarySize;

/** Percentage of distance from boundary to center where fade out starts */
	UPROPERTY(EditAnywhere, Category=NoiseFS, meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float		BoundaryFadePercentage;


/** Type of field */
	UPROPERTY(EditAnywhere, Category=NoiseFS)
	TEnumAsByte<EFieldType>	FieldType;

/** Field drag coefficient (only for VELOCITY_DRAG field type) */
	UPROPERTY(EditAnywhere, Category=NoiseFS, meta=(ClampMin = "0.0"))
	float		FieldDragCoeff;


/** Type of noise */
	UPROPERTY(EditAnywhere, Category=NoiseFS)
	TEnumAsByte<ENoiseType>	NoiseType;

/** Seed for the noise random generator */
	UPROPERTY(EditAnywhere, Category=NoiseFS)
	int32			NoiseSeed;

/** Noise strength */
	UPROPERTY(EditAnywhere, Category=NoiseFS, meta=(ClampMin = "0.0"))
	float		NoiseStrength;
/** Noise period in space */
	UPROPERTY(EditAnywhere, Category=NoiseFS)
	FVector		NoiseSpacePeriod;
/** Noise period in time */
	UPROPERTY(EditAnywhere, Category=NoiseFS)
	float		NoiseTimePeriod;
/** More octaves give more turbulent noise, but increase computational time */
	UPROPERTY(EditAnywhere, Category=NoiseFS, meta=(ClampMin = "0"))
	int32			NoiseOctaves;

/** Noise strength multiplier from one octave to the next */
	UPROPERTY(EditAnywhere, Category=NoiseFS, meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float		NoiseStrengthOctaveMultiplier;
/** Noise period in space multiplier from one octave to the next. Works for 0.0f ~ 1.0f. */
	UPROPERTY(EditAnywhere, Category=NoiseFS)
	FVector		NoiseSpacePeriodOctaveMultiplier;
/** Noise period in time multiplier from one octave to the next */
	UPROPERTY(EditAnywhere, Category=NoiseFS, meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float		NoiseTimePeriodOctaveMultiplier;

public:

#if WITH_APEX_TURBULENCE
	virtual const char* GetApexAssetObjTypeName() override;
	virtual void SetupApexAssetParams(NxParameterized::Interface*) override;
#endif

	virtual EFieldSamplerAssetType GetType() override { return EFSAT_NOISE; }
};
// NVCHANGE_END: JCAO - Add Field Sampler Asset