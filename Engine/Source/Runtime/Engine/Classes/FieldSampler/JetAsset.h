// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset
#pragma once

#include "FieldSampler/FieldSamplerAsset.h"
#include "JetAsset.generated.h"


UCLASS(BlueprintType, hidecategories=object, MinimalAPI)
class UJetAsset : public UFieldSamplerAsset
{
	GENERATED_UCLASS_BODY()


/** Radius of sphere or capsule shape INSIDE the grid */
	UPROPERTY(EditAnywhere, Category=Grid, meta=(ClampMin = "0.0"))
	float	GridShapeRadius;
/** Height of capsule shape INSIDE the grid */
	UPROPERTY(EditAnywhere, Category=Grid, meta=(ClampMin = "0.0"))
	float	GridShapeHeight;

/** Fraction of distance from boundary of the grid shape to center where jet falloff starts. Strength is 0 outside the grid.  */
	UPROPERTY(EditAnywhere, Category=Grid, meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float	GridBoundaryFadePercentage;

/** Field strength */
	UPROPERTY(EditAnywhere, Category=FieldSampler)
	float	FieldStrength;

/** Noise level in percentage of Field Strength */
	UPROPERTY(EditAnywhere, Category=OutOfGrid, meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float	NoisePercentage;

/** Somewhat controls the size of the turbulent vortices caused by the noise */
	UPROPERTY(EditAnywhere, Category=OutOfGrid)
	float	NoiseSpaceScale;
/** Somewhat controls the frequency of occurrence of the turbulent vortices caused by the noise */
	UPROPERTY(EditAnywhere, Category=OutOfGrid)
	float	NoiseTimeScale;
/** More octaves give more turbulent noise, but increase computational time */
	UPROPERTY(EditAnywhere, Category=OutOfGrid, meta=(ClampMin = "0"))
	int32		NoiseOctaves;
/** Field direction deviation angle in degrees */
	UPROPERTY(EditAnywhere, Category=OutOfGrid, meta=(ClampMin = "0.0", ClampMax = "90.0"))
	float	FieldDirectionDeviationAngle;
/** Field direction oscillation period in seconds */
	UPROPERTY(EditAnywhere, Category=OutOfGrid, meta=(ClampMin = "0.0"))
	float	FieldDirectionOscillationPeriod;
/** Field strength deviation percentage. For example, a value of 0.5 would result in the FieldStrength deviating between 50% and 150% */
	UPROPERTY(EditAnywhere, Category=OutOfGrid, meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float	FieldStrengthDeviationPercentage;
/** Field strength oscillation period in seconds */
	UPROPERTY(EditAnywhere, Category=OutOfGrid, meta=(ClampMin = "0.0"))
	float	FieldStrengthOscillationPeriod;
/** Size of the toroidal field */
	UPROPERTY(EditAnywhere, Category=OutOfGrid, meta=(ClampMin = "0.0"))
	float	FieldSize;
/** Near radius ratio of the toroidal field. The field is strongest inside this radius */
	UPROPERTY(EditAnywhere, Category=OutOfGrid, meta=(ClampMin = "0.0"))
	float	FieldNearRatio;
/** Far radius ratio of the toroidal field. The field is empty outside this radius */
	UPROPERTY(EditAnywhere, Category=OutOfGrid, meta=(ClampMin = "0.0"))
	float	FieldFarRatio;
/** Center of the field where it reaches zero */
	UPROPERTY(EditAnywhere, Category=OutOfGrid, meta=(ClampMin = "-0.99999", ClampMax = "0.99999"))
	float	FieldPivot;
/** Stretch of the toroidal field along the direction, relative to Field Size. If set to 1, the cross-section of the toroid is a circle. */
	UPROPERTY(EditAnywhere, Category=OutOfGrid, meta=(ClampMin = "0.0"))
	float	DirectionalStretch;
/** Start distance ratio of blending between averaged & oscillating fields, relative to Field Size */
	UPROPERTY(EditAnywhere, Category=OutOfGrid, meta=(ClampMin = "0.0"))
	float	AverageStartRatio;
/** End distance ratio of blending between averaged & oscillating fields, relative to Field Size */
	UPROPERTY(EditAnywhere, Category=OutOfGrid, meta=(ClampMin = "0.0"))
	float	AverageEndRatio;
/** Fraction of distance from boundary of the toroidal field to center where jet falloff starts. Strength is 0 outside the toroid. */
	UPROPERTY(EditAnywhere, Category=OutOfGrid, meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float	BoundaryFadePercentage;

public:

#if WITH_APEX_TURBULENCE
	virtual const char* GetApexAssetObjTypeName() override;
	virtual void SetupApexAssetParams(NxParameterized::Interface*) override;
#endif
	virtual EFieldSamplerAssetType GetType() override { return EFSAT_JET; }
};
// NVCHANGE_END: JCAO - Add Field Sampler Asset