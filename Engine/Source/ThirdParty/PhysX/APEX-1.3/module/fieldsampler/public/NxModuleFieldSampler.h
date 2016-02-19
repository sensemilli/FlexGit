/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#ifndef NX_MODULE_FIELD_SAMPLER_H
#define NX_MODULE_FIELD_SAMPLER_H

#include "NxApex.h"

namespace physx
{
namespace apex
{

PX_PUSH_PACK_DEFAULT


#if NX_SDK_VERSION_MAJOR == 2
/**
\brief Class for storing 64-bit groups filtering parameters 
	   used for collision filtering between IOSes and Field Samplers.
	   (similar to 128-bit groups filtering system in the 284 PhysX SDK)
*/
class NxGroupsFilteringParams64
{
public:

	/**
	\brief Constructor that configures it such that each of the 64 bits in NxGroupsMasks64 
		   represents a different collision group, and that if two objects interact 
		   (collide) only if there exists at least one group of which they are both members.
		   (Initially DISABLED)
	*/
	PX_INLINE NxGroupsFilteringParams64()
	{
		const0 = const1 = NxGroupsMask64(0, 0);
		op0 = op1 = NxGroupsFilterOp::OR;
		op2 = NxGroupsFilterOp::AND;
		flag = false;
	}

	NxGroupsMask64			const0, const1;
	NxGroupsFilterOp::Enum	op0, op1, op2;
	bool					flag;
};
#endif

#if NX_SDK_VERSION_MAJOR == 3

class NxFieldSamplerWeightedCollisionFilterCallback
{
public:
	/**
	\brief This is an optional callback interface for collision filtering between field samplers and other objects
		   If this interface is not provided, then collision filtering happens normally through the default scene simulationfiltershader callback
		   However, if this is provided, then the user can no only return true/false to indicate whether or not the two objects should interact but
		   can also assign a weighted multiplier value to control how strongly the two objects should interact.
	*/
	virtual bool fieldSamplerWeightedCollisionFilter(const physx::PxFilterData &objectA,const physx::PxFilterData &objectB,float &multiplierValue) = 0;
};

#endif

/**
 \brief FieldSampler module class
 */
class NxModuleFieldSampler : public NxModule
{
protected:
	virtual					~NxModuleFieldSampler() {}

public:

#if NX_SDK_VERSION_MAJOR == 2
	/**
	\brief Set boundary groups filtering parameters for the specified scene
	*/	
	virtual bool			setFieldBoundaryGroupsFilteringParams(const NxApexScene& apexScene ,
	        const NxGroupsFilteringParams64& params) = 0;

	/**
	\brief Get boundary groups filtering parameters for the specified scene
	*/
	virtual bool			getFieldBoundaryGroupsFilteringParams(const NxApexScene& apexScene ,
	        NxGroupsFilteringParams64& params) const = 0;

	/**
	\brief Set field sampler groups filtering parameters for the specified scene
	*/
	virtual bool			setFieldSamplerGroupsFilteringParams(const NxApexScene& apexScene ,
	        const NxGroupsFilteringParams64& params) = 0;

	/**
	\brief Get field sampler groups filtering parameters for the specified scene
	*/
	virtual bool			getFieldSamplerGroupsFilteringParams(const NxApexScene& apexScene ,
	        NxGroupsFilteringParams64& params) const = 0;
#endif

#if NX_SDK_VERSION_MAJOR == 3
	/**
	\brief Sets the optional weighted collision filter callback for this scene. If not provided, it will use the default SimulationFilterShader on the current scene
	*/
	virtual bool			setFieldSamplerWeightedCollisionFilterCallback(const NxApexScene& apexScene,NxFieldSamplerWeightedCollisionFilterCallback *callback) = 0;
#endif

#if NX_SDK_VERSION_MAJOR == 3
	/**
		Set flag to toggle PhysXMonitor for ForceFields.
	*/
	virtual void			enablePhysXMonitor(const NxApexScene& apexScene, bool enable) = 0;
	/**
	\brief Add filter data (collision group) to PhysXMonitor. 
		
	\param apexScene [in] - Apex scene for which to submit the force sample batch.
	\param filterData [in] - PhysX 3.0 collision mask for PhysXMonitor
	*/
	virtual void			setPhysXMonitorFilterData(const NxApexScene& apexScene, physx::PxFilterData filterData) = 0;

	/**
	\brief Initialize a query for a batch of sampling points

	\param apexScene [in] - Apex scene for which to create the force sample batch query.
	\param maxCount [in] - Maximum number of indices (active samples)
	\param filterData [in] - PhysX 3.0 collision mask for data
	\return the ID of created query
	*/
	virtual PxU32			createForceSampleBatch(const NxApexScene& apexScene, physx::PxU32 maxCount, const physx::PxFilterData filterData) = 0;

	/**
	\brief Release a query for a batch of sampling points

	\param apexScene [in] - Apex scene for which to create the force sample batch.
	\param batchId [in] - ID of query that should be released
	*/
	virtual void			releaseForceSampleBatch(const NxApexScene& apexScene, physx::PxU32 batchId) = 0;

	/**
	\brief Submits a batch of sampling points to be evaluated during the simulation step.
	
	\param apexScene [in] - Apex scene for which to submit the force sample batch.
	\param batchId [in] - ID of query for force sample batch.
	\param forces [out] - Buffer to which computed forces are written to. The buffer needs to be persistent between calling this function and the next PxApexScene::fetchResults.
	\param forcesStride [in] - Stride between consecutive force vectors within the forces output.
	\param positions [in] - Buffer containing the positions of the input samples.
	\param positionsStride [in] - Stride between consecutive position vectors within the positions input.
	\param velocities [in] - Buffer containing the velocities of the input samples.
	\param velocitiesStride [in] - Stride between consecutive velocity vectors within the velocities input.
	\param indices [in] - Buffer containing the indices of the active samples that are considered for the input and output buffers.
	\param numIndices [in] - Number of indices (active samples). 
	*/
	virtual void			submitForceSampleBatch(	const NxApexScene& apexScene, physx::PxU32 batchId,
													PxVec4* forces, const PxU32 forcesStride, 
													const PxVec3* positions, const PxU32 positionsStride, 
													const PxVec3* velocities, const PxU32 velocitiesStride,
													const PxF32* mass, const PxU32 massStride,
													const PxU32* indices, const PxU32 numIndices) = 0;

#endif
};



PX_POP_PACK

}
} // end namespace physx::apex

#endif // NX_MODULE_FIELD_SAMPLER_H
