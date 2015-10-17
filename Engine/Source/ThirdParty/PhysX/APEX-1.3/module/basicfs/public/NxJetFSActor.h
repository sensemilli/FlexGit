/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#ifndef NX_JET_FSACTOR_H
#define NX_JET_FSACTOR_H

#include "NxApex.h"

namespace physx
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class NxBasicFSAsset;


/**
 \brief JetFS Actor class
 */
class NxJetFSActor : public NxApexActor, public NxApexRenderable
{
protected:
	virtual ~NxJetFSActor() {}

public:
	/**
	\brief Returns the asset the instance has been created from.
	*/
	virtual NxBasicFSAsset* 	getJetFSAsset() const = 0;


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
	\brief Gets the current scale of the actor
	*/
	virtual physx::PxF32		getCurrentScale() const = 0;
	/**
	\brief Sets the current scale of the actor
	*/
	virtual void				setCurrentScale(const physx::PxF32& scale) = 0;

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

};

PX_POP_PACK

}
} // end namespace physx::apex

#endif // NX_Jet_FSACTOR_H
