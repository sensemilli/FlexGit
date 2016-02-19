// Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.

#pragma once

// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI

#include "GFSDK_VXGI.h"
#include "LightRendering.h"
#include "LightMapRendering.h"
#include "ShaderBaseClasses.h"

namespace VXGI
{
	struct RenderMeshFilter
	{
		enum Enum
		{
			ALL_MESHES = 0,
			ONLY_EMISSIVE,
		};
	};

	struct VoxelizationPass
	{
		enum Enum
		{
			EMISSIVE_AND_IRRADIANCE = -1,
			OPACITY = 0,
			LIGHT0 = 1
		};
	};
};

class FVXGIVoxelizationNoLightMapPolicy : public FNoLightMapPolicy
{
public:
	class PixelParametersType : public FNoLightMapPolicy::PixelParametersType
	{
	public:
		void Bind(const FShaderParameterMap& ParameterMap);
		void Serialize(FArchive& Ar);
		FShaderParameter DirectionalLightShadowing;
	};

	static bool ShouldCache(EShaderPlatform Platform, const FMaterial* Material, const FVertexFactoryType* VertexFactoryType);
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment);
	void SetMesh(
		FRHICommandList& RHICmdList,
		const FSceneView& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const VertexParametersType* VertexShaderParameters,
		const PixelParametersType* PixelShaderParameters,
		FShader* VertexShader,
		FShader* PixelShader,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const ElementDataType& ElementData
		) const;

	/** Initialization constructor. */
	FVXGIVoxelizationNoLightMapPolicy() {}
};

class FVXGIVoxelizationMeshMaterialShaderType : public FMeshMaterialShaderType
{
public:
	FVXGIVoxelizationMeshMaterialShaderType(
		const TCHAR* InName,
		const TCHAR* InSourceFilename,
		const TCHAR* InFunctionName,
		uint32 InFrequency,
		ConstructSerializedType InConstructSerializedRef,
		ConstructCompiledType InConstructCompiledRef,
		ModifyCompilationEnvironmentType InModifyCompilationEnvironmentRef,
		ShouldCacheType InShouldCacheRef,
		GetStreamOutElementsType InGetStreamOutElementsRef
		) :
		FMeshMaterialShaderType(InName, InSourceFilename, InFunctionName, InFrequency, InConstructSerializedRef, InConstructCompiledRef, InModifyCompilationEnvironmentRef, InShouldCacheRef, InGetStreamOutElementsRef)
	{
	}

	mutable FSHAHash HashWithVXGIHash;

	virtual const FSHAHash& GetSourceHash() const override;
};

template<typename LightMapPolicyType>
class TVXGIVoxelizationShader : public FMeshMaterialShader
{
protected:
	TVXGIVoxelizationShader()  {}
	TVXGIVoxelizationShader(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) :
		FMeshMaterialShader(Initializer)
	{
	}

public:
	static bool ShouldCache(EShaderPlatform Platform, const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		if (Platform != SP_PCD3D_SM5)
		{
			return false;
		}

		bool LightMapResult = LightMapPolicyType::ShouldCache(Platform, Material, VertexFactoryType);
		bool MaterialResult = TVXGIVoxelizationDrawingPolicyFactory::IsMaterialVoxelized(Material);
		return LightMapResult && MaterialResult;
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		LightMapPolicyType::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("WITH_GFSDK_VXGI"), 1);
		OutEnvironment.SetDefine(TEXT("VXGI_VOXELIZATION_SHADER"), 1);
		OutEnvironment.SetDefine(TEXT("VXGI_VOXELIZATION_COVERAGE_SUPERSAMPLING"), Material->GetVxgiCoverageSupersampling() ? 1 : 0);

		//Turn tessellation off for this mode regardless of what the material says
		if (!Material->GetVxgiAllowTesselationDuringVoxelization())
		{
			OutEnvironment.SetDefine(TEXT("USING_TESSELLATION"), TEXT("0"));
		}

		UE_LOG(LogShaders, Log, TEXT("Compiling Material %s for voxelization"), *Material->GetFriendlyName());
	}
};

template<typename LightMapPolicyType>
class TVXGIVoxelizationVS : public TVXGIVoxelizationShader<LightMapPolicyType> 
{
	DECLARE_SHADER_TYPE(TVXGIVoxelizationVS, VXGIVoxelizationMeshMaterial);

protected:
	TVXGIVoxelizationVS()  {}
	TVXGIVoxelizationVS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) :
		TVXGIVoxelizationShader<LightMapPolicyType>(Initializer)
	{
		VertexParameters.Bind(Initializer.ParameterMap);
	}

public:
	typename LightMapPolicyType::VertexParametersType VertexParameters;

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = TVXGIVoxelizationShader<LightMapPolicyType>::Serialize(Ar);
		VertexParameters.Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(
		FRHICommandList& RHICmdList,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FVertexFactory* VertexFactory,
		const FMaterial& InMaterialResource,
		const FSceneView& View,
		ESceneRenderTargetsMode::Type TextureMode
		)
	{
		TVXGIVoxelizationShader<LightMapPolicyType>::SetParameters(RHICmdList, GetVertexShader(), MaterialRenderProxy, InMaterialResource, View, TextureMode);
	}

	void SetMesh(FRHICommandList& RHICmdList, const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatchElement& BatchElement, float DitheredLODTransitionValue, uint32 DataFlags = 0)
	{
		TVXGIVoxelizationShader<LightMapPolicyType>::SetMesh(RHICmdList, GetVertexShader(), VertexFactory, View, Proxy, BatchElement, DitheredLODTransitionValue, DataFlags);
	}
};

class FVXGIEmittanceShadowProjectionShaderParameters
{
public:
	FShaderParameter NumCascades;
	FShaderParameter ShadowBufferSize;
	FShaderParameter WorldToShadowMatrixArray;
	FShaderResourceParameter ShadowDepthTextureSampler;
	FShaderResourceParameter ShadowDepthTextureArray[NUM_SHADOW_CASCADE_SURFACES];
	FShaderParameter SoftTransitionScaleArray[NUM_SHADOW_CASCADE_SURFACES];

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		NumCascades.Bind(ParameterMap, TEXT("NumCascades"));
		ShadowBufferSize.Bind(ParameterMap, TEXT("ShadowBufferSize"));
		WorldToShadowMatrixArray.Bind(ParameterMap, TEXT("WorldToShadowMatrices"));
		ShadowDepthTextureSampler.Bind(ParameterMap, TEXT("ShadowDepthTextureSampler"));
		for (int32 CascadeIndex = 0; CascadeIndex < NUM_SHADOW_CASCADE_SURFACES; ++CascadeIndex)
		{
			ShadowDepthTextureArray[CascadeIndex].Bind(ParameterMap, *FString::Printf(TEXT("ShadowDepthTexture%d"), CascadeIndex));
			SoftTransitionScaleArray[CascadeIndex].Bind(ParameterMap, *FString::Printf(TEXT("SoftTransitionScale%d"), CascadeIndex));
		}
	}

	template<typename ShaderRHIParamRef>
	void Set(
		FRHICommandList& RHICmdList,
		const ShaderRHIParamRef ShaderRHI,
		const FSceneView& View) const
	{
		TArray<FMatrix> WorldToShadowMatrixArrayValue;
		TArray<float> SoftTransitionScaleArrayValue;
		TArray<FTexture2DRHIRef> ShadowDepthTextureArrayValue;
		FVector2D ShadowBufferSizeValue(0.f, 0.f);

		const TArray<FProjectedShadowInfo*, SceneRenderingAllocator>& Shadows = View.VxgiEmittanceVoxelizationArgs.Shadows;
		const int32 ShadowCount = FMath::Min(Shadows.Num(), NUM_SHADOW_CASCADE_SURFACES);

		static const auto CVarEmittanceShadowCascade = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.VXGI.EmittanceShadowCascade"));
		const int32 SelectedCascadeIndex = CVarEmittanceShadowCascade->GetValueOnRenderThread();

		for (int32 ShadowIndex = 0; ShadowIndex < ShadowCount; ++ShadowIndex)
		{
			const FProjectedShadowInfo* ProjectedShadowInfo = Shadows[ShadowIndex];
			if (!ProjectedShadowInfo->bRendered)
			{
				continue;
			}
			if (ProjectedShadowInfo->CascadeSettings.bOnePassPointLightShadow)
			{
				continue;
			}
			if (SelectedCascadeIndex >= 0 && SelectedCascadeIndex != ShadowIndex)
			{
				continue;
			}

			FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

			FVector4 ShadowmapMinMaxValue;
			WorldToShadowMatrixArrayValue.Add(ProjectedShadowInfo->GetWorldToShadowMatrix(ShadowmapMinMaxValue));
			SoftTransitionScaleArrayValue.Add(1.0f / ProjectedShadowInfo->ComputeTransitionSize());
			ShadowDepthTextureArrayValue.Add(SceneContext.GetShadowDepthZTexture(ProjectedShadowInfo->CascadeSurfaceIndex, ProjectedShadowInfo->bAllocatedInPreshadowCache));
			ShadowBufferSizeValue = FVector2D(ProjectedShadowInfo->GetShadowBufferResolution());
		}

		SetShaderValue(RHICmdList, ShaderRHI, NumCascades, ShadowDepthTextureArrayValue.Num());

		SetShaderValue(RHICmdList, ShaderRHI, ShadowBufferSize, FVector4(ShadowBufferSizeValue.X, ShadowBufferSizeValue.Y, 1.0f / ShadowBufferSizeValue.X, 1.0f / ShadowBufferSizeValue.Y));

		SetShaderValueArray<ShaderRHIParamRef, FMatrix>(
			RHICmdList,
			ShaderRHI,
			WorldToShadowMatrixArray,
			WorldToShadowMatrixArrayValue.GetData(),
			WorldToShadowMatrixArrayValue.Num());

		SetSamplerParameter(RHICmdList, ShaderRHI, ShadowDepthTextureSampler, TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());

		for (int32 CascadeIndex = 0; CascadeIndex < ShadowDepthTextureArrayValue.Num(); ++CascadeIndex)
		{
			if (ShadowDepthTextureArray[CascadeIndex].IsBound())
			{
				SetTextureParameter(
					RHICmdList,
					ShaderRHI,
					ShadowDepthTextureArray[CascadeIndex],
					ShadowDepthTextureArrayValue[CascadeIndex]);
			}
		}

		for (int32 CascadeIndex = 0; CascadeIndex < SoftTransitionScaleArrayValue.Num(); ++CascadeIndex)
		{
			SetShaderValue<ShaderRHIParamRef, float>(RHICmdList, ShaderRHI, SoftTransitionScaleArray[CascadeIndex], SoftTransitionScaleArrayValue[CascadeIndex]);
		}
	}

	friend FArchive& operator<<(FArchive& Ar, FVXGIEmittanceShadowProjectionShaderParameters& P)
	{
		Ar << P.NumCascades;
		Ar << P.ShadowBufferSize;
		Ar << P.WorldToShadowMatrixArray;
		Ar << P.ShadowDepthTextureSampler;
		for (int32 CascadeIndex = 0; CascadeIndex < NUM_SHADOW_CASCADE_SURFACES; ++CascadeIndex)
		{
			Ar << P.ShadowDepthTextureArray[CascadeIndex];
			Ar << P.SoftTransitionScaleArray[CascadeIndex];
		}
		return Ar;
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("NUM_SHADOW_CASCADE_SURFACES"), NUM_SHADOW_CASCADE_SURFACES);
	}
};

class FEmittanceVoxelizationParameters
{
public:
	FShaderParameter EmittanceShadingMode;
	FShaderParameter EmittanceShadowQuality;
	FShaderParameter IsInverseSquared;
	FShaderParameter IsRadialLight;
	FShaderParameter IsSpotLight;
	FShaderParameter IsPointLight;
	FShaderParameter NumLights;
	FShaderParameter NumShadows;
	FVXGIEmittanceShadowProjectionShaderParameters EmittanceShadowProjectionShaderParameters;
	FOnePassPointShadowProjectionShaderParameters OnePassPointShadowProjectionShaderParameters;
	FShaderParameter PointLightDepthBiasParameters;
	FShaderParameter EnableEmissive;
	FShaderParameter EnableIndirectIrradiance;
	bool bEnableEmissiveForMesh;
	bool bEnableIndirectIrradianceForMesh;

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		EmittanceShadingMode.Bind(ParameterMap, TEXT("EmittanceShadingMode"));
		EmittanceShadowQuality.Bind(ParameterMap, TEXT("EmittanceShadowQuality"));
		IsInverseSquared.Bind(ParameterMap, TEXT("IsInverseSquared"));
		IsRadialLight.Bind(ParameterMap, TEXT("IsRadialLight"));
		IsSpotLight.Bind(ParameterMap, TEXT("IsSpotLight"));
		IsPointLight.Bind(ParameterMap, TEXT("IsPointLight"));
		NumLights.Bind(ParameterMap, TEXT("NumLights"));
		NumShadows.Bind(ParameterMap, TEXT("NumShadows"));
		EmittanceShadowProjectionShaderParameters.Bind(ParameterMap);
		OnePassPointShadowProjectionShaderParameters.Bind(ParameterMap);
		PointLightDepthBiasParameters.Bind(ParameterMap, TEXT("PointLightDepthBiasParameters"));
		EnableEmissive.Bind(ParameterMap, TEXT("EnableEmissive"));
		EnableIndirectIrradiance.Bind(ParameterMap, TEXT("EnableIndirectIrradiance"));
	}

	template<typename ShaderRHIParamRef>
	void Set(
		FRHICommandList& RHICmdList,
		const ShaderRHIParamRef ShaderRHI,
		const FShader* Shader,
		const FSceneView& View) const
	{
		static const auto CVarEmittanceShadingMode = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.VXGI.EmittanceShadingMode"));
		SetShaderValue(RHICmdList, ShaderRHI, EmittanceShadingMode, CVarEmittanceShadingMode->GetValueOnRenderThread());

		static const auto CVarEmittanceShadowQuality = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.VXGI.EmittanceShadowQuality"));
		SetShaderValue(RHICmdList, ShaderRHI, EmittanceShadowQuality, CVarEmittanceShadowQuality->GetValueOnRenderThread());

		const FLightSceneInfo* LightSceneInfo = View.VxgiEmittanceVoxelizationArgs.LightSceneInfo;
		const FProjectedShadowInfo* ProjectedShadowInfo = NULL;
		if (View.VxgiEmittanceVoxelizationArgs.Shadows.Num() != 0)
		{
			ProjectedShadowInfo = View.VxgiEmittanceVoxelizationArgs.Shadows[0];
		}

		static const auto CVarEmittanceShadowEnable = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.VXGI.EmittanceShadowEnable"));
		if (!CVarEmittanceShadowEnable->GetValueOnRenderThread())
		{
			ProjectedShadowInfo = NULL;
		}

		if (LightSceneInfo)
		{
			SetDeferredLightParameters(RHICmdList, ShaderRHI, Shader->GetUniformBufferParameter<FDeferredLightUniformStruct>(), LightSceneInfo, View);

			SetShaderValue(RHICmdList, ShaderRHI, NumLights, 1);

			SetShaderValue(RHICmdList, ShaderRHI, IsInverseSquared, LightSceneInfo->Proxy->IsInverseSquared());
			SetShaderValue(RHICmdList, ShaderRHI, IsRadialLight, (LightSceneInfo->Proxy->GetLightType() != LightType_Directional));
			SetShaderValue(RHICmdList, ShaderRHI, IsSpotLight, (LightSceneInfo->Proxy->GetLightType() == LightType_Spot));
			SetShaderValue(RHICmdList, ShaderRHI, IsPointLight, (LightSceneInfo->Proxy->GetLightType() == LightType_Point));
		}
		else
		{
			FDeferredLightUniformStruct DeferredLightUniformsValue;
			SetUniformBufferParameterImmediate(RHICmdList, ShaderRHI, Shader->GetUniformBufferParameter<FDeferredLightUniformStruct>(), DeferredLightUniformsValue);

			SetShaderValue(RHICmdList, ShaderRHI, NumLights, 0);
		}

		bool bNullProjectedShadow = true;
		bool bNullPointLightShadow = true;

		if (ProjectedShadowInfo)
		{
			SetShaderValue(RHICmdList, ShaderRHI, NumShadows, 1);

			if (ProjectedShadowInfo->CascadeSettings.bOnePassPointLightShadow)
			{
				OnePassPointShadowProjectionShaderParameters.Set(RHICmdList, ShaderRHI, ProjectedShadowInfo);
				SetShaderValue(RHICmdList, ShaderRHI, PointLightDepthBiasParameters, FVector2D(ProjectedShadowInfo->GetShaderDepthBias(), 0.f));
				bNullPointLightShadow = false;
			}
			else
			{
				EmittanceShadowProjectionShaderParameters.Set(RHICmdList, ShaderRHI, View);
				bNullProjectedShadow = false;
			}
		}
		else
		{
			SetShaderValue(RHICmdList, ShaderRHI, NumShadows, 0);
		}

		// Set proper samplers and null textures to keep the D3D runtime happy: there is one shader for all cases, and it references all textures and samplers.

		if (bNullProjectedShadow)
		{
			SetSamplerParameter(RHICmdList, ShaderRHI, EmittanceShadowProjectionShaderParameters.ShadowDepthTextureSampler, TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());

			for (int32 CascadeIndex = 0; CascadeIndex < NUM_SHADOW_CASCADE_SURFACES; ++CascadeIndex)
				SetTextureParameter(RHICmdList, ShaderRHI, EmittanceShadowProjectionShaderParameters.ShadowDepthTextureArray[CascadeIndex], nullptr);
		}

		if (bNullPointLightShadow)
		{
			SetSamplerParameter(RHICmdList, ShaderRHI, OnePassPointShadowProjectionShaderParameters.ShadowDepthCubeComparisonSampler, TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp, 0, 0, 0, SCF_Less>::GetRHI());
			SetTextureParameter(RHICmdList, ShaderRHI, OnePassPointShadowProjectionShaderParameters.ShadowDepthTexture, nullptr);
		}

		SetShaderValue(RHICmdList, ShaderRHI, EnableEmissive, bEnableEmissiveForMesh);
		SetShaderValue(RHICmdList, ShaderRHI, EnableIndirectIrradiance, bEnableIndirectIrradianceForMesh);
	}

	void Serialize(FArchive& Ar)
	{
		Ar << EmittanceShadingMode;
		Ar << EmittanceShadowQuality;
		Ar << IsInverseSquared;
		Ar << IsRadialLight;
		Ar << IsSpotLight;
		Ar << IsPointLight;
		Ar << NumLights;
		Ar << NumShadows;
		Ar << EmittanceShadowProjectionShaderParameters;
		Ar << OnePassPointShadowProjectionShaderParameters;
		Ar << PointLightDepthBiasParameters;
		Ar << EnableEmissive;
		Ar << EnableIndirectIrradiance;
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("VXGI_EMITTANCE_VOXELIZATION"), 1);
		FVXGIEmittanceShadowProjectionShaderParameters::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}
};

//Unfortunately all the binding logic happens in the constructor so we need to make a fake shader to bind stuff
template<typename LightMapPolicyType>
class TVXGIVoxelizationShaderPermutationPS : public TVXGIVoxelizationShader < LightMapPolicyType >
{
	DECLARE_SHADER_TYPE(TVXGIVoxelizationShaderPermutationPS, VXGIVoxelizationMeshMaterial);

public:
	FPixelShaderRHIParamRef MyShader; //The permutation we are using. This comes from our parent's Resource so we don't serialize this

	typename LightMapPolicyType::PixelParametersType PixelParameters;
	FEmittanceVoxelizationParameters EmittanceVoxelizationParameters;

	/** Initialization constructor. */
	TVXGIVoxelizationShaderPermutationPS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer)
		: TVXGIVoxelizationShader<LightMapPolicyType>(Initializer)
		, MyShader(NULL)
	{
		PixelParameters.Bind(Initializer.ParameterMap);
		EmittanceVoxelizationParameters.Bind(Initializer.ParameterMap);
		SetResource(NULL);
	}
	TVXGIVoxelizationShaderPermutationPS() {}

	virtual const FPixelShaderRHIParamRef GetPixelShader() override
	{
		//Store this here since our resource is null since we are not a real shader.
		return MyShader;
	}

	void SetParameters(
		FRHICommandList& RHICmdList,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FMaterial& InMaterialResource,
		const FSceneView& View,
		ESceneRenderTargetsMode::Type TextureMode
		)
	{
		// Set LightMapPolicy parameters
		TVXGIVoxelizationShader<LightMapPolicyType>::SetParameters(RHICmdList, MyShader, MaterialRenderProxy, InMaterialResource, View, TextureMode);
	}

	void SetMesh(FRHICommandList& RHICmdList, const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatchElement& BatchElement, float DitheredLODTransitionValue, uint32 DataFlags = 0)
	{
		EmittanceVoxelizationParameters.Set(RHICmdList, MyShader, this, View);

		TVXGIVoxelizationShader<LightMapPolicyType>::SetMesh(RHICmdList, MyShader, VertexFactory, View, Proxy, BatchElement, DitheredLODTransitionValue, DataFlags);
	}

	static bool ShouldCache(EShaderPlatform Platform, const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		return false;
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = TVXGIVoxelizationShader<LightMapPolicyType>::Serialize(Ar);
		PixelParameters.Serialize(Ar);
		EmittanceVoxelizationParameters.Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}

	friend FArchive& operator<<(FArchive& Ar, TVXGIVoxelizationShaderPermutationPS*& PS)
	{
		if (!PS && Ar.IsLoading())
		{
			PS = new TVXGIVoxelizationShaderPermutationPS();
		}
		if (PS)
		{
			PS->SerializeBase(Ar, false); //don't store resource again so we pass false
			PS->SetResource(NULL);
		}
		return Ar;
	}
	
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		TVXGIVoxelizationShader<LightMapPolicyType>::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		FEmittanceVoxelizationParameters::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}
};

template<typename LightMapPolicyType>
class TVXGIVoxelizationPS : public TVXGIVoxelizationShader<LightMapPolicyType>
{
	DECLARE_SHADER_TYPE(TVXGIVoxelizationPS, VXGIVoxelizationMeshMaterial);

protected:
	TArray<TRefCountPtr<TVXGIVoxelizationShaderPermutationPS<LightMapPolicyType>>> PermuationShaders;

	/** Initialization constructor. */
	TVXGIVoxelizationPS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) :
		TVXGIVoxelizationShader<LightMapPolicyType>(Initializer), ActualPermuationInUse(NULL)
	{
		const auto& PMaps  = Initializer.Resource->GetParameterMapsForVoxelizationPS();
		PermuationShaders.SetNum(PMaps.Num());
		for (int32 Permutation = 0; Permutation < PermuationShaders.Num(); Permutation++)
		{
			FShaderCompilerOutput FakeOutputWithPermuationParameterMap;
			FakeOutputWithPermuationParameterMap.OutputHash = Initializer.OutputHash;
			FakeOutputWithPermuationParameterMap.Target = Initializer.Target;
			FakeOutputWithPermuationParameterMap.ParameterMap = PMaps[Permutation];
			FMeshMaterialShaderType::CompiledShaderInitializerType InitData(
				&TVXGIVoxelizationShaderPermutationPS<LightMapPolicyType>::StaticType,
				FakeOutputWithPermuationParameterMap,
				Initializer.Resource,
				Initializer.UniformExpressionSet,
				Initializer.MaterialShaderMapHash,
				Initializer.DebugDescription,
				Initializer.VertexFactoryType);
			PermuationShaders[Permutation] = new TVXGIVoxelizationShaderPermutationPS<LightMapPolicyType>(InitData);
		}
		check(PermuationShaders.Num() != 0);
	}
	TVXGIVoxelizationPS() : ActualPermuationInUse(NULL) {}

	//We set this after VXGI gives the one to use
	TVXGIVoxelizationShaderPermutationPS<LightMapPolicyType>* ActualPermuationInUse;

public:
	virtual bool Serialize(FArchive& Ar)
	{
		Ar << PermuationShaders;
		return false;
	}

	void SetActualPixelShaderInUse(FPixelShaderRHIParamRef PixelShader, uint32 Index)
	{
		if (PixelShader)
		{
			ActualPermuationInUse = PermuationShaders[Index];
			ActualPermuationInUse->MyShader = PixelShader; //This is not serialized so set this here
		}
		else
		{
			ActualPermuationInUse = NULL;
		}
	}

	int32 GetNumPermuationShaders()
	{
		return PermuationShaders.Num();
	}

	TVXGIVoxelizationShaderPermutationPS<LightMapPolicyType>* GetActualPermuationInUse() { return ActualPermuationInUse;  }

	void SetParameters(FRHICommandList& RHICmdList, const FMaterialRenderProxy* MaterialRenderProxy, const FMaterial& MaterialResource, const FSceneView& View, ESceneRenderTargetsMode::Type TextureMode)
	{
		ActualPermuationInUse->SetParameters(RHICmdList, MaterialRenderProxy, MaterialResource, View, TextureMode);
	}

	void SetMesh(FRHICommandList& RHICmdList, const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatchElement& BatchElement, float DitheredLODTransitionValue, uint32 DataFlags = 0)
	{
		ActualPermuationInUse->SetMesh(RHICmdList, VertexFactory, View, Proxy, BatchElement, DitheredLODTransitionValue, DataFlags);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		TVXGIVoxelizationShaderPermutationPS<LightMapPolicyType>::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}
};


/**
 * The base shader type for hull shaders.
 */
template<typename LightMapPolicyType>
class TVXGIVoxelizationHS : public FBaseHS
{
	DECLARE_SHADER_TYPE(TVXGIVoxelizationHS,MeshMaterial);

protected:

	TVXGIVoxelizationHS() {}

	TVXGIVoxelizationHS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer):
		FBaseHS(Initializer)
	{}

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return FBaseHS::ShouldCache(Platform, Material, VertexFactoryType) && Material->GetVxgiAllowTesselationDuringVoxelization()
			&& TVXGIVoxelizationShader<LightMapPolicyType>::ShouldCache(Platform,Material,VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		TVXGIVoxelizationShader<LightMapPolicyType>::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}
};

/**
 * The base shader type for Domain shaders.
 */
template<typename LightMapPolicyType>
class TVXGIVoxelizationDS : public FBaseDS
{
	DECLARE_SHADER_TYPE(TVXGIVoxelizationDS,MeshMaterial);

protected:

	TVXGIVoxelizationDS() {}

	TVXGIVoxelizationDS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer):
		FBaseDS(Initializer)
	{
	}

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return FBaseDS::ShouldCache(Platform, Material, VertexFactoryType)  && Material->GetVxgiAllowTesselationDuringVoxelization()
			&& TVXGIVoxelizationShader<LightMapPolicyType>::ShouldCache(Platform,Material,VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		TVXGIVoxelizationShader<LightMapPolicyType>::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}
};


template<typename LightMapPolicyType>
class TVXGIVoxelizationDrawingPolicy : public FMeshDrawingPolicy
{
public:

	/** The data the drawing policy uses for each mesh element. */
	class ElementDataType
	{
	public:

		/** The element's light-map data. */
		typename LightMapPolicyType::ElementDataType LightMapElementData;

		FPrimitiveSceneInfo* StaticMeshPrimitiveSceneInfo;

		/** Default constructor. */
		ElementDataType()
			: StaticMeshPrimitiveSceneInfo(nullptr)
		{}

		/** Initialization constructor. */
		ElementDataType(const typename LightMapPolicyType::ElementDataType& InLightMapElementData, FPrimitiveSceneInfo* InStaticMeshPrimitiveSceneInfo)
			: LightMapElementData(InLightMapElementData)
			, StaticMeshPrimitiveSceneInfo(InStaticMeshPrimitiveSceneInfo)
		{}
	};

	/** Initialization constructor. */
	TVXGIVoxelizationDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		const FMaterial& InMaterialResource,
		LightMapPolicyType InLightMapPolicy
		)
		: FMeshDrawingPolicy(InVertexFactory, InMaterialRenderProxy, InMaterialResource, bOverrideWithShaderComplexity)
		, LightMapPolicy(InLightMapPolicy)
	{
		VertexShader = InMaterialResource.GetShader<TVXGIVoxelizationVS<LightMapPolicyType> >(InVertexFactory->GetType());
		PixelShader = InMaterialResource.GetShader<TVXGIVoxelizationPS<LightMapPolicyType> >(InVertexFactory->GetType());
		check(PixelShader->GetNumPermuationShaders() != 0);

		MatInfo.pixelShader = PixelShader->GetVxgiVoxelizationPixelShaderSet();
		MatInfo.twoSided = InMaterialResource.IsTwoSided();
		MatInfo.omnidirectionalLight = InMaterialResource.IsVxgiOmniDirectional();
		MatInfo.frontCounterClockwise = true;
		MatInfo.voxelizationThickness = InMaterialResource.GetVxgiVoxelizationThickness();
		MatInfo.opacityNoiseScale = InMaterialResource.GetVxgiOpacityNoiseScaleBias().X;
		MatInfo.opacityNoiseBias = InMaterialResource.GetVxgiOpacityNoiseScaleBias().Y;
		MatInfo.proportionalEmittance = InMaterialResource.IsVxgiProportionalEmittance();
		switch(InMaterialResource.GetVxgiMaterialSamplingRate())
		{
		case VXGIMSR_Fixed2X:         MatInfo.materialSamplingRate = VXGI::MaterialSamplingRate::FIXED_2X; break;
		case VXGIMSR_Fixed3X:         MatInfo.materialSamplingRate = VXGI::MaterialSamplingRate::FIXED_3X; break;
		case VXGIMSR_Fixed4X:         MatInfo.materialSamplingRate = VXGI::MaterialSamplingRate::FIXED_4X; break;
		case VXGIMSR_AdaptiveDefault: MatInfo.materialSamplingRate = VXGI::MaterialSamplingRate::ADAPTIVE_DEFAULT; break;
		case VXGIMSR_AdaptiveGE2:     MatInfo.materialSamplingRate = VXGI::MaterialSamplingRate::ADAPTIVE_GE2; break;
		case VXGIMSR_AdaptiveGE4:     MatInfo.materialSamplingRate = VXGI::MaterialSamplingRate::ADAPTIVE_GE4; break;
		default:                      MatInfo.materialSamplingRate = VXGI::MaterialSamplingRate::FIXED_DEFAULT; break;
		}
		

		const EMaterialTessellationMode MaterialTessellationMode = InMaterialResource.GetTessellationMode();

		if (RHISupportsTessellation(GMaxRHIShaderPlatform)
			&& InVertexFactory->GetType()->SupportsTessellationShaders() 
			&& MaterialTessellationMode != MTM_NoTessellation
			&& InMaterialResource.GetVxgiAllowTesselationDuringVoxelization())
		{
			// Find the base pass tessellation shaders since the material is tessellated
			HullShader = InMaterialResource.GetShader<TVXGIVoxelizationHS<LightMapPolicyType> >(VertexFactory->GetType());
			DomainShader = InMaterialResource.GetShader<TVXGIVoxelizationDS<LightMapPolicyType> >(VertexFactory->GetType());
			MatInfo.geometryShader = DomainShader->GetVxgiVoxelizationGeometryShaderSet();
		}
		else
		{
			HullShader = NULL;
			DomainShader = NULL;
			MatInfo.geometryShader = VertexShader->GetVxgiVoxelizationGeometryShaderSet();
		}
	}

	// FMeshDrawingPolicy interface.

	bool Matches(const TVXGIVoxelizationDrawingPolicy& Other) const
	{
		return FMeshDrawingPolicy::Matches(Other) &&
			VertexShader == Other.VertexShader &&
			PixelShader == Other.PixelShader &&
			HullShader == Other.HullShader &&
			DomainShader == Other.DomainShader &&
			LightMapPolicy == Other.LightMapPolicy &&
			MatInfo.pixelShader == Other.MatInfo.pixelShader &&
			MatInfo.geometryShader == Other.MatInfo.geometryShader &&
			MatInfo.materialSamplingRate == Other.MatInfo.materialSamplingRate;
	}

	friend int32 CompareDrawingPolicy(const TVXGIVoxelizationDrawingPolicy& A, const TVXGIVoxelizationDrawingPolicy& B)
	{
		COMPAREDRAWINGPOLICYMEMBERS(VertexShader);
		COMPAREDRAWINGPOLICYMEMBERS(PixelShader);
		COMPAREDRAWINGPOLICYMEMBERS(HullShader);
		COMPAREDRAWINGPOLICYMEMBERS(DomainShader);
		COMPAREDRAWINGPOLICYMEMBERS(VertexFactory);
		COMPAREDRAWINGPOLICYMEMBERS(MaterialRenderProxy);
		COMPAREDRAWINGPOLICYMEMBERS(MatInfo.pixelShader);
		COMPAREDRAWINGPOLICYMEMBERS(MatInfo.geometryShader);
		COMPAREDRAWINGPOLICYMEMBERS(MatInfo.materialSamplingRate);
		return CompareDrawingPolicy(A.LightMapPolicy, B.LightMapPolicy);
	}

	FBoundShaderStateInput GetBoundShaderStateInput(ERHIFeatureLevel::Type InFeatureLevel)
	{
		// Let FDrawingPolicyLink::CreateBoundShaderState create a valid BoundShaderState though we do not use it in this drawing policy.
		return FBoundShaderStateInput(
			FMeshDrawingPolicy::GetVertexDeclaration(),
			VertexShader->GetVertexShader(),
			FHullShaderRHIRef(),
			FDomainShaderRHIRef(),
			FPixelShaderRHIRef(),
			FGeometryShaderRHIRef());
	}

	void SetSharedState(FRHICommandList& RHICmdList, const FSceneView* View, const ContextDataType PolicyContext) const
	{
		SCOPE_CYCLE_COUNTER(STAT_VxgiVoxelizationSetSharedState);

		View->ApplyVoxelizationMaterialInfo(MatInfo, false);

		FGeometryShaderRHIParamRef GeometryShaderInUse = (FGeometryShaderRHIParamRef)View->VxgiDrawCallState.GS.shader;
		FPixelShaderRHIParamRef PixelShaderInUse = (FPixelShaderRHIParamRef)View->VxgiDrawCallState.PS.shader;

		PixelShader->SetActualPixelShaderInUse(PixelShaderInUse, View->VxgiDrawCallState.PS.userDefinedShaderPermutationIndex);

		RHICmdList.SetLocalBoundShaderState(RHICmdList.BuildLocalBoundShaderState(
			VertexFactory->GetDeclaration(),
			VertexShader->GetVertexShader(),
			GETSAFERHISHADER_HULL(HullShader),
			GETSAFERHISHADER_DOMAIN(DomainShader),
			PixelShaderInUse,
			GeometryShaderInUse
			));

		VertexShader->SetParameters(RHICmdList, MaterialRenderProxy, VertexFactory, *MaterialResource, *View, ESceneRenderTargetsMode::SetTextures);
		PixelShader->SetParameters(RHICmdList, MaterialRenderProxy, *MaterialResource, *View, ESceneRenderTargetsMode::SetTextures);
		if (HullShader)
		{
			HullShader->SetParameters(RHICmdList, MaterialRenderProxy, *View);
		}
		if (DomainShader)
		{
			DomainShader->SetParameters(RHICmdList, MaterialRenderProxy, *View);
		}

		LightMapPolicy.Set(RHICmdList, &VertexShader->VertexParameters, &PixelShader->GetActualPermuationInUse()->PixelParameters, VertexShader, PixelShader->GetActualPermuationInUse(), VertexFactory, MaterialRenderProxy, View);

		GDynamicRHI->RHIVXGIApplyDrawStateButNotShaders(View->VxgiDrawCallState);
	}

	void SetMeshRenderState(
		FRHICommandList& RHICmdList,
		const FSceneView& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMeshBatch& Mesh,
		int32 BatchElementIndex,
		bool bBackFace,
		float DitheredLODTransitionValue,
		const ElementDataType& ElementData,
		const ContextDataType PolicyContext
		) const
	{
		SCOPE_CYCLE_COUNTER(STAT_VxgiVoxelizationSetMeshRenderState);

		static const auto CVarForceTwoSided = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.VXGI.ForceTwoSided"));
		static const auto CVarForceFrontCounterClockwise = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.VXGI.ForceFrontCounterClockwise"));

		VXGI::MaterialInfo MeshMatInfo = MatInfo;

		MeshMatInfo.frontCounterClockwise = !((View.bReverseCulling ^ bBackFace) ^ Mesh.ReverseCulling);
		if (CVarForceTwoSided->GetValueOnRenderThread())
		{
			MeshMatInfo.twoSided = true;
		}
		if (CVarForceFrontCounterClockwise->GetValueOnRenderThread())
		{
			MeshMatInfo.frontCounterClockwise = true;
		}

		// All materials in this draw list have the same PS, GS and emittanceCoverageSupersampling
		// So they should all share the same VXGI draw call state.
		// Still, they can have different material parameters (frontCounterClockwise, twoSided, etc).
		bool bStateChanged = View.ApplyVoxelizationMaterialInfo(MeshMatInfo, true);
		check(!bStateChanged);

		// Set the light-map policy's mesh-specific settings.
		LightMapPolicy.SetMesh(
			RHICmdList,
			View,
			PrimitiveSceneProxy,
			&VertexShader->VertexParameters,
			&PixelShader->GetActualPermuationInUse()->PixelParameters,
			VertexShader,
			PixelShader->GetActualPermuationInUse(),
			VertexFactory,
			MaterialRenderProxy,
			ElementData.LightMapElementData);

		const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];
		VertexShader->SetMesh(RHICmdList, VertexFactory, View, PrimitiveSceneProxy, BatchElement, DitheredLODTransitionValue);
		
		bool bEnableEmissiveAndIndirectIrradiance = false;
		
		if (ElementData.StaticMeshPrimitiveSceneInfo)
		{
			bEnableEmissiveAndIndirectIrradiance =
				ElementData.StaticMeshPrimitiveSceneInfo->VxgiLastVoxelizationPass == int32(VXGI::VoxelizationPass::OPACITY) ||
				ElementData.StaticMeshPrimitiveSceneInfo->VxgiLastVoxelizationPass == View.VxgiVoxelizationPass;

			if (bEnableEmissiveAndIndirectIrradiance)
				ElementData.StaticMeshPrimitiveSceneInfo->VxgiLastVoxelizationPass = View.VxgiVoxelizationPass;
		}
		else
		{
			bEnableEmissiveAndIndirectIrradiance = View.VxgiVoxelizationPass == int32(VXGI::VoxelizationPass::EMISSIVE_AND_IRRADIANCE);
		}

		PixelShader->GetActualPermuationInUse()->EmittanceVoxelizationParameters.bEnableEmissiveForMesh = bEnableEmissiveAndIndirectIrradiance && View.VxgiEmittanceVoxelizationArgs.bEnableEmissiveMaterials;
		PixelShader->GetActualPermuationInUse()->EmittanceVoxelizationParameters.bEnableIndirectIrradianceForMesh = bEnableEmissiveAndIndirectIrradiance;
		PixelShader->SetMesh(RHICmdList, VertexFactory, View, PrimitiveSceneProxy, BatchElement, DitheredLODTransitionValue);

		if (HullShader)
		{
			HullShader->SetMesh(RHICmdList, VertexFactory, View, PrimitiveSceneProxy, BatchElement, DitheredLODTransitionValue);
		}
		if (DomainShader)
		{
			DomainShader->SetMesh(RHICmdList, VertexFactory, View, PrimitiveSceneProxy, BatchElement, DitheredLODTransitionValue);
		}

		EmitMeshDrawEvents(RHICmdList, PrimitiveSceneProxy, Mesh);

		RHIAllowTessellation(HullShader && DomainShader);
	}

	void DrawMesh(FRHICommandList& RHICmdList, const FMeshBatch& Mesh, int32 BatchElementIndex) const
	{
		FMeshDrawingPolicy::DrawMesh(RHICmdList, Mesh, BatchElementIndex);

		RHIAllowTessellation(true); //turn it back on
	}

protected:
	TVXGIVoxelizationVS<LightMapPolicyType>* VertexShader;
	TVXGIVoxelizationPS<LightMapPolicyType>* PixelShader;
	TVXGIVoxelizationHS<LightMapPolicyType>* HullShader;
	TVXGIVoxelizationDS<LightMapPolicyType>* DomainShader;
	VXGI::MaterialInfo MatInfo;
	LightMapPolicyType LightMapPolicy;
};

/**
* A drawing policy factory for the base pass drawing policy.
*/
class TVXGIVoxelizationDrawingPolicyFactory
{
public:

	enum { bAllowSimpleElements = true };
	struct ContextType
	{
		VXGI::RenderMeshFilter::Enum MeshFilter;
		ContextType(VXGI::RenderMeshFilter::Enum Filter) : MeshFilter(Filter)
		{}
	};

	static void AddStaticMesh(FRHICommandList& RHICmdList, FScene* Scene, FStaticMesh* StaticMesh);
	static bool DrawDynamicMesh(
		FRHICommandList& RHICmdList,
		const FSceneView& View,
		ContextType DrawingContext,
		const FMeshBatch& Mesh,
		bool bBackFace,
		bool bPreFog,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		FHitProxyId HitProxyId
		);
	
	static bool IsMaterialVoxelized(const FMaterial* Material)
	{
		return (Material->GetMaterialDomain() == MD_Surface 
			&& (Material->GetBlendMode() == EBlendMode::BLEND_Opaque || Material->GetBlendMode() == EBlendMode::BLEND_Masked)
			&& !Material->IsPreviewMaterial()
			&& !Material->IsSpecialEngineMaterial()
			&& Material->IsUsedWithVxgiVoxelization()
			);
	}

	static bool IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy, ERHIFeatureLevel::Type InFeatureLevel)
	{
		if (InFeatureLevel != ERHIFeatureLevel::SM5)
		{
			return true;
		}
		if (MaterialRenderProxy)
		{
			const FMaterial* Material = MaterialRenderProxy->GetMaterial(InFeatureLevel);
			if (!IsMaterialVoxelized(Material))
			{
				return true;
			}
		}
		return false;
	}
};

#endif
// NVCHANGE_END: Add VXGI
