// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset

#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"

UGridAsset::UGridAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Grid Parameters
	GridMinResolution=EGR_16;
	GridMaxResolution=EGR_32;

	UpdatesPerFrameRange.Min = 0.0f;
	UpdatesPerFrameRange.Max = 1.0f;
	GridScale = 700.0f;
	GridSize3D = FVector(1.0f, 1.0f, 1.0f);
	BoundaryFadePercentage = 0.1f;
	BoundarySizePercentage = 1.0f;
	MaxCollidingObjects = 8;
	MaxHeatSources = 4;
	ExternalVelocity = FVector(0.0f, 0.0f, 0.0f);
	AngularVelocityMultiplier = 1.0f;
	AngularVelocityClamp = 1000000.0f;
	LinearVelocityMultiplier = 1.0f;
	LinearVelocityClamp = 1000000.0f;
	FieldVelocityMultiplier = 1.0f;
	FieldVelocityWeight = 1.0f;
	UseHeat = true;

	TemperatureBasedForceMultiplier = 0.02f;
	AmbientTemperature = 0.0f;
	HeatForceDirection = FVector(0.0f, 0.0f, 1.0f);
	DragCoeff = 0;
	DragCoeffForRigidBody = 0;
	FluidViscosity = 0;

	VelocityFieldFadeTime = 1;
	VelocityFieldFadeDelay = 2;
	VelocityFieldFadeIntensity = 0.01f;

	NoiseStrength = 0.0f;
	NoiseSpacePeriod = FVector(0.9f, 0.9f, 0.9f);
	NoiseTimePeriod = 10.0f;
	NoiseOctaves = 1;
}

#if WITH_APEX_TURBULENCE
#define ITERATE_ASSET_PARAMS() \
	PROCESS_ASSET_PARAM_ENUM("gridXRange.min", GridMinResolutionEnum) \
	PROCESS_ASSET_PARAM_ENUM("gridXRange.max", GridMaxResolutionEnum) \
	PROCESS_ASSET_PARAM_ENUM("gridYRange.min", GridMinResolutionEnum) \
	PROCESS_ASSET_PARAM_ENUM("gridYRange.max", GridMaxResolutionEnum) \
	PROCESS_ASSET_PARAM_ENUM("gridZRange.min", GridMinResolutionEnum) \
	PROCESS_ASSET_PARAM_ENUM("gridZRange.max", GridMaxResolutionEnum) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "updatesPerFrameRange.min", UpdatesPerFrameRange.Min) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "updatesPerFrameRange.max", UpdatesPerFrameRange.Max) \
	PROCESS_ASSET_PARAM(Vec3, physx::PxVec3, "gridSizeWorld", U2PVector(GridScale * GridSize3D)) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "boundaryFadePercentage", BoundaryFadePercentage) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "boundarySizePercentage", BoundarySizePercentage) \
	PROCESS_ASSET_PARAM(U32,  physx::PxU32,  "maxCollidingObjects", MaxCollidingObjects) \
	PROCESS_ASSET_PARAM(U32,  physx::PxU32,  "maxHeatSources", MaxHeatSources) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "dragCoeff", DragCoeff) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "dragCoeffForRigidBody", DragCoeffForRigidBody) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "fluidViscosity", FluidViscosity) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32, "velocityFieldFadeTime", VelocityFieldFadeTime) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32, "velocityFieldFadeDelay", VelocityFieldFadeDelay) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32, "velocityFieldFadeIntensity", VelocityFieldFadeIntensity) \
	PROCESS_ASSET_PARAM(Bool,  bool,  "useHeat", UseHeat) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "heatParams.temperatureBasedForceMultiplier", TemperatureBasedForceMultiplier) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "heatParams.ambientTemperature", AmbientTemperature) \
	PROCESS_ASSET_PARAM(Vec3,  physx::PxVec3,  "heatParams.heatForceDirection", U2PVector(HeatForceDirection)) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "noiseParams.noiseStrength", NoiseStrength) \
	PROCESS_ASSET_PARAM(Vec3,  physx::PxVec3,  "noiseParams.noiseSpacePeriod", U2PVector(NoiseSpacePeriod)) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "noiseParams.noiseTimePeriod", NoiseTimePeriod) \
	PROCESS_ASSET_PARAM(U32,  physx::PxU32,  "noiseParams.noiseOctaves", NoiseOctaves) \


static char* EnumValFromGridResolutionEnum(EGridResolution GridResolutionEnum)
{
	static char* TurbulenceRangeEnum[] = { "EGR_12", "EGR_16", "EGR_24", "EGR_32", "EGR_48", "EGR_64", "EGR_96", "EGR_128" };
	check((int)GridResolutionEnum < sizeof(TurbulenceRangeEnum) / sizeof(TurbulenceRangeEnum[0]));
	return TurbulenceRangeEnum[GridResolutionEnum];
}

const char* UGridAsset::GetApexAssetObjTypeName()
{
	return NX_TURBULENCE_FS_AUTHORING_TYPE_NAME;
}

void UGridAsset::SetupApexAssetParams(NxParameterized::Interface* Params)
{
	if (Params != NULL)
	{
		const char* GridMinResolutionEnum = EnumValFromGridResolutionEnum((EGridResolution)GridMinResolution);
		const char* GridMaxResolutionEnum = EnumValFromGridResolutionEnum((EGridResolution)GridMaxResolution);

#define PROCESS_ASSET_PARAM_ENUM(_paramName, _value) \
			verify(NxParameterized::setParamEnum(*Params, _paramName, _value));
#define PROCESS_ASSET_PARAM(_typeName, _type, _paramName, _value) \
			verify(NxParameterized::setParam##_typeName(*Params, _paramName, _value));
		ITERATE_ASSET_PARAMS()
#undef PROCESS_ASSET_PARAM
#undef PROCESS_ASSET_PARAM_ENUM
	}
}
#endif // WITH_APEX_TURBULENCE

// NVCHANGE_END: JCAO - Add Field Sampler Asset