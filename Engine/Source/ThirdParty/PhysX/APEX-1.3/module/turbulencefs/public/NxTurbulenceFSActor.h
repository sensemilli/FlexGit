/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NX_TURBULENCE_FSACTOR_H
#define NX_TURBULENCE_FSACTOR_H

#include "NxApex.h"
#include "NxApexShape.h"

namespace physx
{
namespace apex
{

class NxTurbulenceFSAsset;


/**
 \brief Turbulence FieldSampler Actor class
 */
class NxTurbulenceFSActor : public NxApexActor, public NxApexRenderable
{
protected:
	virtual ~NxTurbulenceFSActor() {}

public:

	/**
	 \brief Returns the asset the instance has been created from.
	 */
	virtual NxTurbulenceFSAsset* getTurbulenceFSAsset() const = 0;

	///enable/disable the fluid simulation
	virtual void setEnabled(bool enable) = 0;

	/**
	set the pose of the grid - this includes only the position and rotation.<BR>
	the position is that of the center of the grid, as is determined as (pose.column3.x, pose.column3.y, pose.column3.z)<BR>
	the rotation is the rotation of the object that the grid is centered on <BR>
	(the grid does not rotate, but we use this pose for rotating the collision obstacle, the jets and imparting angular momentum)
	 */
	virtual void setPose(physx::PxMat44 pose) = 0;

	/**
	get the pose of the grid - this includes only the position and rotation.<BR>
	the position is that of the center of the grid, as is determined as (pose.column3.x, pose.column3.y, pose.column3.z)<BR>
	the rotation is the rotation of the object that the grid is centered on <BR>
	(the grid does not rotate, but we use this pose for rotating the collision obstacle, the jets and imparting angular momentum)
	 */
	virtual physx::PxMat44 getPose() const = 0;

	///get the grid bounding box min point
	virtual PxVec3 getGridBoundingBoxMin() = 0;
	///get the grid bounding box max point
	virtual PxVec3 getGridBoundingBoxMax() = 0;

	///get the grid size vector
	virtual PxVec3 getGridSize() = 0;

	///get the grid dimensions
	virtual void getGridDimensions(PxU32 &gridX,PxU32 &gridY,PxU32 &gridZ) = 0;

	///set the LOD weights to affect the internal calculation of LOD for the grid
	virtual void setLODWeights(PxReal maxDistance, PxReal distanceWeight, PxReal bias, PxReal benefitBias, PxReal benefitWeight) = 0;

	/**
	enable for more aggressive LOD<BR>
	if set to true then the actor can choose to use less than the available resources if it computes its benefit is not high enough to  need all available resources
	 */
	virtual void setOptimizedLODEnabled(bool enable) = 0;

	/**
	force the current LOD to a particular value, range is 0-1:<BR>
	1.0f is maximum simulation quality<BR>
	0.0f is minimum simulation quality
	 */
	virtual void setCustomLOD(PxReal LOD) = 0;
	///get the current value of the LOD, set either by the APEX LOD system, or by the user using setCustomLOD (if customLOD was enabled by using enableCustomLOD)
	virtual PxReal getCurrentLOD() const = 0;
	///use this method to switch between using custom LOD and APEX calculated LOD
	virtual void enableCustomLOD(bool enable) = 0;

	/**
	methods to get the velocity field sampled at grid centers.<BR>
	call setSampleVelocityFieldEnabled(true) to enable the sampling and call getVelocityField to get back the sampled results
	 */
	virtual void getVelocityField(void** x, void** y, void** z, PxU32& sizeX, PxU32& sizeY, PxU32& sizeZ) = 0;
	virtual void setSampleVelocityFieldEnabled(bool enabled) = 0;

	///set a multiplier and a clamp on the total angular velocity induced in the system by the internal collision obstacle or by external collision objects
	virtual void setAngularVelocityMultiplierAndClamp(PxReal angularVelocityMultiplier, PxReal angularVelocityClamp) = 0;

	///set a multiplier and a clamp on the total linear velocity induced in the system by a collision obstacle
	virtual void setLinearVelocityMultiplierAndClamp(PxReal linearVelocityMultiplier, PxReal linearVelocityClamp) = 0;

	///set velocity field fade. All cells in the field multiplies by (1 - fade) on each frame
	virtual void setVelocityFieldFade(PxReal fade) = 0;

	///set fluid viscosity (diffusion) for velocity
	virtual void setFluidViscosity(PxReal viscosity) = 0;

	///set time of velocity field cleaning process [sec]
	virtual void setVelocityFieldCleaningTime(PxReal time) = 0;

	///set time without activity before velocity field cleaning process starts [sec]. 
	virtual void setVelocityFieldCleaningDelay(PxReal time) = 0;

	/**
	set parameter which correspond to 'a' in erf(a*(cleaning_timer/velocityFieldCleaningTime)). 
	for full cleaning it should be greater then 2. If want just decrease velocity magitude use smaller value
	*/
	virtual void setVelocityFieldCleaningIntensity(PxReal a) = 0;

	/**
	enable whether or not to use heat in the simulation (enabling heat reduces performance).<BR>
	If you are enabling heat then you also need to add temperature sources (without temperature sources you will see no effect of heat on the simulation, except a drop in performance)
	 */
	virtual	void setUseHeat(bool enable) = 0;

	///set heat specific parameters for the simulation
	virtual void setHeatBasedParameters(PxReal forceMultiplier, PxReal ambientTemperature, physx::PxVec3 heatForceDirection, physx::PxReal thermalConductivity) = 0;

	/**
	enable whether or not to use density in the simulation (enabling density reduces performance).<BR>
	If you are enabling density then you also need to add substance sources (without substance sources you will see no effect of density on the simulation, except a drop in performance)
	 */
	virtual	void setUseDensity(bool enable) = 0;

	/**
	\brief Returns true if turbulence actor is in density mode.
	*/
	virtual bool getUseDensity(void) const = 0;

	///set density specific parameters for the simulation
	virtual void setDensityBasedParameters(PxReal diffusionCoef, PxReal densityFieldFade) = 0;

	///get the density grid dimensions
	virtual void getDensityGridDimensions(PxU32 &gridX,PxU32 &gridY,PxU32 &gridZ) = 0;

	/**
	allows external actors like wind or explosion to add a single directional velocity to the grid.<BR>
	Note that if multiple calls to this function are made only the last call is honored (i.e. the velocities are not accumulated)
	 */
	virtual	void setExternalVelocity(physx::PxVec3 vel) = 0;

	///set a multiplier for the field velocity
	virtual void setFieldVelocityMultiplier(PxReal value) = 0;

	///set a weight for the field velocity
	virtual void setFieldVelocityWeight(PxReal value) = 0;

	///set noise parameters
	virtual void setNoiseParameters(PxF32 noiseStrength, physx::PxVec3 noiseSpacePeriod, PxF32 noiseTimePeriod, PxU32 noiseOctaves) = 0;

	virtual void setDensityTextureRange(float minValue, float maxValue) = 0;

	/**
	\brief Returns the optional volume render material name specified for this turbulence actor.
	*/
	virtual const char *getVolumeRenderMaterialName(void) const = 0;

	///get the velocity field in the user defined surface
	virtual physx::apex::NxUserRenderSurfaceBuffer* getVelocityFieldRenderSurface() const = 0;

	///get the density field in the user defined surface
	virtual physx::apex::NxUserRenderSurfaceBuffer* getDensityFieldRenderSurface() const = 0;

	///get the flame field in the user defined surface
	virtual physx::apex::NxUserRenderSurfaceBuffer* getFlameFieldRenderSurface() const = 0;

	///Sets the uniform overall object scale
	virtual void				setCurrentScale(PxF32 scale) = 0;

	//Retrieves the uniform overall object scale
	virtual PxF32				getCurrentScale(void) const = 0;

	/**
	\brief Returns true if turbulence actor is in flame mode.
	*/
	virtual bool getUseFlame(void) const = 0;

	/**
	\brief Returns flame grid dimensions
	*/
	virtual void getFlameGridDimensions(PxU32 &gridX, PxU32 &gridY, PxU32 &gridZ) const = 0;

};

}
} // end namespace physx::apex

#endif // NX_TURBULENCE_FSACTOR_H
