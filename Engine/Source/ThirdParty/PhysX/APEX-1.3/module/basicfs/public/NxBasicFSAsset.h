/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NX_BASIC_FSASSET_H
#define NX_BASIC_FSASSET_H

#include "NxApex.h"

namespace physx
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

#define NX_JET_FS_AUTHORING_TYPE_NAME	"JetFSAsset"
#define NX_ATTRACTOR_FS_AUTHORING_TYPE_NAME	"AttractorFSAsset"
#define NX_VORTEX_FS_AUTHORING_TYPE_NAME	"VortexFSAsset"
#define NX_NOISE_FS_AUTHORING_TYPE_NAME "NoiseFSAsset"
#define NX_WIND_FS_AUTHORING_TYPE_NAME	"WindFSAsset"

/**
 \brief BasicFS Asset class
 */
class NxBasicFSAsset : public NxApexAsset
{
protected:
	virtual ~NxBasicFSAsset() {}

public:
};

/**
 \brief BasicFS Asset Authoring class
 */
class NxBasicFSAssetAuthoring : public NxApexAssetAuthoring
{
protected:
	virtual ~NxBasicFSAssetAuthoring() {}

public:
};


PX_POP_PACK

}
} // end namespace physx::apex

#endif // NX_BASIC_FSASSET_H
