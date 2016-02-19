// NVCHANGE_BEGIN: JCAO - 

#include "EnginePrivate.h"
#include "ApexFieldSamplerAsset.h"
#include "../PhysicsEngine/PhysXSupport.h"


#if WITH_APEX_TURBULENCE

FApexFieldSamplerAsset::FApexFieldSamplerAsset(NxParameterized::Interface* ApexAssetParams)
	: RefCount(0)
	, ApexAsset(0)
{
	ApexAsset = GApexSDK->createAsset(ApexAssetParams, 0);
}

FApexFieldSamplerAsset::FApexFieldSamplerAsset(NxApexAsset* InApexAsset)
	: RefCount(0)
	, ApexAsset(InApexAsset)
{
}

FApexFieldSamplerAsset::~FApexFieldSamplerAsset()
{
	if (ApexAsset)
	{
		ApexAsset->release();
		ApexAsset = NULL;
	}
}

void FApexFieldSamplerAsset::IncreaseRefCount()
{
	RefCount++;
}

void FApexFieldSamplerAsset::DecreaseRefCount()
{
	RefCount--;
	check( RefCount >= 0 );
	if ( RefCount == 0 )
	{
		delete this;
	}
}

#endif // WITH_APEX_TURBULENCE
// NVCHANGE_END: JCAO - 