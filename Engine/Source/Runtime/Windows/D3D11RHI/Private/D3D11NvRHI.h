// Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.

#ifndef __D3D11NVRHI_H__
#define __D3D11NVRHI_H__

#include "GFSDK_NVRHI.h"
#include <map>

class FD3D11DynamicRHI;

namespace NVRHI
{
	class FRendererInterfaceD3D11 : public IRendererInterface, public IErrorCallback, public VXGI::IPerformanceMonitor
	{
	public:
		FRendererInterfaceD3D11(ID3D11Device* Device);

		void setTreatErrorsAsFatal(bool v);
		virtual void signalError(const char* file, int line, const char* errorDesc);

		virtual TextureHandle createTexture(const TextureDesc& d, const void* data);
		virtual TextureDesc describeTexture(TextureHandle t);
		virtual void clearTextureFloat(TextureHandle t, const Color& clearColor);
		virtual void clearTextureUInt(TextureHandle t, uint32_t clearColor);
		virtual void writeTexture(TextureHandle t, uint32_t subresource, const void* data, uint32_t rowPitch, uint32_t depthPitch);
		virtual void destroyTexture(TextureHandle t);

		virtual BufferHandle createBuffer(const BufferDesc& d, const void* data);
		virtual void writeBuffer(BufferHandle b, const void* data, size_t dataSize);
		virtual void clearBufferUInt(BufferHandle b, uint32_t clearValue);
		virtual void copyToBuffer(BufferHandle dest, uint32_t destOffsetBytes, BufferHandle src, uint32_t srcOffsetBytes, size_t dataSizeBytes);
		virtual void destroyBuffer(BufferHandle b);

		virtual void readBuffer(BufferHandle b, void* data, size_t* dataSize);

		virtual ConstantBufferHandle createConstantBuffer(const ConstantBufferDesc& d, const void* data);
		virtual void writeConstantBuffer(ConstantBufferHandle b, const void* data, size_t dataSize);
		virtual void destroyConstantBuffer(ConstantBufferHandle b);

		virtual ShaderHandle createShader(const ShaderDesc& d, const void* binary, const size_t binarySize);
		virtual ShaderHandle createShaderFromAPIInterface(ShaderType::Enum shaderType, const void* apiInterface);
		virtual void destroyShader(ShaderHandle s);

		virtual SamplerHandle createSampler(const SamplerDesc& d);
		virtual void destroySampler(SamplerHandle s);

		virtual InputLayoutHandle createInputLayout(const VertexAttributeDesc* d, uint32_t attributeCount, const void* vertexShaderBinary, const size_t binarySize);
		virtual void destroyInputLayout(InputLayoutHandle i);

		virtual PerformanceQueryHandle createPerformanceQuery(const char* name);
		virtual void destroyPerformanceQuery(PerformanceQueryHandle query);
		virtual void beginPerformanceQuery(PerformanceQueryHandle query, bool onlyAnnotation);
		virtual void endPerformanceQuery(PerformanceQueryHandle query);
		virtual float getPerformanceQueryTimeMS(PerformanceQueryHandle query);

		virtual GraphicsAPI::Enum getGraphicsAPI();

		virtual void* getAPISpecificInterface(APISpecificInterface::Enum interfaceType);
		virtual bool isOpenGLExtensionSupported(const char* name);
		virtual void* getOpenGLProcAddress(const char* procname);

		virtual void draw(const DrawCallState& state, const DrawArguments* args, uint32_t numDrawCalls);
		virtual void drawIndexed(const DrawCallState& state, const DrawArguments* args, uint32_t numDrawCalls);
		virtual void drawIndirect(const DrawCallState& state, BufferHandle indirectParams, uint32_t offsetBytes);
		virtual void dispatch(const DispatchState& state, uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ);
		virtual void dispatchIndirect(const DispatchState& state, BufferHandle indirectParams, uint32_t offsetBytes);

		virtual void executeRenderThreadCommand(IRenderThreadCommand* onCommand);
		virtual uint32_t getAFRGroupOfCurrentFrameForSLI(uint32_t numAFRGroups);

		virtual void setEnableUavBarriersForTexture(TextureHandle texture, bool enableBarriers);
		virtual void setEnableUavBarriersForBuffer(BufferHandle buffer, bool enableBarriers);

		TextureHandle getTextureFromRHI(FRHITexture* texture);
		FRHITexture* getRHITexture(TextureHandle texture);
		void forgetAboutTexture(FRHITexture* texture);

		template<typename ShaderType>
		void applyShaderState(PipelineStageBindings bindings);

		void applyState(DrawCallState state, bool applyShaders);
		void applyState(DispatchState state);
		void clearUAVs(DispatchState state);

		void setPixelShaderResourceAttributes(NVRHI::ShaderHandle PixelShader, const TArray<uint8>& ShaderResourceTable, bool bUsesGlobalCB);
		void setRHICommandList(FRHICommandList& RHICmdList);

		virtual void beginSection(const char* pSectionName);
		virtual void endSection();
	private:
		bool m_TreatErrorsAsFatal;
		ID3D11Device* m_Device;
		FRHICommandList* m_RHICmdList;
		std::map<FRHITexture*, TextureHandle> m_UnmanagedTextures;
		std::map<uint32, FRasterizerStateRHIRef> m_RasterizerStates;
		std::map<uint32, FDepthStencilStateRHIRef> m_DepthStencilStates;
		std::map<uint32, FBlendStateRHIRef> m_BlendStates;

		FRHIShaderResourceView* getTextureSRV(TextureHandle t, uint32 mipLevel, Format::Enum format);
		FRHIUnorderedAccessView* getTextureUAV(TextureHandle t, uint32 mipLevel, Format::Enum format);
		FRHIShaderResourceView* getBufferSRV(BufferHandle b, Format::Enum format);
		FRHIUnorderedAccessView* getBufferUAV(BufferHandle b, Format::Enum format);
		FRasterizerStateRHIParamRef getRasterizerState(const RasterState& rasterState);
		FDepthStencilStateRHIParamRef getDepthStencilState(const DepthStencilState& depthStencilState, bool depthTargetPresent);
		FBlendStateRHIParamRef getBlendState(const BlendState& blendState);
	};
}

#endif