#include "EnginePrivate.h"

//#include "../../Windows/D3D11RHI/Private/D3D11RHIPrivate.h"	// Hack
#include "RHIStaticStates.h"
#include "ShaderParameterUtils.h"
#include "GlobalShader.h"
#include "SkeletalRenderGPUSkin.h"
#include "SimpleElementShaders.h"
#include "Hair.h"
#include "HairSceneProxy.h"
#include "FogRendering.h"

#include "../../Renderer/Private/LightGrid.h"

DEFINE_LOG_CATEGORY(LogHairWorks);

// Debug render console variables.
#define HairVisualizerCVarDefine(name)	\
	static TAutoConsoleVariable<int> CVarHairVisualize##name(TEXT("r.Hair.Visualize")TEXT(#name),	0, TEXT(""), ECVF_RenderThreadSafe)

HairVisualizerCVarDefine(GuideHairs);
HairVisualizerCVarDefine(SkinnedGuideHairs);
HairVisualizerCVarDefine(HairInteractions);
HairVisualizerCVarDefine(ControlVertices);
HairVisualizerCVarDefine(Frames);
HairVisualizerCVarDefine(LocalPos);
HairVisualizerCVarDefine(ShadingNormals);
HairVisualizerCVarDefine(GrowthMesh);
HairVisualizerCVarDefine(Bones);
HairVisualizerCVarDefine(Capsules);
HairVisualizerCVarDefine(BoundingBox);
HairVisualizerCVarDefine(PinConstraints);
HairVisualizerCVarDefine(ShadingNormalBone);

// Configuration console variables.
static TAutoConsoleVariable<int> CVarHairMsaaLevel(TEXT("r.Hair.MsaaLevel"), 8, TEXT(""), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int> CVarHairTemporalAa(TEXT("r.Hair.TemporalAa"), 0, TEXT(""), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarHairShadowBiasScale(TEXT("r.Hair.Shadow.BiasScale"), 0.1, TEXT(""), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarHairShadowTransitionScale(TEXT("r.Hair.Shadow.TransitionScale"), 0.1, TEXT(""), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarHairShadowWidthScale(TEXT("r.Hair.Shadow.WidthScale"), 1, TEXT(""), ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarHairShadowTexelsScale(TEXT("r.Hair.Shadow.TexelsScale"), 5, TEXT(""), ECVF_RenderThreadSafe);

#ifndef _CPP
#define _CPP
#endif
#include "../../../../Shaders/GFSDK_HairWorks_ShaderCommon.usf"

class CustomLogHandler : public GFSDK_HAIR_LogHandler {
public:
	~CustomLogHandler()
	{
		UE_LOG(LogHairWorks, Log, TEXT("log handler destroyed\n"));
	}

	void Log(GFSDK_HAIR_LOG_TYPES logType, const char* message, const char* file, int line)
	{
		switch (logType)
		{
		case GFSDK_HAIR_LOG_ERROR:
			UE_LOG(LogHairWorks, Error, TEXT("%S"), message);
			break;
		case GFSDK_HAIR_LOG_WARNING:
			UE_LOG(LogHairWorks, Warning, TEXT("%S"), message);
			break;
		case GFSDK_HAIR_LOG_INFO:
			UE_LOG(LogHairWorks, Log, TEXT("%S"), message);
			break;
		};
	}
};

static CustomLogHandler s_logger;

// Pixel shaders
class FHairWorksPs : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FHairWorksPs, Global);

	FHairWorksPs()
	{}

	FHairWorksPs(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		LightDir.Bind(Initializer.ParameterMap, TEXT("LightDir"));
		LightColor.Bind(Initializer.ParameterMap, TEXT("LightColor"));
		LightAttenuation.Bind(Initializer.ParameterMap, TEXT("LightAttenuationTexture"));

		HairConstantBuffer.Bind(Initializer.ParameterMap, TEXT("HairConstantBuffer"));

		TextureSampler.Bind(Initializer.ParameterMap, TEXT("TextureSampler"));

		RootColorTexture.Bind(Initializer.ParameterMap, TEXT("RootColorTexture"));
		TipColorTexture.Bind(Initializer.ParameterMap, TEXT("TipColorTexture"));
		SpecularColorTexture.Bind(Initializer.ParameterMap, TEXT("SpecularColorTexture"));

		IndirectLightingSHCoefficients.Bind(Initializer.ParameterMap, TEXT("IndirectLightingSHCoefficients"));

		GFSDK_HAIR_RESOURCE_FACE_HAIR_INDICES.Bind(Initializer.ParameterMap, TEXT("GFSDK_HAIR_RESOURCE_FACE_HAIR_INDICES"));
		GFSDK_HAIR_RESOURCE_TANGENTS.Bind(Initializer.ParameterMap, TEXT("GFSDK_HAIR_RESOURCE_TANGENTS"));
		GFSDK_HAIR_RESOURCE_NORMALS.Bind(Initializer.ParameterMap, TEXT("GFSDK_HAIR_RESOURCE_NORMALS"));
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);

		Ar << HairConstantBuffer << TextureSampler << RootColorTexture << TipColorTexture << SpecularColorTexture << LightDir << LightColor << LightAttenuation << IndirectLightingSHCoefficients << GFSDK_HAIR_RESOURCE_FACE_HAIR_INDICES << GFSDK_HAIR_RESOURCE_TANGENTS << GFSDK_HAIR_RESOURCE_NORMALS;

		return bShaderHasOutdatedParameters;
	}

	void SetParameters(FRHICommandListImmediate& RHICmdList, const FSceneView& View, const GFSDK_Hair_ConstantBuffer& HairConstBuffer, const TArray<FTexture2DRHIRef>& HairTextures, const FVector& LightDir, const FLinearColor& LightColor, FTextureRHIRef LightAttenuation, const FVector4 IndirectLight[3])//const FTextureRHIParamRef LightCache[3], const FVector LightCatcheAlloc[4]
	{
		FGlobalShader::SetParameters(RHICmdList, GetPixelShader(), View);

		SetShaderValue(RHICmdList, GetPixelShader(), this->LightDir, -LightDir);
		SetShaderValue(RHICmdList, GetPixelShader(), this->LightColor, LightColor);
		SetTextureParameter(RHICmdList, GetPixelShader(), this->LightAttenuation, LightAttenuation);

		SetShaderValue(RHICmdList, GetPixelShader(), this->HairConstantBuffer, HairConstBuffer);

		SetSamplerParameter(RHICmdList, GetPixelShader(), TextureSampler, TStaticSamplerState<>::GetRHI());

		SetTextureParameter(RHICmdList, GetPixelShader(), RootColorTexture, HairTextures[GFSDK_HAIR_TEXTURE_ROOT_COLOR]);
		SetTextureParameter(RHICmdList, GetPixelShader(), TipColorTexture, HairTextures[GFSDK_HAIR_TEXTURE_TIP_COLOR]);
		SetTextureParameter(RHICmdList, GetPixelShader(), SpecularColorTexture, HairTextures[GFSDK_HAIR_TEXTURE_SPECULAR]);

		SetShaderValueArray(RHICmdList, GetPixelShader(), IndirectLightingSHCoefficients, IndirectLight, 3);

		check(GetUniformBufferParameter<FForwardLightData>().IsInitialized());
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
	}

protected:
	FShaderParameter LightDir;
	FShaderParameter LightColor;
	FShaderResourceParameter LightAttenuation;

	FShaderParameter HairConstantBuffer;

	FShaderResourceParameter TextureSampler;

	FShaderResourceParameter RootColorTexture;;
	FShaderResourceParameter TipColorTexture;
	FShaderResourceParameter SpecularColorTexture;

	FShaderParameter IndirectLightingSHCoefficients;

	// To suppress warnings.
	FShaderResourceParameter	GFSDK_HAIR_RESOURCE_FACE_HAIR_INDICES;
	FShaderResourceParameter	GFSDK_HAIR_RESOURCE_TANGENTS;
	FShaderResourceParameter	GFSDK_HAIR_RESOURCE_NORMALS;
};

IMPLEMENT_SHADER_TYPE(, FHairWorksPs, TEXT("HairWorks"), TEXT("Main"), SF_Pixel);

class FHairWorksMultiLightPs : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FHairWorksMultiLightPs, Global);

	FHairWorksMultiLightPs()
	{}

	FHairWorksMultiLightPs(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		LightAttenuation.Bind(Initializer.ParameterMap, TEXT("LightAttenuationTexture"));
		HairConstantBuffer.Bind(Initializer.ParameterMap, TEXT("HairConstantBuffer"));
		TextureSampler.Bind(Initializer.ParameterMap, TEXT("TextureSampler"));

		RootColorTexture.Bind(Initializer.ParameterMap, TEXT("RootColorTexture"));
		TipColorTexture.Bind(Initializer.ParameterMap, TEXT("TipColorTexture"));
		SpecularColorTexture.Bind(Initializer.ParameterMap, TEXT("SpecularColorTexture"));

		GFSDK_HAIR_RESOURCE_FACE_HAIR_INDICES.Bind(Initializer.ParameterMap, TEXT("GFSDK_HAIR_RESOURCE_FACE_HAIR_INDICES"));
		GFSDK_HAIR_RESOURCE_TANGENTS.Bind(Initializer.ParameterMap, TEXT("GFSDK_HAIR_RESOURCE_TANGENTS"));
		GFSDK_HAIR_RESOURCE_NORMALS.Bind(Initializer.ParameterMap, TEXT("GFSDK_HAIR_RESOURCE_NORMALS"));

		LightGrid.Bind(Initializer.ParameterMap, TEXT("LightGrid"));
		
		ExponentialFogParameters.Bind(Initializer.ParameterMap, TEXT("SharedFogParameter0"));
		ExponentialFogColorParameter.Bind(Initializer.ParameterMap, TEXT("SharedFogParameter1"));
		InscatteringLightDirection.Bind(Initializer.ParameterMap, TEXT("InscatteringLightDirection"));
		DirectionalInscatteringColor.Bind(Initializer.ParameterMap, TEXT("DirectionalInscatteringColor"));
		DirectionalInscatteringStartDistance.Bind(Initializer.ParameterMap, TEXT("DirectionalInscatteringStartDistance"));
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);

		Ar << HairConstantBuffer << TextureSampler << RootColorTexture << TipColorTexture << SpecularColorTexture << LightAttenuation << GFSDK_HAIR_RESOURCE_FACE_HAIR_INDICES << GFSDK_HAIR_RESOURCE_TANGENTS << GFSDK_HAIR_RESOURCE_NORMALS;
		Ar << LightGrid;
		Ar << ExponentialFogParameters;
		Ar << ExponentialFogColorParameter;
		Ar << InscatteringLightDirection;
		Ar << DirectionalInscatteringColor;
		Ar << DirectionalInscatteringStartDistance;

		return bShaderHasOutdatedParameters;
	}

	void SetParameters(FRHICommandListImmediate& RHICmdList, const FSceneView& View, const GFSDK_Hair_ConstantBuffer& HairConstBuffer, const TArray<FTexture2DRHIRef>& HairTextures, const FVector& LightDir, const FLinearColor& LightColor, FTextureRHIRef LightAttenuation, const FVector4 IndirectLight[3])//const FTextureRHIParamRef LightCache[3], const FVector LightCatcheAlloc[4]
	{
		FGlobalShader::SetParameters(RHICmdList, GetPixelShader(), View);

		SetTextureParameter(RHICmdList, GetPixelShader(), this->LightAttenuation, LightAttenuation);
		SetShaderValue(RHICmdList, GetPixelShader(), this->HairConstantBuffer, HairConstBuffer);
		SetSamplerParameter(RHICmdList, GetPixelShader(), TextureSampler, TStaticSamplerState<>::GetRHI());

		SetTextureParameter(RHICmdList, GetPixelShader(), RootColorTexture, HairTextures[GFSDK_HAIR_TEXTURE_ROOT_COLOR]);
		SetTextureParameter(RHICmdList, GetPixelShader(), TipColorTexture, HairTextures[GFSDK_HAIR_TEXTURE_TIP_COLOR]);
		SetTextureParameter(RHICmdList, GetPixelShader(), SpecularColorTexture, HairTextures[GFSDK_HAIR_TEXTURE_SPECULAR]);

		check(GetUniformBufferParameter<FForwardLightData>().IsInitialized());

		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.ForwardLighting"));
		bool bForwardLighting = CVar->GetValueOnRenderThread() != 0;

		IRendererModule& RendererModule = FModuleManager::LoadModuleChecked<IRendererModule>("Renderer");

		if (bForwardLighting)
		{
			SetUniformBufferParameter(RHICmdList, GetPixelShader(), GetUniformBufferParameter<FForwardLightData>(), View.ForwardLightData);
			RendererModule.SetLightGridResource(RHICmdList, GetPixelShader(), LightGrid);
		}
		else
		{
			SetUniformBufferParameter(RHICmdList, GetPixelShader(), GetUniformBufferParameter<FForwardLightData>(), 0);
			SetSRVParameter(RHICmdList, GetPixelShader(), LightGrid, 0);
		}
		RendererModule.SetHeightFogParams(RHICmdList, GetPixelShader(), &View, ExponentialFogParameters, ExponentialFogColorParameter, InscatteringLightDirection, DirectionalInscatteringColor, DirectionalInscatteringStartDistance);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
	}

protected:

	FShaderResourceParameter LightAttenuation;
	FShaderParameter HairConstantBuffer;
	FShaderResourceParameter TextureSampler;

	FShaderResourceParameter RootColorTexture;;
	FShaderResourceParameter TipColorTexture;
	FShaderResourceParameter SpecularColorTexture;

	// To suppress warnings.
	FShaderResourceParameter	GFSDK_HAIR_RESOURCE_FACE_HAIR_INDICES;
	FShaderResourceParameter	GFSDK_HAIR_RESOURCE_TANGENTS;
	FShaderResourceParameter	GFSDK_HAIR_RESOURCE_NORMALS;

	FShaderResourceParameter LightGrid;
	
	FShaderParameter ExponentialFogParameters;
	FShaderParameter ExponentialFogColorParameter;
	FShaderParameter InscatteringLightDirection;
	FShaderParameter DirectionalInscatteringColor;
	FShaderParameter DirectionalInscatteringStartDistance;
};

IMPLEMENT_SHADER_TYPE(, FHairWorksMultiLightPs, TEXT("HairWorks"), TEXT("MultiLightMain"), SF_Pixel);

class FHairWorksSimplePs : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FHairWorksSimplePs, Global);

	FHairWorksSimplePs()
	{}

	FHairWorksSimplePs(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
	}
};

IMPLEMENT_SHADER_TYPE(, FHairWorksSimplePs, TEXT("HairWorks"), TEXT("SimpleMain"), SF_Pixel);

class FHairWorksShadowDepthPs : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FHairWorksShadowDepthPs, Global);

	FHairWorksShadowDepthPs()
	{}

	FHairWorksShadowDepthPs(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		ShadowParams.Bind(Initializer.ParameterMap, TEXT("ShadowParams"));
	}

	bool Serialize(FArchive& Ar) override
	{
		bool bSerialized = FGlobalShader::Serialize(Ar);
		Ar << ShadowParams;
		return bSerialized;
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
	}

	FShaderParameter ShadowParams;
};

IMPLEMENT_SHADER_TYPE(, FHairWorksShadowDepthPs, TEXT("HairWorks"), TEXT("ShadowDepthMain"), SF_Pixel);

// HairWorks system
static GFSDK_HairSDK* HairWorksSdk = nullptr;

ENGINE_API void InitializeHair()
{
	// Check platform
	if (GMaxRHIShaderPlatform != EShaderPlatform::SP_PCD3D_SM5)
		return;

	// Initialize SDK
	FString LibPath = FPaths::EngineDir() / TEXT("Binaries/ThirdParty/HairWorks/GFSDK_HairWorks.win");

#if PLATFORM_64BITS
	LibPath += TEXT("64");
#else
	LibPath += TEXT("32");
#endif

	LibPath += TEXT(".dll");

	HairWorksSdk = GFSDK_LoadHairSDK(TCHAR_TO_ANSI(*LibPath), GFSDK_HAIRWORKS_VERSION, 0, &s_logger);
	if (!HairWorksSdk)
		return;

	if (GUsingNullRHI)
		return;

	UE_LOG(LogHairWorks, Log, TEXT("HairWorks: %S"), GFSDK_HAIRWORKS_FILE_VERSION_STRING);

	ID3D11Device* D3D11Device = (ID3D11Device*)GDynamicRHI->RHIGetNativeDevice();
	ID3D11DeviceContext* D3D11DeviceContext = nullptr;
	D3D11Device->GetImmediateContext(&D3D11DeviceContext);

	HairWorksSdk->InitRenderResources(D3D11Device, D3D11DeviceContext);
}

FHairSceneProxy::FHairSceneProxy(const UPrimitiveComponent* InComponent, UHair* InHair)
	:FPrimitiveSceneProxy(InComponent)
	, Hair(InHair)
{
}

FHairSceneProxy::~FHairSceneProxy()
{
	if (HairInstanceId != GFSDK_HairInstanceID_NULL)
		HairWorksSdk->FreeHairInstance(HairInstanceId);
}

uint32 FHairSceneProxy::GetMemoryFootprint(void) const
{
	return 0;
}

void FHairSceneProxy::StepSimulation()
{
	if (!HairWorksSdk)
		return;

	static uint32 LastFrameNumber = -1;
	if (LastFrameNumber != GFrameNumberRenderThread)
	{
		LastFrameNumber = GFrameNumberRenderThread;

		HairWorksSdk->StepSimulation();
	}
}

void FHairSceneProxy::StartMsaa()
{
	if (!HairWorksSdk)
		return;

	FRHICommandListExecutor::GetImmediateCommandList().SetDepthStencilState(TStaticDepthStencilState<>::GetRHI(), 0, true);	// Render targets may be changed in this function. We need to call this before MSAA.
	
	HairWorksSdk->StartMSAARendering(CVarHairMsaaLevel.GetValueOnRenderThread(), false);
}

void FHairSceneProxy::FinishMsaa()
{
	if (!HairWorksSdk)
		return;

	HairWorksSdk->FinishMSAARendering();
}

void FHairSceneProxy::DrawColorAndDepth()
{
	if (!HairWorksSdk)
		return;

	HairWorksSdk->DrawMSAAPostDepth();
	HairWorksSdk->DrawMSAAColor();
}

void FHairSceneProxy::UpdateShaderCache()
{
	GFSDK_HairShaderCacheSettings ShaderCacheSetting;
	ShaderCacheSetting.SetFromInstanceDescriptor(HairDesc);
	HairTextures.SetNum(GFSDK_HAIR_NUM_TEXTURES, false);
	for (int i = 0; i < GFSDK_HAIR_NUM_TEXTURES; i++)
	{
		ShaderCacheSetting.isTextureUsed[i] = HairTextures[i] != nullptr;
	}

	HairWorksSdk->AddToShaderCache(ShaderCacheSetting);
}

void FHairSceneProxy::DrawTranslucency(const FSceneView& View, const FVector& LightDir, const FLinearColor& LightColor, FTextureRHIRef LightAttenuation, const FVector4 IndirectLight[3])
{
	if (HairInstanceId == GFSDK_HairInstanceID_NULL)
		return;

	// Simulate.
	StepSimulation();

	// Pass rendering parameters
#define HairVisualizerCVarUpdate(name)	\
	HairDesc.m_visualize##name = CVarHairVisualize##name.GetValueOnRenderThread() != 0

	HairVisualizerCVarUpdate(GuideHairs);
	HairVisualizerCVarUpdate(SkinnedGuideHairs);
	HairVisualizerCVarUpdate(HairInteractions);
	HairVisualizerCVarUpdate(ControlVertices);
	HairVisualizerCVarUpdate(Frames);
	HairVisualizerCVarUpdate(LocalPos);
	HairVisualizerCVarUpdate(ShadingNormals);
	HairVisualizerCVarUpdate(GrowthMesh);
	HairVisualizerCVarUpdate(Bones);
	HairVisualizerCVarUpdate(Capsules);
	HairVisualizerCVarUpdate(BoundingBox);
	HairVisualizerCVarUpdate(PinConstraints);
	HairVisualizerCVarUpdate(ShadingNormalBone);

	HairWorksSdk->UpdateInstanceDescriptor(HairInstanceId, HairDesc);

	// Set states
	auto& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();

	RHICmdList.SetDepthStencilState(TStaticDepthStencilState<>::GetRHI(), 0, true);

	RHICmdList.SetBlendState(TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha>::GetRHI());

	// Pass camera inforamtin
	auto ViewMatrices = View.ViewMatrices;

	// Remove temporal AA jitter.
	if (!CVarHairTemporalAa.GetValueOnRenderThread())
	{
		ViewMatrices.ProjMatrix.M[2][0] = 0.0f;
		ViewMatrices.ProjMatrix.M[2][1] = 0.0f;
	}

	HairWorksSdk->SetViewProjection((gfsdk_float4x4*)ViewMatrices.ViewMatrix.M, (gfsdk_float4x4*)ViewMatrices.ProjMatrix.M, GFSDK_HAIR_LEFT_HANDED);

	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.ForwardLighting"));
	bool bForwardLighting = CVar->GetValueOnRenderThread() != 0;

	GFSDK_HairShaderConstantBuffer ConstBuffer;
	HairWorksSdk->PrepareShaderConstantBuffer(HairInstanceId, &ConstBuffer);

	// Setup shaders
	TShaderMapRef<FSimpleElementVS> VertexShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	HairTextures.SetNum(GFSDK_HAIR_NUM_TEXTURES, false);

	if (bForwardLighting)
	{
		static FGlobalBoundShaderState MultiLightBoundShaderState;
		TShaderMapRef<FHairWorksMultiLightPs> PixelShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
		SetGlobalBoundShaderState(
			RHICmdList,
			ERHIFeatureLevel::SM5,
			MultiLightBoundShaderState,
			GSimpleElementVertexDeclaration.VertexDeclarationRHI,
			*VertexShader,
			*PixelShader
			);
		PixelShader->SetParameters(RHICmdList, View, reinterpret_cast<GFSDK_Hair_ConstantBuffer&>(ConstBuffer), HairTextures, LightDir, LightColor, LightAttenuation, IndirectLight);
	}
	else
	{
		static FGlobalBoundShaderState BoundShaderState;
		TShaderMapRef<FHairWorksPs> PixelShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
		SetGlobalBoundShaderState(
			RHICmdList,
			ERHIFeatureLevel::SM5,
			BoundShaderState,
			GSimpleElementVertexDeclaration.VertexDeclarationRHI,
			*VertexShader,
			*PixelShader
			);
		PixelShader->SetParameters(RHICmdList, View, reinterpret_cast<GFSDK_Hair_ConstantBuffer&>(ConstBuffer), HairTextures, LightDir, LightColor, LightAttenuation, IndirectLight);
	}

	// To update shader states
	RHICmdList.DrawPrimitive(0, 0, 0, 0);

	// Handle shader cache.
	UpdateShaderCache();

	// Draw
	GFSDK_HairShaderSettings HairShaderSettings;
	HairShaderSettings.m_useCustomConstantBuffer = true;

	ID3D11ShaderResourceView* HairSrvs[GFSDK_HAIR_NUM_SHADER_RESOUCES];
	HairWorksSdk->GetShaderResources(HairInstanceId, HairSrvs);

	ID3D11Device* D3D11Device = (ID3D11Device*)GDynamicRHI->RHIGetNativeDevice();
	ID3D11DeviceContext* D3D11DeviceContext = nullptr;
	D3D11Device->GetImmediateContext(&D3D11DeviceContext);

	D3D11DeviceContext->PSSetShaderResources(10, GFSDK_HAIR_NUM_SHADER_RESOUCES, HairSrvs);

	HairWorksSdk->RenderHairs(HairInstanceId, &HairShaderSettings);
	HairWorksSdk->RenderVisualization(HairInstanceId);
}

void FHairSceneProxy::DrawShadow(const FViewMatrices& ViewMatrices, float DepthBias, float DepthScale)
{
	if (HairInstanceId == GFSDK_HairInstanceID_NULL)
		return;

	// Simulate
	StepSimulation();

	// Pass rendering parameters
	HairDesc.m_castShadows = true;

	const auto bOrgUseBackFaceCulling = HairDesc.m_useBackfaceCulling;
	const auto OrgHairWidth = HairDesc.m_width;
	HairDesc.m_width *= CVarHairShadowWidthScale.GetValueOnRenderThread();
	HairDesc.m_useBackfaceCulling = false;

	HairWorksSdk->UpdateInstanceDescriptor(HairInstanceId, HairDesc);

	// Revert parameters
	HairDesc.m_width = OrgHairWidth;
	HairDesc.m_useBackfaceCulling = bOrgUseBackFaceCulling;

	// Pass camera inforamtin
	HairWorksSdk->SetViewProjection((gfsdk_float4x4*)ViewMatrices.ViewMatrix.M, (gfsdk_float4x4*)ViewMatrices.ProjMatrix.M, GFSDK_HAIR_LEFT_HANDED);

	// Set shaders.
	auto& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();

	TShaderMapRef<FSimpleElementVS> VertexShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	TShaderMapRef<FHairWorksShadowDepthPs> PixelShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));

	static FGlobalBoundShaderState BoundShaderState;
	SetGlobalBoundShaderState(RHICmdList, ERHIFeatureLevel::SM5, BoundShaderState, GSimpleElementVertexDeclaration.VertexDeclarationRHI,
		*VertexShader, *PixelShader);

	SetShaderValue(RHICmdList, PixelShader->GetPixelShader(), PixelShader->ShadowParams, FVector2D(DepthBias * CVarHairShadowBiasScale.GetValueOnRenderThread(), DepthScale));

	// To update shader states
	RHICmdList.DrawPrimitive(0, 0, 0, 0);

	// Handle shader cache
	UpdateShaderCache();

	// Draw
	GFSDK_HairShaderSettings HairShaderSettings;
	HairShaderSettings.m_useCustomConstantBuffer = true;
	HairShaderSettings.m_shadowPass = true;

	HairWorksSdk->RenderHairs(HairInstanceId, &HairShaderSettings);
}

void FHairSceneProxy::DrawBasePass(const FSceneView& View)
{
	if (HairInstanceId == GFSDK_HairInstanceID_NULL)
		return;

	// Simulate
	StepSimulation();

	// Pass camera inforamtin
	auto ViewMatrices = View.ViewMatrices;

	// Remove temporal AA jitter.
	if (!CVarHairTemporalAa.GetValueOnRenderThread())
	{
		ViewMatrices.ProjMatrix.M[2][0] = 0.0f;
		ViewMatrices.ProjMatrix.M[2][1] = 0.0f;
	}

	HairWorksSdk->SetViewProjection((gfsdk_float4x4*)ViewMatrices.ViewMatrix.M, (gfsdk_float4x4*)ViewMatrices.ProjMatrix.M, GFSDK_HAIR_LEFT_HANDED);

	// Set render states
	auto& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();

	RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());
	RHICmdList.SetDepthStencilState(TStaticDepthStencilState<>::GetRHI(), 0, true);

	// Set shaders
	TShaderMapRef<FSimpleElementVS> VertexShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	TShaderMapRef<FHairWorksSimplePs> PixelShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	static FGlobalBoundShaderState BoundShaderState;
	SetGlobalBoundShaderState(RHICmdList, ERHIFeatureLevel::SM5, BoundShaderState, GSimpleElementVertexDeclaration.VertexDeclarationRHI,
		*VertexShader, *PixelShader);

	// Handle shader cache.
	UpdateShaderCache();

	// Draw
	GFSDK_HairShaderSettings HairShaderSettings;
	HairShaderSettings.m_useCustomConstantBuffer = true;

	HairWorksSdk->RenderHairs(HairInstanceId, &HairShaderSettings);
}

void FHairSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, class FMeshElementCollector& Collector) const
{
}

void FHairSceneProxy::CreateRenderThreadResources()
{
	FPrimitiveSceneProxy::CreateRenderThreadResources();

	if (!HairWorksSdk)
		return;

	// Initialize Hair asset and instance
	if (Hair->AssetId == GFSDK_HairAssetID_NULL)
	{
		GFSDK_HairConversionSettings LoadSettings;
		LoadSettings.m_targetHandednessHint = GFSDK_HAIR_HANDEDNESS_HINT::GFSDK_HAIR_LEFT_HANDED;
		LoadSettings.m_targetUpAxisHint = GFSDK_HAIR_UP_AXIS_HINT::GFSDK_HAIR_Z_UP;
		HairWorksSdk->LoadHairAssetFromMemory(Hair->AssetData.GetData(), Hair->AssetData.Num(), &Hair->AssetId, 0, &LoadSettings);
	}

	HairWorksSdk->CreateHairInstance(Hair->AssetId, &HairInstanceId);

	// Get parameters
	HairWorksSdk->CopyInstanceDescriptorFromAsset(Hair->AssetId, HairDesc);
}

FPrimitiveViewRelevance FHairSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance ViewRel;
	ViewRel.bDrawRelevance = IsShown(View);
	ViewRel.bShadowRelevance = IsShadowCast(View);
	ViewRel.bDynamicRelevance = true;
	ViewRel.bRenderCustomDepth = false;
	ViewRel.bRenderInMainPass = true;
	ViewRel.bOpaqueRelevance = true;
	ViewRel.bMaskedRelevance = false;
	ViewRel.bDistortionRelevance = false;
	ViewRel.bSeparateTranslucencyRelevance = false;
	ViewRel.bNormalTranslucencyRelevance = true;

	ViewRel.bHair = true;

	return ViewRel;
}

void FHairSceneProxy::SetupBoneMapping_GameThread(const TArray<FMeshBoneInfo>& Bones)
{
	// Send bone names to setup mapping
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		HairSetupBoneMapping,
		FHairSceneProxy&, ThisProxy, *this,
		TArray<FMeshBoneInfo>, Bones, Bones,
		{
			ThisProxy.SetupBoneMapping_RenderThread(Bones);
		}
		)
}

void FHairSceneProxy::SetupBoneMapping_RenderThread(const TArray<FMeshBoneInfo>& Bones)
{
	// Setup bone mapping
	if (HairInstanceId == GFSDK_HairInstanceID_NULL)
		return;
	
	gfsdk_U32 BoneNum = 0;
	HairWorksSdk->GetNumBones(Hair->AssetId, &BoneNum);

	BoneMapping.SetNumUninitialized(BoneNum);

	for (auto Idx = 0; Idx < BoneMapping.Num(); ++Idx)
	{
		gfsdk_char BoneName[GFSDK_HAIR_MAX_STRING];
		HairWorksSdk->GetBoneName(Hair->AssetId, Idx, BoneName);

		BoneMapping[Idx] = Bones.IndexOfByPredicate([&](const FMeshBoneInfo& BoneInfo){return BoneInfo.Name == BoneName; });
	}
}

void FHairSceneProxy::UpdateBones_GameThread(USkinnedMeshComponent& ParentSkeleton)
{
	// Send bone matrices for rendering
	TSharedRef<FDynamicSkelMeshObjectDataGPUSkin> DynamicData = MakeShareable(new FDynamicSkelMeshObjectDataGPUSkin(
		&ParentSkeleton,
		ParentSkeleton.SkeletalMesh->GetResourceForRendering(),
		0,
		TArray<FActiveVertexAnim>()
		));

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		HairUpdateData,
		FHairSceneProxy&, ThisProxy, *this,
		TSharedRef<FDynamicSkelMeshObjectDataGPUSkin>, DynamicData, DynamicData,
		{
			ThisProxy.UpdateBones_RenderThread(DynamicData->ReferenceToLocal);
		}
		);
}

void FHairSceneProxy::UpdateBones_RenderThread(const TArray<FMatrix>& RefMatrices)
{
	if (HairInstanceId == GFSDK_HairInstanceID_NULL)
		return;

	// Setup bones
	FMemMark MemMark(FMemStack::Get());
	TArray<FMatrix, TMemStackAllocator<>> BoneMatrices;
	BoneMatrices.SetNumUninitialized(BoneMapping.Num());

	for (auto Idx = 0; Idx < BoneMatrices.Num(); ++Idx)
	{
		auto SkeletonBoneIdx = BoneMapping[Idx];
		BoneMatrices[Idx] = SkeletonBoneIdx >= 0 && SkeletonBoneIdx < RefMatrices.Num() ? RefMatrices[SkeletonBoneIdx] : FMatrix::Identity;
	}

	// Update to hair
	HairWorksSdk->UpdateSkinningMatrices(HairInstanceId, BoneMatrices.Num(), (gfsdk_float4x4*)BoneMatrices.GetData());
}

void FHairSceneProxy::UpdateHairParams_GameThread(const GFSDK_HairInstanceDescriptor& HairDesc, const TArray<FTexture2DRHIRef>& HairTextures)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		HairUpdateParams,
		FHairSceneProxy&, ThisProxy, *this,
		GFSDK_HairInstanceDescriptor, HairDesc, HairDesc,
		const TArray<FTexture2DRHIRef>, HairTextures, HairTextures,
		{
			ThisProxy.UpdateHairParams_RenderThread(HairDesc, HairTextures);
		}
		);
}

void FHairSceneProxy::UpdateHairParams_RenderThread(const GFSDK_HairInstanceDescriptor& InHairDesc, const TArray<FTexture2DRHIRef>& InHairTextures)
{
	if (HairInstanceId == GFSDK_HairInstanceID_NULL)
		return;

	// Update parameters
	HairDesc = InHairDesc;
	HairDesc.m_modelToWorld = (gfsdk_float4x4&)GetLocalToWorld().M;
	HairDesc.m_useViewfrustrumCulling = false;
	HairWorksSdk->UpdateInstanceDescriptor(HairInstanceId, HairDesc);	// Mainly for simulation.

	// Update textures
	checkSlow(InHairTextures.Num() >= GFSDK_HAIR_NUM_TEXTURES);
	HairTextures = InHairTextures;
	HairTextures.SetNum(GFSDK_HAIR_NUM_TEXTURES, false);

	for (auto Idx = 0; Idx < GFSDK_HAIR_NUM_TEXTURES; ++Idx)
	{
		FTexture2DRHIRef TextureRef = HairTextures[Idx];
		ID3D11ShaderResourceView* TextureSRV = (TextureRef != nullptr) ? (ID3D11ShaderResourceView*)TextureRef->GetNativeShaderResourceView() : nullptr;

		HairWorksSdk->SetTextureSRV(HairInstanceId, (GFSDK_HAIR_TEXTURE_TYPE)Idx, TextureSRV);
	}
}

bool FHairSceneProxy::IsHair_GameThread(const void* AssetData, unsigned DataSize)
{
	if (!HairWorksSdk)
		return false;

	bool IsHairAsset = false;

	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		IsHairAsset,
		const void*, AssetData, AssetData,
		unsigned, DataSize, DataSize,
		bool&, IsHairAsset, IsHairAsset,
		{
			// Try to create Hair asset
			GFSDK_HairAssetID AssetId = GFSDK_HairAssetID_NULL;
			HairWorksSdk->LoadHairAssetFromMemory(AssetData, DataSize, &AssetId);

			// Release created asset
			if (AssetId == GFSDK_HairAssetID_NULL)
				return;

			HairWorksSdk->FreeHairAsset(AssetId);
			IsHairAsset = true;
		}
	);

	FRenderCommandFence RenderCmdFenc;
	RenderCmdFenc.BeginFence();
	RenderCmdFenc.Wait();

	return IsHairAsset;
}

void FHairSceneProxy::ReleaseHair_GameThread(GFSDK_HairAssetID AssetId)
{
	if (!HairWorksSdk || AssetId == GFSDK_HairAssetID_NULL)
		return;

	HairWorksSdk->FreeHairAsset(AssetId);
}

bool FHairSceneProxy::GetHairInfo_GameThread(GFSDK_HairInstanceDescriptor& HairDescriptor, TMap<FName, int32>& BoneToIdxMap, const UHair& Hair)
{
	if (!HairWorksSdk)
		return false;

	bool bOk = false;

	ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER(
		GetHairInfo,
		GFSDK_HairInstanceDescriptor&, HairDescriptor, HairDescriptor,
		decltype(BoneToIdxMap)&, BoneToIdxMap, BoneToIdxMap,	// Use decltype to avoid compile error.
		const UHair&, Hair, Hair,
		bool&, bOk, bOk,
		{
			// Try to create HairWorks asset
			auto AssetId = Hair.AssetId;
			if (AssetId == GFSDK_HairAssetID_NULL)
			{
				HairWorksSdk->LoadHairAssetFromMemory(Hair.AssetData.GetData(), Hair.AssetData.Num() * Hair.AssetData.GetTypeSize(), &AssetId);
			}

			if (AssetId == GFSDK_HairAssetID_NULL)
				return;

			// Copy descriptor
			HairWorksSdk->CopyInstanceDescriptorFromAsset(AssetId, HairDescriptor);

			// Copy bones
			gfsdk_U32 BoneNum = 0;
			HairWorksSdk->GetNumBones(AssetId, &BoneNum);

			for (gfsdk_U32 BoneIdx = 0; BoneIdx < BoneNum; ++BoneIdx)
			{
				gfsdk_char BoneName[GFSDK_HAIR_MAX_STRING];
				HairWorksSdk->GetBoneName(AssetId, BoneIdx, BoneName);

				BoneToIdxMap.Add(BoneName, BoneIdx);
			}

			// Release asset
			if (AssetId != Hair.AssetId)
				HairWorksSdk->FreeHairAsset(AssetId);

			bOk = true;
		}
	);

	// Wait
	FRenderCommandFence RenderCmdFenc;
	RenderCmdFenc.BeginFence();
	RenderCmdFenc.Wait();

	return bOk;
}

bool FHairSceneProxy::GetHairBounds_GameThread(FBoxSphereBounds& Bounds)const
{
	if (HairInstanceId == GFSDK_HairInstanceID_NULL)
		return false;

	FBox HairBox;
	if(HairWorksSdk->GetBounds(HairInstanceId, reinterpret_cast<gfsdk_float3*>(&HairBox.Min), reinterpret_cast<gfsdk_float3*>(&HairBox.Max)) != GFSDK_HAIR_RETURN_OK)
		return false;

	Bounds = FBoxSphereBounds(HairBox);

	return true;

	//// Send command to get hair bounds
	//bool bOk = false;

	//ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
	//	GetHairBounds,
	//	const FHairSceneProxy&, ThisProxy, *this,
	//	bool&, bOk, bOk,
	//	FBoxSphereBounds&, Bounds, Bounds,
	//	{
	//		if (ThisProxy.HairInstanceId == GFSDK_HairInstanceID_NULL)
	//			return;

	//		FBox HairBox;
	//		HairWorksSdk->GetBounds(ThisProxy.HairInstanceId, reinterpret_cast<gfsdk_float3*>(&HairBox.Min), reinterpret_cast<gfsdk_float3*>(&HairBox.Max));

	//		Bounds = FBoxSphereBounds(HairBox);

	//		bOk = true;
	//	}
	//);

	//// Wait
	//FRenderCommandFence RenderCmdFenc;
	//RenderCmdFenc.BeginFence();
	//RenderCmdFenc.Wait();

	//return bOk;
}