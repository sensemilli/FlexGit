/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NX_FLAME_EMITTER_ASSET_H
#define NX_FLAME_EMITTER_ASSET_H

#include "NxApex.h"
#include "NxApexCustomBufferIterator.h"

#define NX_FLAME_EMITTER_AUTHORING_TYPE_NAME "FlameEmitterAsset"

namespace physx
{
namespace apex
{

/**
 \brief Flame Emitter Asset class
 */
class NxFlameEmitterAsset : public NxApexAsset
{
protected:
	virtual ~NxFlameEmitterAsset() {}
};


/*
brief Flame Emitter Asset Authoring class
 */
class NxFlameEmitterAssetAuthoring : public NxApexAssetAuthoring
{
protected:
	virtual ~NxFlameEmitterAssetAuthoring() {}
};


}
} // end namespace physx::apex

#endif // NX_FLAME_EMITTER_ASSET_H
