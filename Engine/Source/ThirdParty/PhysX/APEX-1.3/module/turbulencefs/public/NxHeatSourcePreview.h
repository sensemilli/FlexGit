/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NX_HEAT_SOURCE_PREVIEW_H
#define NX_HEAT_SOURCE_PREVIEW_H

#include "NxApex.h"
#include "NxApexAssetPreview.h"

namespace physx
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief This class provides the asset preview for APEX HeatSource Assets.
*/
class NxHeatSourcePreview : public NxApexAssetPreview
{
public:
	public:
	///Draws the emitter 
	virtual void	drawHeatSourcePreview() = 0;
	///Sets the scaling factor of the renderable
	virtual void	setScale(physx::PxF32 scale) = 0;

protected:
	NxHeatSourcePreview() {}
};


PX_POP_PACK

}
} // namespace physx::apex

#endif // NX_HEAT_SOURCE_PREVIEW_H