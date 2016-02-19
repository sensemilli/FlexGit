// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset
#pragma once

#include "FieldSampler/FieldSamplerAsset.h"
#include "GridAsset.generated.h"

UENUM()
enum EGridResolution
{
	EGR_12,
	EGR_16,
	EGR_24,
	EGR_32,
	EGR_48,
	EGR_64,
	EGR_96,
	EGR_128,
};

USTRUCT()
struct FGridFloatRange
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=Grid)
	float Min;

	UPROPERTY(EditAnywhere, Category=Grid)
	float Max;
};

UCLASS(BlueprintType, hidecategories=object, MinimalAPI)
class UGridAsset : public UFieldSamplerAsset
{
	GENERATED_UCLASS_BODY()

	
	/** Min resolution of grid for fluid simulation */
	UPROPERTY(EditAnywhere, Category=Grid)
	TEnumAsByte<EGridResolution>		GridMinResolution;
	
	/** Max resolution of grid for fluid simulation */
	UPROPERTY(EditAnywhere, Category=Grid)
	TEnumAsByte<EGridResolution>		GridMaxResolution;

	/** TBD */
	UPROPERTY(EditAnywhere, Category=Grid)
	FGridFloatRange	UpdatesPerFrameRange;

	/** Grid scale */
	UPROPERTY(EditAnywhere, Category=Grid)
	float		GridScale;

	/** Grid size */
	UPROPERTY(EditAnywhere, Category=Grid, meta=( DisplayName = "Grid Size" ))
	FVector		GridSize3D;

	/** Fraction of distance from boundary to center where grid falloff starts. Effect of grid is 0 outside the boundary. */
	UPROPERTY(EditAnywhere, Category=Grid, meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float		BoundaryFadePercentage;
	
	/** Boundary size as a percentage of grid size */
	UPROPERTY(EditAnywhere, Category=Grid, meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float		BoundarySizePercentage;

	/** Constant fluid velocity */
	UPROPERTY(EditAnywhere, Category=Grid)
	FVector		ExternalVelocity;

	/** Keep the same forces but multiply the force vectors */
	UPROPERTY(EditAnywhere, Category=Grid)
	float		FieldVelocityMultiplier;

	UPROPERTY(EditAnywhere, Category=Grid, meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float		FieldVelocityWeight;

	UPROPERTY(EditAnywhere, Category=Grid)
	uint32		UseHeat : 1;

	/** Coefficient for deriving drag force from the field velocity (if equals to zero, direct field velocity is used) */
	UPROPERTY(EditAnywhere, Category=Grid, meta=(ClampMin = "0.0"))
	float		DragCoeff;

	/** Coefficient for mixing slip (=0) and no-slip (=1) boundary condition on rigid bodies */
	UPROPERTY(EditAnywhere, Category=Grid, meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float		DragCoeffForRigidBody;
	/** Viscosity is the coefficient for velocity diffusion, if it equals to 0 then there is no diffusion */
	UPROPERTY(EditAnywhere, Category=Grid, meta=(ClampMin = "0.0"))
	float		FluidViscosity;

	/** Time of velocity field cleaning process [sec]. Cleaning disabled by default. */
	UPROPERTY(EditAnywhere, Category=Grid, meta=(ClampMin = "0.0", ClampMax = "1000.0"))
	float		VelocityFieldFadeTime;

	/** Time without activity before velocity field cleaning process starts [sec]. */
	UPROPERTY(EditAnywhere, Category=Grid, meta=(ClampMin = "0.0", ClampMax = "1000.0"))
	float		VelocityFieldFadeDelay;

	/** This parameter sets how much each velocity vector should be damped, so (1 - velocityFieldFadeIntensity) of original velocity vector magnitude will be left after the fade process finishes. */
	UPROPERTY(EditAnywhere, Category=Grid, meta=(ClampMin = "0.01", ClampMax = "1.0"))
	float		VelocityFieldFadeIntensity;

	/** Multiply the effect of rigid body angular velocity on the grid simulation */
	UPROPERTY(EditAnywhere, Category=RBCollision)
	float		AngularVelocityMultiplier;
	/** Clamp the effect of rigid body angular velocity on the grid simulation */	
	UPROPERTY(EditAnywhere, Category=RBCollision)
	float		AngularVelocityClamp;
	/** Multiply the effect of rigid body linear velocity on the grid simulation */
	UPROPERTY(EditAnywhere, Category=RBCollision)
	float		LinearVelocityMultiplier;
	/** Clamp the effect of rigid body linear velocity on the grid simulation */
	UPROPERTY(EditAnywhere, Category=RBCollision)
	float		LinearVelocityClamp;
	/** Maximum number of colliding objects (affects performance) */
	UPROPERTY(EditAnywhere, Category=RBCollision, meta=(ClampMin = "0"))
	int32			MaxCollidingObjects;


	/** Maximum number of heat sources (affects performance) */
	/** Note: removed from UI for now, until heat sources are implemented */
	UPROPERTY(EditAnywhere, Category=Heat, meta=(ClampMin = "0"))
	int32 		MaxHeatSources;

	UPROPERTY(EditAnywhere, Category=Heat)
	float		TemperatureBasedForceMultiplier;

	UPROPERTY(EditAnywhere, Category=Heat)
	float		AmbientTemperature;

	UPROPERTY(EditAnywhere, Category=Heat)
	FVector		HeatForceDirection;


	/** Noise strength */
	UPROPERTY(EditAnywhere, Category=Noise, meta=(ClampMin = "0.0"))
	float		NoiseStrength;
	/** Somewhat controls the size of the turbulent vortices caused by the noise */
	UPROPERTY(EditAnywhere, Category=Noise)
	FVector		NoiseSpacePeriod;
	/** Somewhat controls the frequency of occurrence of the turbulent vortices caused by the noise */
	UPROPERTY(EditAnywhere, Category=Noise)
	float		NoiseTimePeriod;
	/** More octaves give more turbulent noise, but increase computational time */
	UPROPERTY(EditAnywhere, Category=Noise, meta=(ClampMin = "0"))
	int32		NoiseOctaves;


public:

#if WITH_APEX_TURBULENCE
	virtual const char* GetApexAssetObjTypeName() override;
	virtual void SetupApexAssetParams(NxParameterized::Interface*) override;
#endif

	virtual EFieldSamplerAssetType GetType() override { return EFSAT_GRID; }
};
// NVCHANGE_END: JCAO - Add Field Sampler Asset