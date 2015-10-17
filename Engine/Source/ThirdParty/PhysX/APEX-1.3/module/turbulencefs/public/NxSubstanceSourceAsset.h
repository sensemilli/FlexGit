/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NX_SUBSTANCE_SOURCE_ASSET_H
#define NX_SUBSTANCE_SOURCE_ASSET_H

#include "NxApex.h"
#include "NxApexCustomBufferIterator.h"

#define NX_SUBSTANCE_SOURCE_AUTHORING_TYPE_NAME "SubstanceSourceAsset"

namespace physx
{
namespace apex
{

class TurbulenceFSActorParams;
class NxTurbulenceFSActor;
class NxTurbulenceFSPreview;

/**
 \brief Turbulence FieldSampler Asset class
 */
class NxSubstanceSourceAsset : public NxApexAsset
{
protected:
	virtual ~NxSubstanceSourceAsset() {}
};


/*
brief Turbulence FieldSampler Asset Authoring class
 */
class NxSubstanceSourceAssetAuthoring : public NxApexAssetAuthoring
{
protected:
	virtual ~NxSubstanceSourceAssetAuthoring() {}
};


}
} // end namespace physx::apex

#endif // NX_SUBSTANCE_SOURCE_ASSET_H
