/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NX_HEAT_SOURCE_ASSET_H
#define NX_HEAT_SOURCE_ASSET_H

#include "NxApex.h"
#include "NxApexCustomBufferIterator.h"

#define NX_HEAT_SOURCE_AUTHORING_TYPE_NAME "HeatSourceAsset"

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
class NxHeatSourceAsset : public NxApexAsset
{
protected:
	virtual ~NxHeatSourceAsset() {}
};


/*
brief Turbulence FieldSampler Asset Authoring class
 */
class NxHeatSourceAssetAuthoring : public NxApexAssetAuthoring
{
protected:
	virtual ~NxHeatSourceAssetAuthoring() {}
};


}
} // end namespace physx::apex

#endif // NX_HEAT_SOURCE_ASSET_H
