/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NX_FLAME_EMITTER_ACTOR_H
#define NX_FLAME_EMITTER_ACTOR_H

#include "NxApex.h"
#include "NxApexShape.h"

namespace physx
{
namespace apex
{

class NxFlameEmitterAsset;


/**
 \brief Flame Emitter Actor class
 */
class NxFlameEmitterActor : public NxApexActor
{
protected:
	virtual ~NxFlameEmitterActor() {}

public:
	 /**
	 \brief Returns the asset the instance has been created from.
	 */
	virtual NxFlameEmitterAsset*		getFlameEmitterAsset() const = 0;


	//enable/disable
	virtual void setEnabled(bool enable) = 0;

	virtual bool isEnabled() const = 0;

	///intersect the collision shape against a given AABB
	virtual bool						intersectAgainstAABB(const physx::PxBounds3&) const = 0;

	//get the global pose
	virtual physx::PxMat44				getPose() const = 0;

	//set the global pose
	virtual void						setPose(physx::PxMat44 pose) = 0;

	///Sets the uniform overall object scale
	virtual void						setCurrentScale(PxF32 scale) = 0;

	//Retrieves the uniform overall object scale
	virtual PxF32						getCurrentScale(void) const = 0;

	virtual void						release() = 0;
};


}
} // end namespace physx::apex

#endif 
