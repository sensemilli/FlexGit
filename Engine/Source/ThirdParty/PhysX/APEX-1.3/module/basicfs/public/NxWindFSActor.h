/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#ifndef NX_WIND_FSACTOR_H
#define NX_WIND_FSACTOR_H

#include "NxApex.h"

namespace physx
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class NxBasicFSAsset;


/**
 \brief WindFS Actor class
 */
class NxWindFSActor : public NxApexActor, public NxApexRenderable
{
protected:
	virtual ~NxWindFSActor() {}

public:
	/**
	\brief Returns the asset the instance has been created from.
	*/
	virtual NxBasicFSAsset* 	getWindFSAsset() const = 0;

	/**
	\brief Gets the current pose of the actor
	*/
	virtual physx::PxMat44		getCurrentPose() const = 0;
	/**
	\brief Sets the current pose of the actor
	*/
	virtual void				setCurrentPose(const physx::PxMat44& pose) = 0;
	/**
	\brief Gets the current position of the actor
	*/
	virtual physx::PxVec3		getCurrentPosition() const = 0;
	/**
	\brief Sets the current position of the actor
	*/
	virtual void				setCurrentPosition(const physx::PxVec3& pos) = 0;

	/**
	\brief Sets the field strength
	*/
	virtual void				setFieldStrength(physx::PxF32) = 0;
	/**
	\brief Sets the field direction
	*/
	virtual void				setFieldDirection(const physx::PxVec3&) = 0;

	/**
	\brief Enable/Disable the field simulation
	*/
	virtual void				setEnabled(bool isEnabled) = 0;

	///Sets the uniform overall object scale
	virtual void				setCurrentScale(PxF32 scale) = 0;

	//Retrieves the uniform overall object scale
	virtual PxF32				getCurrentScale(void) const = 0;
};

PX_POP_PACK

}
} // end namespace physx::apex

#endif // NX_WIND_FSACTOR_H
