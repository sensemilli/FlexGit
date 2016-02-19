// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset

#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"

UVortexAsset::UVortexAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Vortex Parameters
	RotationalFieldStrength = 75;
	RadialFieldStrength = -40;
	LiftFieldStrength = 0.0f;
	CapsuleFieldHeight = 100.0f;
	CapsuleFieldBottomRadius = 100.0f;
	CapsuleFieldTopRadius = 100.0f;
	BoundaryFadePercentage = 0.1f;
}

#if WITH_APEX_TURBULENCE
#define ITERATE_ASSET_PARAMS() \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "rotationalStrength", RotationalFieldStrength ) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "radialStrength", RadialFieldStrength) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "liftStrength", LiftFieldStrength) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "boundaryFadePercentage", BoundaryFadePercentage) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "height", CapsuleFieldHeight) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "bottomRadius", CapsuleFieldBottomRadius) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "topRadius", CapsuleFieldTopRadius) \

const char* UVortexAsset::GetApexAssetObjTypeName()
{
	return NX_VORTEX_FS_AUTHORING_TYPE_NAME;
}

void UVortexAsset::SetupApexAssetParams(NxParameterized::Interface* Params)
{
	if (Params != NULL)
	{
		verify(NxParameterized::setParamVec3(*Params, "axis", physx::PxVec3(1.0f, 0.0f, 0.0f)));

#define PROCESS_ASSET_PARAM(_typeName, _type, _paramName, _value) \
		verify(NxParameterized::setParam##_typeName(*Params, _paramName, _value));

		ITERATE_ASSET_PARAMS()
#undef PROCESS_ASSET_PARAM

		//NxParameterized::setParamString(*Params, "fieldSamplerFilterDataName", TCHAR_TO_ANSI(*GetPathName()));
	}
}
#endif // WITH_APEX_TURBULENCE

// NVCHANGE_END: JCAO - Add Field Sampler Asset