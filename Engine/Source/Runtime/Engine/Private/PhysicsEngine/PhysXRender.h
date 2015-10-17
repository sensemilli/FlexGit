// NVCHANGE_BEGIN: JCAO - PhysX Render Resource

/*=============================================================================
	PhysXRender.h: PhysX render resource
=============================================================================*/

#pragma once

#if WITH_PHYSX

#include "PhysXIncludes.h"

#if WITH_APEX

/**
	The velocity field from Turbulence require the surface buffer
*/
class FApexRenderSurfaceBuffer: public NxUserRenderSurfaceBuffer, public FRenderResource
{
public:
	FTexture3DRHIRef VolumeTextureRHI;
	
	int32 SizeX;
	
	int32 SizeY;
	
	int32 SizeZ;

	bool bRegisteredInCUDA;

	uint8 Format;

	FApexRenderSurfaceBuffer(const NxUserRenderSurfaceBufferDesc &desc);
	virtual		~FApexRenderSurfaceBuffer();

	virtual void InitRHI() override;
	virtual void ReleaseRHI() override;

	virtual void writeBuffer(const void* srcData, physx::PxU32 srcPitch, physx::PxU32 srcHeight, physx::PxU32 dstX, physx::PxU32 dstY, physx::PxU32 dstZ, physx::PxU32 width, physx::PxU32 height, physx::PxU32 depth) override;

#if WITH_CUDA_INTEROP
	virtual bool getInteropResourceHandle(CUgraphicsResource& handle) override
	{
		handle = CudaGraphicsResource;
		return bRegisteredInCUDA;
	}
#endif

private:

#if WITH_CUDA_INTEROP
	physx::PxCudaContextManager*	InteropContext;
	CUgraphicsResource				CudaGraphicsResource;
#endif //WITH_CUDA_INTEROP
};

class FApexRenderResourceManager : public NxUserRenderResourceManager
{
public:
	// NxUserRenderResourceManager interface.

	virtual NxUserRenderVertexBuffer*	createVertexBuffer(const NxUserRenderVertexBufferDesc&) override
	{
		return NULL;
	}
	virtual NxUserRenderIndexBuffer*	createIndexBuffer(const NxUserRenderIndexBufferDesc&) override
	{
		return NULL;
	}
	virtual NxUserRenderBoneBuffer*		createBoneBuffer(const NxUserRenderBoneBufferDesc&) override
	{
		return NULL;
	}
	virtual NxUserRenderInstanceBuffer*	createInstanceBuffer(const NxUserRenderInstanceBufferDesc&) override
	{
		return NULL;
	}
	virtual NxUserRenderSpriteBuffer*   createSpriteBuffer(const NxUserRenderSpriteBufferDesc&) override
	{
		return NULL;
	}
	virtual NxUserRenderSurfaceBuffer*   createSurfaceBuffer(const NxUserRenderSurfaceBufferDesc&) override;

	virtual NxUserRenderResource*		createResource(const NxUserRenderResourceDesc&) override
	{
		return NULL;
	}
	virtual void						releaseVertexBuffer(NxUserRenderVertexBuffer&) override {}
	virtual void						releaseIndexBuffer(NxUserRenderIndexBuffer&) override {}
	virtual void						releaseBoneBuffer(NxUserRenderBoneBuffer&) override {}
	virtual void						releaseInstanceBuffer(NxUserRenderInstanceBuffer&) override {}
	virtual void						releaseSpriteBuffer(NxUserRenderSpriteBuffer&) override {}
	virtual void						releaseSurfaceBuffer(NxUserRenderSurfaceBuffer&) override;
	virtual void						releaseResource(NxUserRenderResource&) override {}
	virtual physx::PxU32				getMaxBonesForMaterial(void*) override
	{
		return 0;
	}
	virtual bool						getSpriteLayoutData(physx::PxU32 , physx::PxU32 , NxUserRenderSpriteBufferDesc* ) override
	{
		return false;
	}
	virtual bool						getInstanceLayoutData(physx::PxU32 , physx::PxU32 , NxUserRenderInstanceBufferDesc* ) override
	{
		return false;
	}

};
extern FApexRenderResourceManager GApexRenderResourceManager;

#endif // WITH_APEX

#endif // WITH_PHYSX

// NVCHANGE_END: JCAO - PhysX Render Resource