// Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.

#include "D3D12RHIPrivate.h"

// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI


VXGI::IGlobalIllumination* FD3D12DynamicRHI::RHIVXGIGetInterface()
{
	return VxgiInterface;
}

void FD3D12DynamicRHI::CreateVxgiInterface()
{
	check(!VxgiRendererD3D12);
	VxgiRendererD3D12 = new NVRHI::FRendererInterfaceD3D12(GetRHIDevice());
	check(VxgiRendererD3D12);

	VXGI::GIParameters Params;
	Params.rendererInterface = VxgiRendererD3D12;
	Params.errorCallback = VxgiRendererD3D12;

	check(!VxgiInterface);
	auto Status = VXGI::VFX_VXGI_CreateGIObject(Params, &VxgiInterface);
	check(VXGI_SUCCEEDED(Status));

	VXGI::Version VxgiVersion;
	UE_LOG(LogD3D12RHI, Log, TEXT("VXGI: Version %u.%u.%u.%u"), VxgiVersion.Major, VxgiVersion.Minor, VxgiVersion.Branch, VxgiVersion.Revision);

	bVxgiVoxelizationParametersSet = false;
}

void FD3D12DynamicRHI::ReleaseVxgiInterface()
{
	if (VxgiInterface)
	{
		VXGI::VFX_VXGI_DestroyGIObject(VxgiInterface);
		VxgiInterface = NULL;
	}

	if (VxgiRendererD3D12)
	{
		delete VxgiRendererD3D12;
		VxgiRendererD3D12 = NULL;
	}

	bVxgiVoxelizationParametersSet = false;
}

void FD3D12DynamicRHI::RHIVXGISetVoxelizationParameters(const VXGI::VoxelizationParameters& Parameters)
{
	// If the cvars define a new set of parameters, see if it's valid and try to set them
	if(!bVxgiVoxelizationParametersSet || Parameters != VxgiVoxelizationParameters)
	{
		VxgiRendererD3D12->setTreatErrorsAsFatal(false);
		auto Status = VxgiInterface->validateVoxelizationParameters(Parameters);
		VxgiRendererD3D12->setTreatErrorsAsFatal(true);

		if(VXGI_SUCCEEDED(Status))
		{
			// If the call to setVoxelizationParameters fails, VXGI will be in an unititialized state, so set bVxgiVoxelizationParametersSet to false
			bVxgiVoxelizationParametersSet = VXGI_SUCCEEDED(VxgiInterface->setVoxelizationParameters(Parameters));
		}
	}

	// If the new parameters are invalid, set the default parameters - they should always work
	if(!bVxgiVoxelizationParametersSet)
	{
		VXGI::VoxelizationParameters DefaultVParams;

		auto Status = VxgiInterface->setVoxelizationParameters(DefaultVParams);
		check(VXGI_SUCCEEDED(Status));
		bVxgiVoxelizationParametersSet = true;
	}

	// Regardless of whether the new parameters are valid, store them to avoid re-initializing VXGI on the next frame
	VxgiVoxelizationParameters = Parameters;
}

void FD3D12DynamicRHI::RHIVXGISetPixelShaderResourceAttributes(NVRHI::ShaderHandle PixelShader, const TArray<uint8>& ShaderResourceTable, bool bUsesGlobalCB)
{
	VxgiRendererD3D12->setPixelShaderResourceAttributes(PixelShader, ShaderResourceTable, bUsesGlobalCB);
}

void FD3D12DynamicRHI::RHIVXGIApplyDrawStateButNotShaders(const NVRHI::DrawCallState& DrawCallState)
{
	VxgiRendererD3D12->applyState(DrawCallState, false);
}

void FD3D12DynamicRHI::RHIVXGISetCommandList(FRHICommandList& RHICommandList) 
{ 
	VxgiRendererD3D12->setRHICommandList(RHICommandList);
}

void FD3D12CommandContext::RHIVXGICleanupAfterVoxelization()
{
	FlushCommands(false);
}

FRHITexture* FD3D12DynamicRHI::GetRHITextureFromVXGI(NVRHI::TextureHandle texture)
{
	return VxgiRendererD3D12->getRHITexture(texture);
}

NVRHI::TextureHandle FD3D12DynamicRHI::GetVXGITextureFromRHI(FRHITexture* texture)
{
	return VxgiRendererD3D12->getTextureFromRHI(texture);
}

void FD3D12CommandContext::RHISetViewportsAndScissorRects(uint32 Count, const FViewportBounds* Viewports, const FScissorRect* ScissorRects)
{
	StateCache.SetViewports(Count, (D3D12_VIEWPORT*)Viewports);
	StateCache.SetScissorRects(Count, (D3D12_RECT*)ScissorRects);
}

void FD3D12CommandContext::RHIDispatchIndirectComputeShaderStructured(FStructuredBufferRHIParamRef ArgumentBufferRHI, uint32 ArgumentOffset)
{
	// Can't use m_RHICmdList->DispatchIndirectComputeShader(...) method here because:
	// - It requires the argument buffer to be a VertexBuffer, and it's a StructuredBuffer so far as UE is concerned;
	// - It multiplies the offset by 20, and VXGI uses different offsets.
	// So below is a modified version of FD3D12CommandContext::RHIDispatchIndirectComputeShader method.

	FComputeShaderRHIParamRef ComputeShaderRHI = GetCurrentComputeShader();
	FD3D12ComputeShader* ComputeShader = FD3D12DynamicRHI::ResourceCast(ComputeShaderRHI);
	FD3D12StructuredBuffer* ArgumentBuffer = FD3D12DynamicRHI::ResourceCast(ArgumentBufferRHI);

	OwningRHI.RegisterGPUWork(1);

	StateCache.SetComputeShader(ComputeShader);

	if (ComputeShader->bShaderNeedsGlobalConstantBuffer)
	{
		CommitComputeShaderConstants();
	}
	CommitComputeResourceTables(ComputeShader);
	StateCache.ApplyState(true);

	FD3D12DynamicRHI::TransitionResource(CommandListHandle, ArgumentBuffer->ResourceLocation->GetResource(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_BARRIER_TYPE_TRANSITION);

	numDispatches++;
	CommandListHandle->ExecuteIndirect(
		GetParentDevice()->GetDispatchIndirectCommandSignature(),
		1,
		ArgumentBuffer->ResourceLocation->GetResource()->GetResource(),
		ArgumentBuffer->ResourceLocation->GetOffset() + ArgumentOffset,
		NULL,
		0
		);

	StateCache.FlushComputeShaderCache();

	DEBUG_EXECUTE_COMMAND_LIST(this);

	StateCache.SetComputeShader(nullptr);
}

void FD3D12CommandContext::RHICopyStructuredBufferData(FStructuredBufferRHIParamRef DestBufferRHI, uint32 DestOffset, FStructuredBufferRHIParamRef SrcBufferRHI, uint32 SrcOffset, uint32 DataSize) 
{
	auto DestBuffer = FD3D12DynamicRHI::ResourceCast(DestBufferRHI);
	auto SrcBuffer = FD3D12DynamicRHI::ResourceCast(SrcBufferRHI);

	FD3D12DynamicRHI::TransitionResource(CommandListHandle, DestBuffer->Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_BARRIER_TYPE_TRANSITION);

	if(SrcBuffer->Resource->RequiresResourceStateTracking())
		FD3D12DynamicRHI::TransitionResource(CommandListHandle, SrcBuffer->Resource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_BARRIER_TYPE_TRANSITION);
	
	numCopies++;
	CommandListHandle->CopyBufferRegion(
		DestBuffer->Resource->GetResource(), DestOffset + DestBuffer->ResourceLocation->GetOffset(), 
		SrcBuffer->Resource->GetResource(), SrcOffset + SrcBuffer->ResourceLocation->GetOffset(), 
		DataSize);

	DEBUG_EXECUTE_COMMAND_LIST(this);
}


#endif
// NVCHANGE_END: Add VXGI
