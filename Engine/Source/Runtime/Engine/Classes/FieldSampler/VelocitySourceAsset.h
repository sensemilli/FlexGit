// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset
#pragma once

#include "FieldSampler/FieldSamplerAsset.h"
#include "VelocitySourceAsset.generated.h"

UCLASS(BlueprintType, hidecategories=object, MinimalAPI)
class UVelocitySourceAsset : public UFieldSamplerAsset
{
	GENERATED_UCLASS_BODY()


	/** The arithmetic mean of a sample. Normal distribution parameter. */
	UPROPERTY(EditAnywhere, Category=VelocitySource)
	float	AverageVelocity;
	
	/** Standard deviation. Normal distribution parameter. */
	UPROPERTY(EditAnywhere, Category=VelocitySource)
	float	StandardVelocity;

	UPROPERTY(EditAnywhere, Category=VelocitySource)
	TEnumAsByte<EGeometryType>		GeometryType;

	/** Radius of Geometry Sphere. */
	UPROPERTY(EditAnywhere, Category=VelocitySource)
	float	Radius;

	/** Extents of Geometry Box*/
	UPROPERTY(EditAnywhere, Category=VelocitySource)
	FVector Extents;

public:

#if WITH_APEX_TURBULENCE
	virtual const char* GetApexAssetObjTypeName() override;
	virtual void SetupApexAssetParams(NxParameterized::Interface*) override;
#endif

	virtual EFieldSamplerAssetType GetType() override { return EFSAT_VELOCITY_SOURCE; }
};
// NVCHANGE_END: JCAO - Add Field Sampler Asset