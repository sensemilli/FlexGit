// NVCHANGE_BEGIN: JCAO - PhysX Render Resource

/*=============================================================================
	PhysXRender.cpp: PhysX Render Resource
=============================================================================*/

#include "EnginePrivate.h"


#if WITH_PHYSX

#include "PhysXRender.h"

#if WITH_CUDA_INTEROP
//#include "AllowWindowsPlatformTypes.h"
//	#include <D3D11.h>
//#include "HideWindowsPlatformTypes.h"
//#include "../../../Windows/D3D11RHI/Public/D3D11State.h"
//#include "../../../Windows/D3D11RHI/Public/D3D11Resources.h"
#endif

FApexRenderResourceManager			GApexRenderResourceManager;


#if WITH_APEX

// Render Surface for the velocity field
NxUserRenderSurfaceBuffer*   FApexRenderResourceManager::createSurfaceBuffer(const NxUserRenderSurfaceBufferDesc& Desc)
{
	check(IsInRenderingThread());
	FApexRenderSurfaceBuffer *buffer = new FApexRenderSurfaceBuffer(Desc);
	return static_cast< NxUserRenderSurfaceBuffer* >(buffer);
}

void FApexRenderResourceManager::releaseSurfaceBuffer(NxUserRenderSurfaceBuffer& InBuffer)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER
	(
		ReleaseSurfaceBuffer,
		NxUserRenderSurfaceBuffer*, Buffer, &InBuffer,
		{
			delete static_cast<FApexRenderSurfaceBuffer*>(Buffer);
		}
	);
}


FApexRenderSurfaceBuffer::FApexRenderSurfaceBuffer(const NxUserRenderSurfaceBufferDesc &desc)
	: SizeX(desc.width)
	, SizeY(desc.height)
	, SizeZ(desc.depth)
	, bRegisteredInCUDA(desc.registerInCUDA)
#if WITH_CUDA_INTEROP
	, InteropContext(0)
	, CudaGraphicsResource(0)
#endif
{
	switch (desc.format)
	{
	case NxRenderDataFormat::FLOAT4:
		Format = PF_A32B32G32R32F;
		break;
	case NxRenderDataFormat::UBYTE1:
		Format = PF_G8;
		break;
	case NxRenderDataFormat::HALF4:
		Format = PF_FloatRGBA;
		break;
	default:
		Format = PF_A32B32G32R32F;
	}

#if WITH_CUDA_INTEROP
	if (bRegisteredInCUDA)
	{
		InteropContext = desc.interopContext;
	}
#endif

	InitResource();
}

FApexRenderSurfaceBuffer::~FApexRenderSurfaceBuffer()
{
	ReleaseResource();

#if WITH_CUDA_INTEROP
	InteropContext = NULL;
#endif
}

void FApexRenderSurfaceBuffer::InitRHI()
{
	if (GMaxRHIFeatureLevel >= ERHIFeatureLevel::SM4)
	{
		FRHIResourceCreateInfo CreateInfo;
		VolumeTextureRHI = RHICreateTexture3D(
			SizeX, SizeY, SizeZ, Format,
					/*NumMips=*/ 1,
					/*Flags=*/ TexCreate_ShaderResource,
					CreateInfo );

#if WITH_CUDA_INTEROP
		if (bRegisteredInCUDA && InteropContext && InteropContext->getInteropMode() == physx::PxCudaInteropMode::D3D11_INTEROP)
		{
			bRegisteredInCUDA  = InteropContext->registerResourceInCudaD3D( CudaGraphicsResource, VolumeTextureRHI->GetNativeResource() );
		}
#endif
	}
	else
	{
		bRegisteredInCUDA = false;
	}
}

void FApexRenderSurfaceBuffer::ReleaseRHI()
{
#if WITH_CUDA_INTEROP
	if (bRegisteredInCUDA && InteropContext)
	{
		if (CudaGraphicsResource != 0)
		{
			InteropContext->unregisterResourceInCuda(CudaGraphicsResource);
			CudaGraphicsResource = 0;
		}

		bRegisteredInCUDA = false;
	}
#endif

	VolumeTextureRHI.SafeRelease();
}

void FApexRenderSurfaceBuffer::writeBuffer(const void* srcData, physx::PxU32 srcPitch, physx::PxU32 srcHeight, physx::PxU32 dstX, physx::PxU32 dstY, physx::PxU32 dstZ, physx::PxU32 width, physx::PxU32 height, physx::PxU32 depth)
{
	const int32 FormatSize = GPixelFormats[Format].BlockBytes;

	FUpdateTextureRegion3D Region(dstX, dstY, dstZ, 0, 0, 0, width, height, depth);

	RHIUpdateTexture3D(
		VolumeTextureRHI,
		0,
		Region,
		width * FormatSize,
		width * height * FormatSize,
		(const uint8*)srcData);
}


#endif	// WITH_APEX


#endif // WITH_PHYSX

// NVCHANGE_END: JCAO - PhysX Render Resource
