/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NX_JET_FSPREVIEW_H
#define NX_JET_FSPREVIEW_H

#include "NxApex.h"
#include "NxApexAssetPreview.h"

namespace physx
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class NxApexRenderDebug;

/**
	\brief The APEX_JET namespace contains the defines for setting the preview detail levels.

	JET_DRAW_NOTHING - draw nothing<BR>
	JET_DRAW_SHAPE - draw the shape<BR>
	JET_DRAW_ASSET_INFO - draw the asset info in the screen space<BR>
	JET_DRAW_FULL_DETAIL - draw all components of the preview<BR>
*/
namespace APEX_JET
{
/**
	\def JET_DRAW_NOTHING
	Draw no preview items.
*/
static const physx::PxU32 JET_DRAW_NOTHING = 0x00;
/**
	\def JET_DRAW_SHAPE
	Draw the box.  The top of the box should be above ground and the bottom should be below ground.
*/
static const physx::PxU32 JET_DRAW_SHAPE = 0x01;
/**
	\def JET_DRAW_ASSET_INFO
	Draw the Asset Info in Screen Space.  This displays the various asset members that are relevant to the current asset.
	parameters that are not relevant because they are disabled are not displayed.
*/
static const physx::PxU32 JET_DRAW_ASSET_INFO = 0x02;
/**
	\def JET_DRAW_FULL_DETAIL
	Draw all of the preview rendering items.
*/
static const physx::PxU32 JET_DRAW_FULL_DETAIL = (JET_DRAW_SHAPE + JET_DRAW_ASSET_INFO);
}

/**
\brief This class provides the asset preview for APEX TurbulenceFS Assets.  The class provides multiple levels of prevew
detail that can be selected individually.
*/
class NxJetFSPreview : public NxApexAssetPreview
{
public:
	/**
	Set the detail level of the preview rendering by selecting which features to enable.<BR>
	Any, all, or none of the following masks may be added together to select what should be drawn.<BR>
	The defines for the individual items are:<br>
		JET_DRAW_NOTHING - draw nothing<BR>
		JET_DRAW_BOX - draw the box<BR>
		JET_DRAW_COLLISION_IMPLICIT - draw the collision implicit<BR>
		JET_DRAW_GRIDS - draw the top and bottom grids of the box<BR>
		JET_DRAW_ASSET_INFO - draw the turbulencefs asset info in the screen space<BR>
		JET_DRAW_FULL_DETAIL - draw all components of the turbulencefs preview<BR>
	All items may be drawn using the macro DRAW_FULL_DETAIL.
	*/
	virtual void	setDetailLevel(physx::PxU32 detail) = 0;
};


PX_POP_PACK

}
} // namespace physx::apex

#endif // NX_JET_FSPREVIEW_H
