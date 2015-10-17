// NVCHANGE_BEGIN: JCAO - 
/*==============================================================================
	ApexFieldSamplerAsset.h: 
==============================================================================*/

#pragma once

#if WITH_APEX_TURBULENCE
namespace physx
{
	namespace apex
	{
		class NxApexActor;
		class NxApexAsset;
	}
}

class FApexFieldSamplerAsset
{
public:
	ENGINE_API FApexFieldSamplerAsset(NxParameterized::Interface* ApexAssetParams);
	ENGINE_API FApexFieldSamplerAsset(physx::apex::NxApexAsset* InApexAsset);

	virtual ENGINE_API ~FApexFieldSamplerAsset();

	void IncreaseRefCount();
	void DecreaseRefCount();

	physx::apex::NxApexAsset* GetApexAsset()
	{
		return ApexAsset;
	}

protected:
	physx::apex::NxApexAsset*	ApexAsset;
	int32			RefCount;
};


#endif
// NVCHANGE_END: JCAO - 