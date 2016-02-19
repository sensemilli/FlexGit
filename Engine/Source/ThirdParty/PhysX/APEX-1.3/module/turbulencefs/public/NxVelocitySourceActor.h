/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NX_VELOCITY_SOURCE_ACTOR_H
#define NX_VELOCITY_SOURCE_ACTOR_H

#include "NxApex.h"
#include "NxApexShape.h"

namespace physx
{
namespace apex
{

class NxVelocitySourceAsset;


/**
 \brief Turbulence VelocitySource Actor class
 */
class NxVelocitySourceActor : public NxApexActor//, public NxApexRenderable
{
protected:
	virtual ~NxVelocitySourceActor() {}

public:
	 /**
	 \brief Returns the asset the instance has been created from.
	 */
	virtual NxVelocitySourceAsset*			getVelocitySourceAsset() const = 0;


	//enable/disable the velocity source
	virtual void setEnabled(bool enable) = 0;

	virtual bool isEnabled() const = 0;

	///intersect the collision shape against a given AABB
	virtual bool						intersectAgainstAABB(physx::PxBounds3) = 0;

	virtual  NxApexShape* 				getShape() const = 0;

	///If it is a box, cast to box class, return NULL otherwise
	virtual NxApexBoxShape* 			getBoxShape() = 0;

	///If it is a sphere, cast to sphere class, return NULL otherwise
	virtual NxApexSphereShape* 			getSphereShape() = 0;

	///Return average value of velocity
	virtual PxF32						getAverageVelocity() const = 0;

	///Retrun STD value of velocity
	virtual PxF32						getStdVelocity() const = 0;

	//get the pose of a velocity source shape
	virtual physx::PxMat44				getPose() const = 0;

	///Set average and STD values for velocity
	virtual void						setVelocity(PxF32 averageVelocity, PxF32 stdVelocity) = 0;

	//set the pose of a velocity source shape
	virtual void						setPose(physx::PxMat44 pose) = 0;

	///Sets the uniform overall object scale
	virtual void				setCurrentScale(PxF32 scale) = 0;

	//Retrieves the uniform overall object scale
	virtual PxF32				getCurrentScale(void) const = 0;

	virtual void						release() = 0;
};


}
} // end namespace physx::apex

#endif 
