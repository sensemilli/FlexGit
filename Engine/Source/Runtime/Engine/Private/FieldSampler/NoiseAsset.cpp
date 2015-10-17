// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset

#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"

UNoiseAsset::UNoiseAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Noise Parameters
	BoundaryScale = 700.0f;
	BoundarySize = FVector(1.0f, 1.0f, 1.0f);

	BoundaryFadePercentage = 0.1f;

	FieldType = VELOCITY_DIRECT;
	FieldDragCoeff = 10;

	NoiseType = CURL;
	NoiseSeed = 0;

	NoiseStrength = 0.1f;
	NoiseSpacePeriod = FVector(10.0f, 10.0f, 10.0f);
	NoiseTimePeriod = 10.0f;
	NoiseOctaves = 1;
	NoiseStrengthOctaveMultiplier = 0.5f;
	NoiseSpacePeriodOctaveMultiplier = FVector(0.5f, 0.5f, 0.5f);
	NoiseTimePeriodOctaveMultiplier = 0.5f;
}

#if WITH_APEX_TURBULENCE
#define ITERATE_ASSET_PARAMS() \
	PROCESS_ASSET_PARAM(Vec3, physx::PxVec3, "boundarySize", U2PVector(BoundaryScale * BoundarySize)) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "boundaryFadePercentage", BoundaryFadePercentage) \
	PROCESS_ASSET_PARAM_ENUM("fieldType", ResolveFieldTypeEnum((EFieldType)FieldType)) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "fieldDragCoeff", FieldDragCoeff) \
	PROCESS_ASSET_PARAM_ENUM("noiseType", ResolveNoiseTypeEnum((ENoiseType)NoiseType)) \
	PROCESS_ASSET_PARAM(U32,  physx::PxU32,  "noiseSeed", NoiseSeed) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "noiseStrength", NoiseStrength) \
	PROCESS_ASSET_PARAM(Vec3, physx::PxVec3, "noiseSpacePeriod", U2PVector(NoiseSpacePeriod)) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "noiseTimePeriod", NoiseTimePeriod) \
	PROCESS_ASSET_PARAM(U32,  physx::PxU32,  "noiseOctaves", NoiseOctaves) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "noiseStrengthOctaveMultiplier", NoiseStrengthOctaveMultiplier) \
	PROCESS_ASSET_PARAM(Vec3, physx::PxVec3, "noiseSpacePeriodOctaveMultiplier", U2PVector(NoiseSpacePeriodOctaveMultiplier)) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "noiseTimePeriodOctaveMultiplier", NoiseTimePeriodOctaveMultiplier) \

static const char* ResolveFieldTypeEnum(EFieldType value)
{
	static const char* EnumValues[] = { "FORCE", "VELOCITY_DRAG", "VELOCITY_DIRECT" };
	check((int)value < sizeof(EnumValues) / sizeof(EnumValues[0]));
	return EnumValues[value];
}

static const char* ResolveNoiseTypeEnum(ENoiseType value)
{
	static const char* EnumValues[] = { "SIMPLEX", "CURL" };
	check((int)value < sizeof(EnumValues) / sizeof(EnumValues[0]));
	return EnumValues[value];
}

const char* UNoiseAsset::GetApexAssetObjTypeName()
{
	return NX_NOISE_FS_AUTHORING_TYPE_NAME;
}

void UNoiseAsset::SetupApexAssetParams(NxParameterized::Interface* Params)
{
	if (Params != NULL)
	{
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