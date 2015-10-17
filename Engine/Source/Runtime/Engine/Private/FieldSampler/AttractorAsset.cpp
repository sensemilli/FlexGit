// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset

#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"

UAttractorAsset::UAttractorAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Attractor Parameters
	BoundaryFadePercentage = 0.1f;
	Radius = 0.0f;
	ConstFieldStrength = 0.0f;
	VariableFieldStrength = 0.0f;
}

#if WITH_APEX_TURBULENCE
#define ITERATE_ASSET_PARAMS() \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "boundaryFadePercentage", BoundaryFadePercentage) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "radius", Radius) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "constFieldStrength", ConstFieldStrength) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "variableFieldStrength", VariableFieldStrength) \

const char* UAttractorAsset::GetApexAssetObjTypeName()
{
	return NX_ATTRACTOR_FS_AUTHORING_TYPE_NAME;
}

void UAttractorAsset::SetupApexAssetParams(NxParameterized::Interface* Params)
{
	if (Params != NULL)
	{
#define PROCESS_ASSET_PARAM(_typeName, _type, _paramName, _value) \
			verify(NxParameterized::setParam##_typeName(*Params, _paramName, _value));

		ITERATE_ASSET_PARAMS()
#undef PROCESS_ASSET_PARAM
	}
}
#endif // WITH_APEX_TURBULENCE

// NVCHANGE_END: JCAO - Add Field Sampler Asset