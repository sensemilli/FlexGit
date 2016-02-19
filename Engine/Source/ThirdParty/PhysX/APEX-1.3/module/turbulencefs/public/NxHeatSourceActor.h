/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NX_HEAT_SOURCE_ACTOR_H
#define NX_HEAT_SOURCE_ACTOR_H

#include "NxApex.h"
#include "NxApexShape.h"

namespace physx
{
namespace apex
{

class NxHeatSourceAsset;


/**
 \brief Turbulence HeatSource Actor class
 */
class NxHeatSourceActor : public NxApexActor//, public NxApexRenderable
{
protected:
	virtual ~NxHeatSourceActor() {}

public:
	 /**
	 \brief Returns the asset the instance has been created from.
	 */
	virtual NxHeatSourceAsset*			getHeatSourceAsset() const = 0;


	//enable/disable the heating
	virtual void setEnabled(bool enable) = 0;

	virtual bool isEnabled() const = 0;

	///intersect the collision shape against a given AABB
	virtual bool						intersectAgainstAABB(physx::PxBounds3) = 0;

	virtual  NxApexShape* 				getShape() const = 0;

	///If it is a box, cast to box class, return NULL otherwise
	virtual NxApexBoxShape* 			getBoxShape() = 0;

	///If it is a sphere, cast to sphere class, return NULL otherwise
	virtual NxApexSphereShape* 			getSphereShape() = 0;

	///Return average value of temperature
	virtual PxF32						getAverageTemperature() const = 0;

	///Retrun STD value of temperature
	virtual PxF32						getStdTemperature() const = 0;

	//get the pose of a heat source shape
	virtual physx::PxMat44				getPose() const = 0;

	///Set average and STD values for temperature
	virtual void						setTemperature(PxF32 averageTemperature, PxF32 stdTemperature) = 0;

	//set the pose of a heat source shape
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
