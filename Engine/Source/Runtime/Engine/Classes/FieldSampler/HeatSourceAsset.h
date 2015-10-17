// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset
#pragma once

#include "FieldSampler/FieldSamplerAsset.h"
#include "HeatSourceAsset.generated.h"

UCLASS(BlueprintType, hidecategories=object, MinimalAPI)
class UHeatSourceAsset : public UFieldSamplerAsset
{
	GENERATED_UCLASS_BODY()


	/** The arithmetic mean of a sample. Normal distribution parameter. */
	UPROPERTY(EditAnywhere, Category=HeatSource)
	float	AverageTemperature;
	
	/** Standard deviation. Normal distribution parameter. */
	UPROPERTY(EditAnywhere, Category=HeatSource)
	float	StandardTemperature;

	UPROPERTY(EditAnywhere, Category=HeatSource)
	TEnumAsByte<EGeometryType>		GeometryType;

	UPROPERTY(EditAnywhere, Category=HeatSource)
	float	Radius;

	UPROPERTY(EditAnywhere, Category=HeatSource)
	FVector Extents;

public:

#if WITH_APEX_TURBULENCE
	virtual const char* GetApexAssetObjTypeName() override;
	virtual void SetupApexAssetParams(NxParameterized::Interface*) override;
#endif

	virtual EFieldSamplerAssetType GetType() override { return EFSAT_HEAT_SOURCE; }
};
// NVCHANGE_END: JCAO - Add Field Sampler Asset