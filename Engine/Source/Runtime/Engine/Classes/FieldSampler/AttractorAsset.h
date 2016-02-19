// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset
#pragma once

#include "FieldSampler/FieldSamplerAsset.h"
#include "AttractorAsset.generated.h"


UCLASS(BlueprintType, hidecategories=object, MinimalAPI)
class UAttractorAsset : public UFieldSamplerAsset
{
	GENERATED_UCLASS_BODY()

/** Percentage of distance from boundary to center where fade out starts */
	UPROPERTY(EditAnywhere, Category=AttractorFS, meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float	BoundaryFadePercentage;
/** Attraction radius */
	UPROPERTY(EditAnywhere, Category=AttractorFS, meta=(ClampMin = "0.0"))
	float	Radius;
/** Const attracting force */
	UPROPERTY(EditAnywhere, Category=AttractorFS)
	float	ConstFieldStrength;
/** Field strength coefficient for inverse square part of the formula*/
	UPROPERTY(EditAnywhere, Category=AttractorFS)
	float	VariableFieldStrength;

public:

#if WITH_APEX_TURBULENCE
	virtual const char* GetApexAssetObjTypeName() override;
	virtual void SetupApexAssetParams(NxParameterized::Interface*) override;
#endif

	virtual EFieldSamplerAssetType GetType() override { return EFSAT_ATTRACTOR; }
};
// NVCHANGE_END: JCAO - Add Field Sampler Asset