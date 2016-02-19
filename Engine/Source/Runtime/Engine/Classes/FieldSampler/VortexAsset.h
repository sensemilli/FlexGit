// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset
#pragma once

#include "FieldSampler/FieldSamplerAsset.h"
#include "VortexAsset.generated.h"


UCLASS(BlueprintType, hidecategories=object, MinimalAPI)
class UVortexAsset : public UFieldSamplerAsset
{
	GENERATED_UCLASS_BODY()

	/** Rotational field strength */
	UPROPERTY(EditAnywhere, Category=VortexFS)
	float	RotationalFieldStrength;

	/** Radial field strength */
	UPROPERTY(EditAnywhere, Category=VortexFS)
	float	RadialFieldStrength;

	/** Lift field strength */
	UPROPERTY(EditAnywhere, Category=VortexFS)
	float	LiftFieldStrength;

	/** Height of capsule field */
	UPROPERTY(EditAnywhere, Category=VortexFS, meta=(ClampMin = "0.0"))
	float	CapsuleFieldHeight;

	/** Bottom radius of capsule field */
	UPROPERTY(EditAnywhere, Category=VortexFS, meta=(ClampMin = "0.0"))
	float	CapsuleFieldBottomRadius;

	/** Top radius of capsule field */
	UPROPERTY(EditAnywhere, Category=VortexFS, meta=(ClampMin = "0.0"))
	float	CapsuleFieldTopRadius;

	/** Percentage of distance from boundary to center where fade out starts */
	UPROPERTY(EditAnywhere, Category=VortexFS, meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float	BoundaryFadePercentage;


public:

#if WITH_APEX_TURBULENCE
	virtual const char* GetApexAssetObjTypeName() override;
	virtual void SetupApexAssetParams(NxParameterized::Interface*) override;
#endif
	virtual EFieldSamplerAssetType GetType() override { return EFSAT_VORTEX; }
};
// NVCHANGE_END: JCAO - Add Field Sampler Asset