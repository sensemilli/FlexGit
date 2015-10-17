/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#ifndef NX_VORTEX_FSACTOR_H
#define NX_VORTEX_FSACTOR_H

#include "NxApex.h"

namespace physx
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class NxBasicFSAsset;


/**
 \brief VortexFS Actor class
 */
class NxVortexFSActor : public NxApexActor, public NxApexRenderable
{
protected:
	virtual ~NxVortexFSActor() {}

public:
	/**
	\brief Returns the asset the instance has been created from.
	*/
	virtual NxBasicFSAsset* 	getVortexFSAsset() const = 0;

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
	\brief Sets the axis of the capsule in local coordinate system
	*/
	virtual void				setAxis(const physx::PxVec3&) = 0;
	/**
	\brief Sets the height of the capsule
	*/
	virtual void				setHeight(physx::PxF32) = 0;
	/**
	\brief Sets the bottom radius of the capsule
	*/
	virtual void				setBottomRadius(physx::PxF32) = 0;
	/**
	\brief Sets the top radius of the capsule
	*/
	virtual void				setTopRadius(physx::PxF32) = 0;

	/**
	\brief Sets the bottom spherical force of the capsule
	*/
	virtual void				setBottomSphericalForce(bool) = 0;
	/**
	\brief Sets the top spherical force of the capsule
	*/
	virtual void				setTopSphericalForce(bool) = 0;

	/**
	\brief Sets strength of the rotational part of vortex field
	*/
	virtual void				setRotationalStrength(physx::PxF32) = 0;
	/**
	\brief Sets strength of the radial part of vortex field
	*/
	virtual void				setRadialStrength(physx::PxF32) = 0;
	/**
	\brief Sets strength of the lifting part of vortex field
	*/
	virtual void				setLiftStrength(physx::PxF32) = 0;

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

#endif // NX_VORTEX_FSACTOR_H
