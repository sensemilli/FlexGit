/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#ifndef NX_MODULE_TURBULENCE_FS_H
#define NX_MODULE_TURBULENCE_FS_H

#include "NxApex.h"
#include "NxApexShape.h"
#include "NxVelocitySourceActor.h"
#include "NxHeatSourceActor.h"
#include "NxSubstanceSourceActor.h"
#include "NxFlameEmitterActor.h"
#include "NxTestBase.h"

namespace physx
{
namespace apex
{

class NxTurbulenceFSAsset;
class NxTurbulenceFSAssetAuthoring;


/**
\brief Class for TurbulenceFS module.
*/
class NxModuleTurbulenceFS : public NxModule
{
public:

	/// Set custom timestep parameters for the specified scene (only for one simulation call)
	virtual bool                        setCustomTimestep(const NxApexScene& apexScene, float timestep, int numIterations) = 0;

	/// Enable/disable multi-solve feature for the specified scene (disable by default)
	virtual bool                        setMultiSolveEnabled(const NxApexScene& apexScene, bool enabled) = 0;

	/// Enable output velocity field to NxUserRenderSurfaceBuffer
	virtual void						enableOutputVelocityField() = 0;

	/// Enable output density field to NxUserRenderSurfaceBuffer
	virtual void						enableOutputDensityField() = 0;

	/// Enable output flame field to NxUserRenderSurfaceBuffer
	virtual void						enableOutputFlameField() = 0;

#if NX_SDK_VERSION_MAJOR == 3
	/// Set custum filter shader to filter collision shapes interacting with Turbulence grids
	virtual void						setCustomFilterShader(physx::PxSimulationFilterShader shader, const void* shaderData, physx::PxU32 shaderDataSize) = 0;
#endif

	virtual const NxTestBase*						getTestBase(NxApexScene* apexScene) const = 0;

protected:
	virtual ~NxModuleTurbulenceFS() {}
};

#if !defined(_USRDLL)
/* If this module is distributed as a static library, the user must call this
 * function before calling NxApexSDK::createModule("Turbulence")
 */
void instantiateModuleTurbulenceFS();
#endif

}
} // end namespace physx::apex

#endif // NX_MODULE_TURBULENCE_FS_H
