// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset

#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"

UJetAsset::UJetAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Jet Parameters
	FieldDirectionDeviationAngle = 0.0f;
	FieldDirectionOscillationPeriod = 0.0f;
	FieldStrength = 75.0f;
	FieldStrengthDeviationPercentage = 0.0f;
	FieldStrengthOscillationPeriod = 0.0f;
	
	FieldSize = 10.0f;
	FieldNearRatio = 1.0f;
	FieldFarRatio = 4.0f;
	FieldPivot = 0.0f;
	DirectionalStretch = 1.0f;
	AverageStartRatio = 1.0f;
	AverageEndRatio = 5.0f;
	BoundaryFadePercentage = 0.1f;

	GridShapeRadius = 100.0f;
	GridShapeHeight = 200.0f;
	GridBoundaryFadePercentage = 0.01f;
	
	NoisePercentage = 0.0f;
	NoiseSpaceScale = 1.0f;
	NoiseTimeScale = 1.0f;
	NoiseOctaves = 1;
}

#if WITH_APEX_TURBULENCE
#define ITERATE_ASSET_PARAMS() \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "fieldDirectionDeviationAngle", FieldDirectionDeviationAngle) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "fieldDirectionOscillationPeriod", FieldDirectionOscillationPeriod) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "fieldStrength", FieldStrength) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "fieldStrengthDeviationPercentage", FieldStrengthDeviationPercentage) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "fieldStrengthOscillationPeriod", FieldStrengthOscillationPeriod) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "gridShapeRadius", GridShapeRadius) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "gridShapeHeight", GridShapeHeight) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "gridBoundaryFadePercentage", GridBoundaryFadePercentage) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "boundaryFadePercentage", BoundaryFadePercentage) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "nearRadius", FieldSize * FieldNearRatio) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "pivotRadius", FieldSize * (FieldNearRatio + (FieldFarRatio - FieldNearRatio)*(FieldPivot + 1.0f)*0.5f)) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "farRadius", FieldSize * FieldFarRatio) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "directionalStretch", DirectionalStretch) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "averageStartDistance", FieldSize * AverageStartRatio) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "averageEndDistance", FieldSize * AverageEndRatio) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "noisePercentage", NoisePercentage) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "noiseSpaceScale", NoiseSpaceScale) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "noiseTimeScale", NoiseTimeScale) \
	PROCESS_ASSET_PARAM(U32,  physx::PxU32,  "noiseOctaves", NoiseOctaves) \

const char* UJetAsset::GetApexAssetObjTypeName()
{
	return NX_JET_FS_AUTHORING_TYPE_NAME;
}

void UJetAsset::SetupApexAssetParams(NxParameterized::Interface* Params)
{
	if (Params != NULL)
	{
		verify(NxParameterized::setParamF32(*Params, "defaultScale", 1.0f));
		verify(NxParameterized::setParamVec3(*Params, "fieldDirection", physx::PxVec3(1, 0, 0)));
#define PROCESS_ASSET_PARAM(_typeName, _type, _paramName, _value) \
		verify(NxParameterized::setParam##_typeName(*Params, _paramName, _value));

		ITERATE_ASSET_PARAMS()
#undef PROCESS_ASSET_PARAM
	}
}
#endif // WITH_APEX_TURBULENCE

// NVCHANGE_END: JCAO - Add Field Sampler Asset