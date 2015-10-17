/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NX_TURBULENCE_FSASSET_H
#define NX_TURBULENCE_FSASSET_H

#include "NxApex.h"
#include "NxApexCustomBufferIterator.h"

#define NX_TURBULENCE_FS_AUTHORING_TYPE_NAME "TurbulenceFSAsset"

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
class NxTurbulenceFSAsset : public NxApexAsset
{
protected:
	virtual ~NxTurbulenceFSAsset() {}

};

/**
 \brief Turbulence FieldSampler Asset Authoring class
 */
class NxTurbulenceFSAssetAuthoring : public NxApexAssetAuthoring
{
protected:
	virtual ~NxTurbulenceFSAssetAuthoring() {}

};

}
} // end namespace physx::apex

#endif // NX_TURBULENCE_FSASSET_H
