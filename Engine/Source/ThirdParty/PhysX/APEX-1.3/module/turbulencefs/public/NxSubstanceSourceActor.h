/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NX_SUBSTANCE_SOURCE_ACTOR_H
#define NX_SUBSTANCE_SOURCE_ACTOR_H

#include "NxApex.h"
#include "NxApexShape.h"

namespace physx
{
namespace apex
{

class NxSubstanceSourceAsset;


/**
 \brief Turbulence ScalarSource Actor class
 */
class NxSubstanceSourceActor : public NxApexActor//, public NxApexRenderable
{
protected:
	virtual ~NxSubstanceSourceActor() {}

public:
	 /**
	 \brief Returns the asset the instance has been created from.
	 */
	virtual NxSubstanceSourceAsset*			getSubstanceSourceAsset() const = 0;

	virtual void setEnabled(bool enable) = 0;

	virtual bool isEnabled() const = 0;

	/*enable whether or not to use density in the simulation (enabling density reduces performance).<BR>
	If you are enabling density then you also need to add substance sources (without substance sources you will see no effect of density on the simulation, except a drop in performance)
	 */

	///intersect the collision shape against a given AABB
	virtual bool						intersectAgainstAABB(physx::PxBounds3) = 0;

	virtual  NxApexShape* 				getShape() const = 0;

	///If it is a box, cast to box class, return NULL otherwise
	virtual NxApexBoxShape* 			getBoxShape() = 0;

	///If it is a sphere, cast to sphere class, return NULL otherwise
	virtual NxApexSphereShape* 			getSphereShape() = 0;

	///Return average value of density
	virtual PxF32						getAverageDensity() const = 0;

	///Retrun STD value of density
	virtual PxF32						getStdDensity() const = 0;

	///Set average and STD values for density
	virtual void						setDensity(PxF32 averageDensity, PxF32 stdDensity) = 0;

	///Sets the uniform overall object scale
	virtual void				setCurrentScale(PxF32 scale) = 0;

	//Retrieves the uniform overall object scale
	virtual PxF32				getCurrentScale(void) const = 0;

	virtual void						release() = 0;
};


}
} // end namespace physx::apex

#endif 
