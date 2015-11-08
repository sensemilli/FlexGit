// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
FlexFluidSurfaceRendering.cpp: Flex fluid surface rendering implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "ScreenRendering.h"
#include "SceneFilterRendering.h"
#include "SceneUtils.h"
#include "RenderingCompositionGraph.h"
#include "ParticleHelper.h"
#include "FlexFluidSurfaceSceneProxy.h"
#include "FlexFluidSurfaceRendering.h"
#include "PhysicsEngine/FlexFluidSurfaceComponent.h"
#include "FlexRender.h"

FFlexFluidSurfaceRenderer GFlexFluidSurfaceRenderer;

/*=============================================================================
FAnisotropyResources
=============================================================================*/

class FAnisotropyResources : public FRenderResource
{
public:

	FAnisotropyResources() :
		MaxParticles(0)
	{
	}

	virtual void InitDynamicRHI()
	{
		if (MaxParticles > 0)
		{
			AnisoBuffer1.Initialize(sizeof(FVector4), MaxParticles, PF_A32B32G32R32F, BUF_Volatile);
			AnisoBuffer2.Initialize(sizeof(FVector4), MaxParticles, PF_A32B32G32R32F, BUF_Volatile);
			AnisoBuffer3.Initialize(sizeof(FVector4), MaxParticles, PF_A32B32G32R32F, BUF_Volatile);
		}
	}

	virtual void ReleaseDynamicRHI()
	{
		if (AnisoBuffer1.NumBytes > 0)
		{
			AnisoBuffer1.Release();
			AnisoBuffer2.Release();
			AnisoBuffer3.Release();
		}
	}

	void AllocateFor(int32 InMaxParticles)
	{
		if (InMaxParticles > MaxParticles)
		{
			if (!IsInitialized())
			{
				InitResource();
			}
			MaxParticles = InMaxParticles;
			UpdateRHI();
		}
	}

	int32 MaxParticles;

	FReadBuffer AnisoBuffer1;
	FReadBuffer AnisoBuffer2;
	FReadBuffer AnisoBuffer3;
};

TGlobalResource<FAnisotropyResources> GAnisotropyResources;

/*=============================================================================
FFlexFluidSurfaceSpriteBaseVS
=============================================================================*/

class FFlexFluidSurfaceSpriteBaseVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FFlexFluidSurfaceSpriteBaseVS, MeshMaterial);
	FFlexFluidSurfaceSpriteBaseVS() {}

public:
	FFlexFluidSurfaceSpriteBaseVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FMeshMaterialShader(Initializer)
	{
		ParticleSizeScale.Bind(Initializer.ParameterMap, TEXT("ParticleSizeScale"));
		AnisotropyBuffer1.Bind(Initializer.ParameterMap, TEXT("AnisotropyBuffer1"));
		AnisotropyBuffer2.Bind(Initializer.ParameterMap, TEXT("AnisotropyBuffer2"));
		AnisotropyBuffer3.Bind(Initializer.ParameterMap, TEXT("AnisotropyBuffer3"));
	}

	static bool ShouldCache(EShaderPlatform Platform, const FMaterial* Material, const FVertexFactoryType* VertexFactoryType) { return true; }

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		Ar << ParticleSizeScale;
		Ar << AnisotropyBuffer1;
		Ar << AnisotropyBuffer2;
		Ar << AnisotropyBuffer3;
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(
		FRHICommandList& RHICmdList,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FMaterial& InMaterialResource,
		const FSceneView& View,
		ESceneRenderTargetsMode::Type TextureMode,
		float ParticleScale
		)
	{
		FMeshMaterialShader::SetParameters(RHICmdList, GetVertexShader(), MaterialRenderProxy, InMaterialResource, View, TextureMode);

		if (ParticleSizeScale.IsBound())
		{
			float ParticleSizeScaleValue = ParticleScale;
			SetShaderValue(RHICmdList, GetVertexShader(), ParticleSizeScale, ParticleSizeScaleValue);
		}

		if (AnisotropyBuffer1.IsBound())
		{
			SetSRVParameter(RHICmdList, GetVertexShader(), AnisotropyBuffer1, GAnisotropyResources.AnisoBuffer1.SRV);
		}

		if (AnisotropyBuffer2.IsBound())
		{
			SetSRVParameter(RHICmdList, GetVertexShader(), AnisotropyBuffer2, GAnisotropyResources.AnisoBuffer2.SRV);
		}

		if (AnisotropyBuffer3.IsBound())
		{
			SetSRVParameter(RHICmdList, GetVertexShader(), AnisotropyBuffer3, GAnisotropyResources.AnisoBuffer3.SRV);
		}
	}

	void SetMesh(FRHICommandList& RHICmdList, const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement, float DitheredLODTransitionValue)
	{
		FMeshMaterialShader::SetMesh(RHICmdList, GetVertexShader(),VertexFactory,View,Proxy,BatchElement,DitheredLODTransitionValue);
	}

	FShaderParameter ParticleSizeScale;
	FShaderResourceParameter AnisotropyBuffer1;
	FShaderResourceParameter AnisotropyBuffer2;
	FShaderResourceParameter AnisotropyBuffer3;
};

/*=============================================================================
FFlexFluidSurfaceSpriteSphereVS
=============================================================================*/

class FFlexFluidSurfaceSpriteSphereVS : public FFlexFluidSurfaceSpriteBaseVS
{
	DECLARE_SHADER_TYPE(FFlexFluidSurfaceSpriteSphereVS, MeshMaterial);
	FFlexFluidSurfaceSpriteSphereVS() {}

public:
	FFlexFluidSurfaceSpriteSphereVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FFlexFluidSurfaceSpriteBaseVS(Initializer) {}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FFlexFluidSurfaceSpriteSphereVS, TEXT("FlexFluidSurfaceSpriteVertexShader"), TEXT("SphereMainVS"), SF_Vertex);

/*=============================================================================
FFlexFluidSurfaceSpriteEllipsoidVS
=============================================================================*/

class FFlexFluidSurfaceSpriteEllipsoidVS : public FFlexFluidSurfaceSpriteBaseVS
{
	DECLARE_SHADER_TYPE(FFlexFluidSurfaceSpriteEllipsoidVS, MeshMaterial);
	FFlexFluidSurfaceSpriteEllipsoidVS() {}

public:
	FFlexFluidSurfaceSpriteEllipsoidVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FFlexFluidSurfaceSpriteBaseVS(Initializer) {}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FFlexFluidSurfaceSpriteEllipsoidVS, TEXT("FlexFluidSurfaceSpriteVertexShader"), TEXT("EllipsoidMainVS"), SF_Vertex);

/*=============================================================================
FFlexFluidSurfaceSpriteBasePS
=============================================================================*/

class FFlexFluidSurfaceSpriteBasePS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FFlexFluidSurfaceSpriteBasePS, MeshMaterial);
	FFlexFluidSurfaceSpriteBasePS() {}

public:
	FFlexFluidSurfaceSpriteBasePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FMeshMaterialShader(Initializer)
	{
		ParticleSizeScaleInv.Bind(Initializer.ParameterMap, TEXT("ParticleSizeScaleInv"));
	}

	static bool ShouldCache(EShaderPlatform Platform, const FMaterial* Material, const FVertexFactoryType* VertexFactoryType) { return true; }

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		Ar << ParticleSizeScaleInv;
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(FRHICommandList& RHICmdList, const FMaterialRenderProxy* MaterialRenderProxy, const FMaterial& InMaterialResource,
		const FSceneView& View, ESceneRenderTargetsMode::Type TextureMode, float ParticleScale)
	{
		FMeshMaterialShader::SetParameters(RHICmdList, GetPixelShader(), MaterialRenderProxy, InMaterialResource, View, TextureMode);

		if (ParticleSizeScaleInv.IsBound())
		{
			float ParticleSizeScaleInvValue = 1.0f / ParticleScale;
			SetShaderValue(RHICmdList, GetPixelShader(), ParticleSizeScaleInv, ParticleSizeScaleInvValue);
		}
	}

	void SetMesh(FRHICommandList& RHICmdList, const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement, float DitheredLODTransitionValue)
	{
		FMeshMaterialShader::SetMesh(RHICmdList, GetPixelShader(), VertexFactory, View, Proxy, BatchElement, DitheredLODTransitionValue);
	}

	FShaderParameter ParticleSizeScaleInv;
};

/*=============================================================================
FFlexFluidSurfaceSpriteSphereDepthPS
=============================================================================*/

class FFlexFluidSurfaceSpriteSphereDepthPS : public FFlexFluidSurfaceSpriteBasePS
{
	DECLARE_SHADER_TYPE(FFlexFluidSurfaceSpriteSphereDepthPS, MeshMaterial);
	FFlexFluidSurfaceSpriteSphereDepthPS() {}

public:
	FFlexFluidSurfaceSpriteSphereDepthPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FFlexFluidSurfaceSpriteBasePS(Initializer) {}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FFlexFluidSurfaceSpriteSphereDepthPS, TEXT("FlexFluidSurfaceSpritePixelShader"), TEXT("SphereDepthMainPS"), SF_Pixel);

/*=============================================================================
FFlexFluidSurfaceSpriteEllipsoidDepthPS
=============================================================================*/

class FFlexFluidSurfaceSpriteEllipsoidDepthPS : public FFlexFluidSurfaceSpriteBasePS
{
	DECLARE_SHADER_TYPE(FFlexFluidSurfaceSpriteEllipsoidDepthPS, MeshMaterial);
	FFlexFluidSurfaceSpriteEllipsoidDepthPS() {}

public:
	FFlexFluidSurfaceSpriteEllipsoidDepthPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FFlexFluidSurfaceSpriteBasePS(Initializer) {}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FFlexFluidSurfaceSpriteEllipsoidDepthPS, TEXT("FlexFluidSurfaceSpritePixelShader"), TEXT("EllipsoidDepthMainPS"), SF_Pixel);

/*=============================================================================
FFlexFluidSurfaceSpriteSphereThicknessPS
=============================================================================*/

class FFlexFluidSurfaceSpriteSphereThicknessPS : public FFlexFluidSurfaceSpriteBasePS
{
	DECLARE_SHADER_TYPE(FFlexFluidSurfaceSpriteSphereThicknessPS, MeshMaterial);
	FFlexFluidSurfaceSpriteSphereThicknessPS() {}

public:
	FFlexFluidSurfaceSpriteSphereThicknessPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FFlexFluidSurfaceSpriteBasePS(Initializer) {}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FFlexFluidSurfaceSpriteSphereThicknessPS, TEXT("FlexFluidSurfaceSpritePixelShader"), TEXT("SphereThicknessMainPS"), SF_Pixel);

/*=============================================================================
FFlexFluidSurfaceScreenVS
=============================================================================*/

class FFlexFluidSurfaceScreenVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FFlexFluidSurfaceScreenVS, Global);
	FFlexFluidSurfaceScreenVS() {}

public:
	FFlexFluidSurfaceScreenVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer) {}

	static bool ShouldCache(EShaderPlatform Platform) { return true; }

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View)
	{
		FGlobalShader::SetParameters(RHICmdList, GetVertexShader(), View);
	}
};

IMPLEMENT_SHADER_TYPE(, FFlexFluidSurfaceScreenVS, TEXT("FlexFluidSurfaceScreenShader"), TEXT("ScreenMainVS"), SF_Vertex);

/*=============================================================================
FFlexFluidSurfaceDepthSmoothPS
=============================================================================*/

class FFlexFluidSurfaceDepthSmoothPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FFlexFluidSurfaceDepthSmoothPS, Global);
	FFlexFluidSurfaceDepthSmoothPS() {}

public:
	FFlexFluidSurfaceDepthSmoothPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DepthTexture.Bind(Initializer.ParameterMap, TEXT("FlexFluidSurfaceDepthTexture"), SPF_Mandatory);
		DepthTextureSampler.Bind(Initializer.ParameterMap, TEXT("FlexFluidSurfaceDepthTextureSampler"));
		SmoothScale.Bind(Initializer.ParameterMap, TEXT("SmoothScale"));
		MaxSmoothTexelRadius.Bind(Initializer.ParameterMap, TEXT("MaxSmoothTexelRadius"));
		DepthEdgeFalloff.Bind(Initializer.ParameterMap, TEXT("DepthEdgeFalloff"));
		TexelSize.Bind(Initializer.ParameterMap, TEXT("TexelSize"));
	}

	static bool ShouldCache(EShaderPlatform Platform) { return true; }

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetRenderTargetOutputFormat(0, PF_R32_FLOAT);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DepthTexture;
		Ar << DepthTextureSampler;
		Ar << SmoothScale;
		Ar << MaxSmoothTexelRadius;
		Ar << DepthEdgeFalloff;
		Ar << TexelSize;

		return bShaderHasOutdatedParameters;
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, FFlexFluidSurfaceTextures& Textures, float SmoothingRadius, float MaxRadialSmoothingSamples, float SmothingDepthEdgeFalloff)
	{
		FGlobalShader::SetParameters(RHICmdList, GetPixelShader(), View);

		if (DepthTexture.IsBound())
		{
			FTextureRHIParamRef TextureRHI = Textures.GetDepthTexture();

			SetTextureParameter(RHICmdList, GetPixelShader(), DepthTexture, DepthTextureSampler,
				TStaticSamplerState<SF_Point, AM_Border, AM_Border, AM_Clamp>::GetRHI(), TextureRHI);
		}

		float FOV = PI / 4.0f;
		float AspectRatio = 1.0f;

		if (View.IsPerspectiveProjection())
		{
			// Derive FOV and aspect ratio from the perspective projection matrix
			FOV = FMath::Atan(1.0f / View.ViewMatrices.ProjMatrix.M[0][0]);
			AspectRatio = View.ViewMatrices.ProjMatrix.M[1][1] / View.ViewMatrices.ProjMatrix.M[0][0];
		}

		if (SmoothScale.IsBound())
		{
			// SmoothScale is the factor used to compute the texture space smoothing radius (R[tex])
			// from the world space surface depth (depth[world]) in the smoothing shader like this:
			// R[tex] = SmoothScale / depth[world]
			// 
			// Derivation:
			// R[tex] / textureHeight == R[world] / h[world](depth[world])
			// h[world](depth[world])*0.5 / depth[world] == tan(FOV*0.5)
			// --> SmoothScale == R[world]*textureHeight*0.5 / tan(FOV*0.5)
			float SmoothScaleValue = SmoothingRadius*View.ViewRect.Height()*0.5f / FMath::Tan(FOV*0.5f);
			SetShaderValue(RHICmdList, GetPixelShader(), SmoothScale, SmoothScaleValue);
		}

		if (MaxSmoothTexelRadius.IsBound())
		{
			SetShaderValue(RHICmdList, GetPixelShader(), MaxSmoothTexelRadius, MaxRadialSmoothingSamples);
		}

		if (DepthEdgeFalloff.IsBound())
		{
			SetShaderValue(RHICmdList, GetPixelShader(), DepthEdgeFalloff, SmothingDepthEdgeFalloff);
		}

		if (TexelSize.IsBound())
		{
			const FIntPoint BufferSize = Textures.BufferSize;
			FVector2D TexelSizeVal = FVector2D(1.0f / BufferSize.X, 1.0f / BufferSize.Y);
			SetShaderValue(RHICmdList, GetPixelShader(), TexelSize, TexelSizeVal);
		}
	}

	FShaderResourceParameter DepthTexture;
	FShaderResourceParameter DepthTextureSampler;
	FShaderParameter SmoothScale;
	FShaderParameter MaxSmoothTexelRadius;
	FShaderParameter DepthEdgeFalloff;
	FShaderParameter TexelSize;
};

IMPLEMENT_SHADER_TYPE(, FFlexFluidSurfaceDepthSmoothPS, TEXT("FlexFluidSurfaceScreenShader"), TEXT("DepthSmoothMainPS"), SF_Pixel);

/*=============================================================================
FFlexFluidSurfaceDrawingPolicy, draws the surface with a screen space mesh
=============================================================================*/

class FFlexFluidSurfaceDrawingPolicy : public FMeshDrawingPolicy
{
public:
	/** Initialization constructor. */
	FFlexFluidSurfaceDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		const FMaterial& InMaterialResource,
		ERHIFeatureLevel::Type InFeatureLevel,
		ESceneRenderTargetsMode::Type InSceneTextureMode) :
		FMeshDrawingPolicy(InVertexFactory, InMaterialRenderProxy, InMaterialResource),
		SceneTextureMode(InSceneTextureMode)
	{
		SphereVS = InMaterialResource.GetShader<FFlexFluidSurfaceSpriteSphereVS>(InVertexFactory->GetType());
		EllipsoidVS = InMaterialResource.GetShader<FFlexFluidSurfaceSpriteEllipsoidVS>(InVertexFactory->GetType());
		SphereDepthPS = InMaterialResource.GetShader<FFlexFluidSurfaceSpriteSphereDepthPS>(InVertexFactory->GetType());
		EllipsoidDepthPS = InMaterialResource.GetShader<FFlexFluidSurfaceSpriteEllipsoidDepthPS>(InVertexFactory->GetType());
		SphereThicknessPS = InMaterialResource.GetShader<FFlexFluidSurfaceSpriteSphereThicknessPS>(InVertexFactory->GetType());
	}

	// FMeshDrawingPolicy interface.
	bool Matches(const FFlexFluidSurfaceDrawingPolicy& Other) const
	{
		return FMeshDrawingPolicy::Matches(Other) &&
			SphereVS == Other.SphereVS &&
			EllipsoidVS == Other.EllipsoidVS &&
			SphereDepthPS == Other.SphereDepthPS &&
			EllipsoidDepthPS == Other.EllipsoidDepthPS &&
			SphereThicknessPS == Other.SphereThicknessPS &&
			SceneTextureMode == Other.SceneTextureMode;
	}

	FFlexFluidSurfaceSpriteBaseVS* GetVertexShader(bool bThicknessPass, bool bDrawEllipsoids) const
	{
		if (bThicknessPass || !bDrawEllipsoids)
			return SphereVS;

		return EllipsoidVS;
	}

	FFlexFluidSurfaceSpriteBasePS* GetPixelShader(bool bThicknessPass, bool bDrawEllipsoids) const
	{
		if (bThicknessPass)
			return SphereThicknessPS;

		if (bDrawEllipsoids)
			return EllipsoidDepthPS;

		return SphereDepthPS;
	}

	void SetSharedState(FRHICommandList& RHICmdList, const FViewInfo* View, const ContextDataType PolicyContext,
		bool bThicknessPass, bool bDrawEllipsoids, float ParticleSizeScale) const
	{
		VertexFactory->Set(RHICmdList);

		FFlexFluidSurfaceSpriteBaseVS* BaseVS = GetVertexShader(bThicknessPass, bDrawEllipsoids);
		FFlexFluidSurfaceSpriteBasePS* BasePS = GetPixelShader(bThicknessPass, bDrawEllipsoids);

		BaseVS->SetParameters(RHICmdList, MaterialRenderProxy, *MaterialResource, *View, SceneTextureMode, ParticleSizeScale);
		BasePS->SetParameters(RHICmdList, MaterialRenderProxy, *MaterialResource, *View, SceneTextureMode, ParticleSizeScale);

		FMeshDrawingPolicy::SetSharedState(RHICmdList, View, PolicyContext);
	}

	/**
	* Create bound shader state using the vertex decl from the mesh draw policy
	* as well as the shaders needed to draw the mesh
	* @param DynamicStride - optional stride for dynamic vertex data
	* @return new bound shader state object
	*/
	FBoundShaderStateInput GetBoundShaderStateInput(ERHIFeatureLevel::Type InFeatureLevel, bool bThicknessPass, bool bDrawEllipsoids)
	{
		FFlexFluidSurfaceSpriteBaseVS* BaseVS = GetVertexShader(bThicknessPass, bDrawEllipsoids);
		FFlexFluidSurfaceSpriteBasePS* BasePS = GetPixelShader(bThicknessPass, bDrawEllipsoids);

		return FBoundShaderStateInput(
			FMeshDrawingPolicy::GetVertexDeclaration(),
			BaseVS->GetVertexShader(),
			FHullShaderRHIParamRef(),
			FDomainShaderRHIParamRef(),
			BasePS->GetPixelShader(),
			FGeometryShaderRHIRef());
	}

	void SetMeshRenderState(
		FRHICommandList& RHICmdList,
		const FViewInfo& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMeshBatch& Mesh,
		int32 BatchElementIndex,
		bool bBackFace,
		const ContextDataType PolicyContext,
		bool bThicknessPass,
		bool bDrawEllipsoids) const
	{
		const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];

		FFlexFluidSurfaceSpriteBaseVS* BaseVS = GetVertexShader(bThicknessPass, bDrawEllipsoids);
		FFlexFluidSurfaceSpriteBasePS* BasePS = GetPixelShader(bThicknessPass, bDrawEllipsoids);

		BaseVS->SetMesh(RHICmdList, VertexFactory, View, PrimitiveSceneProxy, BatchElement, 0.0f);
		BasePS->SetMesh(RHICmdList, VertexFactory, View, PrimitiveSceneProxy, BatchElement, 0.0f);

		FMeshDrawingPolicy::SetMeshRenderState(RHICmdList, View, PrimitiveSceneProxy, Mesh, BatchElementIndex, bBackFace, 0.0f, FMeshDrawingPolicy::ElementDataType(), PolicyContext);
	}

	friend int32 CompareDrawingPolicy(const FFlexFluidSurfaceDrawingPolicy& A, const FFlexFluidSurfaceDrawingPolicy& B)
	{
		COMPAREDRAWINGPOLICYMEMBERS(SphereVS);
		COMPAREDRAWINGPOLICYMEMBERS(EllipsoidVS);
		COMPAREDRAWINGPOLICYMEMBERS(SphereDepthPS);
		COMPAREDRAWINGPOLICYMEMBERS(EllipsoidDepthPS);
		COMPAREDRAWINGPOLICYMEMBERS(SphereThicknessPS);

		COMPAREDRAWINGPOLICYMEMBERS(VertexFactory);
		COMPAREDRAWINGPOLICYMEMBERS(MaterialRenderProxy);
		return 0;
	}

protected:
	FFlexFluidSurfaceSpriteSphereVS* SphereVS;
	FFlexFluidSurfaceSpriteEllipsoidVS* EllipsoidVS;
	FFlexFluidSurfaceSpriteSphereDepthPS* SphereDepthPS;
	FFlexFluidSurfaceSpriteEllipsoidDepthPS* EllipsoidDepthPS;
	FFlexFluidSurfaceSpriteSphereThicknessPS* SphereThicknessPS;
	ESceneRenderTargetsMode::Type SceneTextureMode;
};

/*=============================================================================
FFlexFluidSurfaceDrawingPolicyFactory
=============================================================================*/

class FFlexFluidSurfaceDrawingPolicyFactory
{
public:
	enum { bAllowSimpleElements = true };
	struct ContextType
	{
		ESceneRenderTargetsMode::Type TextureMode;
		float ParticleSizeScale;
		bool bThicknessPass;
		bool bDrawEllipsoids;

		ContextType(ESceneRenderTargetsMode::Type InTextureMode, bool bInThicknessPass, bool bInDrawEllipsoids, float InParticleSizeScale)
			: TextureMode(InTextureMode)
			, ParticleSizeScale(InParticleSizeScale)
			, bThicknessPass(bInThicknessPass)
			, bDrawEllipsoids(bInDrawEllipsoids)
		{}
	};

	static void AddStaticMesh(FRHICommandList& RHICmdList, FScene* Scene, FStaticMesh* StaticMesh) {}

	static bool DrawDynamicMesh(
		FRHICommandList& RHICmdList,
		const FViewInfo& View,
		ContextType DrawingContext,
		const FMeshBatch& Mesh,
		bool bBackFace,
		bool bPreFog,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		FHitProxyId HitProxyId
		)
	{
		//Draw depths based on particles
		{
			FFlexFluidSurfaceDrawingPolicy DrawingPolicy(
				Mesh.VertexFactory,
				Mesh.MaterialRenderProxy,
				*Mesh.MaterialRenderProxy->GetMaterial(View.GetFeatureLevel()),
				View.GetFeatureLevel(),
				ESceneRenderTargetsMode::DontSet
				);

			RHICmdList.BuildAndSetLocalBoundShaderState(DrawingPolicy.GetBoundShaderStateInput(View.GetFeatureLevel(),
				DrawingContext.bThicknessPass, DrawingContext.bDrawEllipsoids));

			DrawingPolicy.SetSharedState(RHICmdList, &View, FFlexFluidSurfaceDrawingPolicy::ContextDataType(),
				DrawingContext.bThicknessPass, DrawingContext.bDrawEllipsoids, DrawingContext.ParticleSizeScale);

			check(Mesh.Elements.Num() == 1);
			int BatchElementIndex = 0;
			DrawingPolicy.SetMeshRenderState(RHICmdList, View, PrimitiveSceneProxy, Mesh, BatchElementIndex, bBackFace,
				FFlexFluidSurfaceDrawingPolicy::ContextDataType(), DrawingContext.bThicknessPass, DrawingContext.bDrawEllipsoids);

			DrawingPolicy.DrawMesh(RHICmdList, Mesh, BatchElementIndex);
		}

		return true;
	}

	static bool IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy, ERHIFeatureLevel::Type InFeatureLevel)
	{
		return !MaterialRenderProxy;
	}
};

void AllocateTexturesIfNecessary(FFlexFluidSurfaceTextures& Textures, FIntPoint NewBufferSize)
{
	// Allocate textures if current BufferSize doesn't match up with new one
	if (NewBufferSize != Textures.BufferSize)
	{
		Textures.BufferSize = NewBufferSize;

		// Release old textures
		{
			Textures.Depth.SafeRelease();
			Textures.DepthStencil.SafeRelease();
			Textures.Thickness.SafeRelease();
			Textures.SmoothDepth.SafeRelease();
			GRenderTargetPool.FreeUnusedResources();
		}

		// Alloc new textures
		if (Textures.BufferSize.X > 0 && Textures.BufferSize.Y > 0)
		{
			{
				FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(Textures.BufferSize, PF_R32_FLOAT, FClearValueBinding(FLinearColor(FVector(65536.0f))), TexCreate_None, TexCreate_RenderTargetable, false));
				GRenderTargetPool.FindFreeElement(Desc, Textures.Depth, TEXT("FlexFluidSurfaceDepth"));
			}

			{
				FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(Textures.BufferSize, PF_DepthStencil, FClearValueBinding::DepthFar, TexCreate_None, TexCreate_DepthStencilTargetable, false));
				GRenderTargetPool.FindFreeElement(Desc, Textures.DepthStencil, TEXT("FlexFluidSurfaceDepthStencil"));
			}

			{
				FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(Textures.BufferSize, PF_R32_FLOAT, FClearValueBinding::Black, TexCreate_None, TexCreate_RenderTargetable, false));
				GRenderTargetPool.FindFreeElement(Desc, Textures.Thickness, TEXT("FlexFluidSurfaceThickness"));
			}

			{
				FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(Textures.BufferSize, PF_R32_FLOAT, FClearValueBinding::None, TexCreate_None, TexCreate_RenderTargetable, false));
				GRenderTargetPool.FindFreeElement(Desc, Textures.SmoothDepth, TEXT("FlexFluidSurfaceSmoothDepth"));
			}
		}
	}
}

void ClearTextures(FRHICommandList& RHICmdList, FFlexFluidSurfaceTextures& Textures, const FViewInfo& View)
{
	//Clear depth buffers
	{
		SetRenderTarget(RHICmdList, Textures.GetDepthSurface(), Textures.GetDepthStencilSurface(), ESimpleRenderTargetMode::EClearColorAndDepth);
	}

	//Clear thickness buffer
	{
		SetRenderTarget(RHICmdList, Textures.GetThicknessSurface(), NULL, ESimpleRenderTargetMode::EClearColorExistingDepth);
	}
}

void UpdateAnisotropyBuffers(const FDynamicSpriteEmitterData* EmitterData)
{
	int32 MaxParticleCount = EmitterData->Source.ActiveParticleCount;
	if (EmitterData->Source.MaxDrawCount >= 0 && EmitterData->Source.ActiveParticleCount > EmitterData->Source.MaxDrawCount)
	{
		MaxParticleCount = EmitterData->Source.MaxDrawCount;
	}

	GAnisotropyResources.AllocateFor(MaxParticleCount);

	FVector4* AnisoBuffer1 = (FVector4*)RHILockVertexBuffer(GAnisotropyResources.AnisoBuffer1.Buffer, 0, GAnisotropyResources.AnisoBuffer1.NumBytes, RLM_WriteOnly);
	FVector4* AnisoBuffer2 = (FVector4*)RHILockVertexBuffer(GAnisotropyResources.AnisoBuffer2.Buffer, 0, GAnisotropyResources.AnisoBuffer2.NumBytes, RLM_WriteOnly);
	FVector4* AnisoBuffer3 = (FVector4*)RHILockVertexBuffer(GAnisotropyResources.AnisoBuffer3.Buffer, 0, GAnisotropyResources.AnisoBuffer3.NumBytes, RLM_WriteOnly);

	check(GAnisotropyResources.MaxParticles >= MaxParticleCount);
	for (int32 i = 0; i < MaxParticleCount; i++)
	{
		const uint8* ParticleData = EmitterData->Source.ParticleData.GetData();
		int32 ParticleStride = EmitterData->Source.ParticleStride;
		uint32 ParticleIndex = EmitterData->Source.ParticleIndices[i];

		DECLARE_PARTICLE(Particle, ParticleData + ParticleStride*ParticleIndex);
		verify(EmitterData->Source.FlexDataOffset > 0);

		int32 CurrentOffset = EmitterData->Source.FlexDataOffset;
		const uint8* ParticleBase = (const uint8*)&Particle;
		PARTICLE_ELEMENT(int32, FlexParticleIndex);

		PARTICLE_ELEMENT(FVector, Alignment16);

		PARTICLE_ELEMENT(FVector4, FlexAnisotropy1);
		PARTICLE_ELEMENT(FVector4, FlexAnisotropy2);
		PARTICLE_ELEMENT(FVector4, FlexAnisotropy3);

		AnisoBuffer1[i] = FlexAnisotropy1;
		AnisoBuffer2[i] = FlexAnisotropy2;
		AnisoBuffer3[i] = FlexAnisotropy3;
	}
	RHIUnlockVertexBuffer(GAnisotropyResources.AnisoBuffer1.Buffer);
	RHIUnlockVertexBuffer(GAnisotropyResources.AnisoBuffer2.Buffer);
	RHIUnlockVertexBuffer(GAnisotropyResources.AnisoBuffer3.Buffer);
}

void RenderParticleDepthAndThickness(FRHICommandList& RHICmdList, FFlexFluidSurfaceSceneProxy* SurfaceSceneProxy, const FViewInfo& View)
{
	check(SurfaceSceneProxy && SurfaceSceneProxy->Textures);

	//render surface depth
	{
		SetRenderTarget(RHICmdList, SurfaceSceneProxy->Textures->GetDepthSurface(), SurfaceSceneProxy->Textures->GetDepthStencilSurface(), ESimpleRenderTargetMode::EExistingColorAndDepth);
		// Opaque blending for all G buffer targets, depth tests and writes.
		RHICmdList.SetBlendState(TStaticBlendStateWriteMask<CW_RGBA, CW_RGBA, CW_RGBA, CW_RGBA>::GetRHI());
		// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
		RHICmdList.SetDepthStencilState(TStaticDepthStencilState<true, CF_GreaterEqual>::GetRHI());
		RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1);
		RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
		RHICmdList.SetScissorRect(false, 0, 0, 0, 0);

		for (int i = 0; i < SurfaceSceneProxy->VisibleParticleMeshes.Num(); i++)
		{
			const FSurfaceParticleMesh& ParticleMesh = SurfaceSceneProxy->VisibleParticleMeshes[i];
			
			bool bHasAnisotropy = false;
			if (ParticleMesh.DynamicEmitterData)
			{
				FDynamicSpriteEmitterData* SpriteEmitterData = ((FDynamicSpriteEmitterData*)ParticleMesh.DynamicEmitterData);
				bHasAnisotropy = SpriteEmitterData->Source.bFlexAnisotropyData;

				if (bHasAnisotropy)
				{
					UpdateAnisotropyBuffers(SpriteEmitterData);
				}
			}
			else
			{
				FFlexParticleSceneProxy* SceneProxy = (FFlexParticleSceneProxy*)ParticleMesh.PSysSceneProxy;
				FFlexParticleUserData* UserData = (FFlexParticleUserData*)ParticleMesh.Mesh->Elements[0].UserData;

				if (SceneProxy->Anisotropy1.Num() > 0)
				{
					GAnisotropyResources.AllocateFor(UserData->ParticleCount);

					FVector4* AnisoBuffer1 = (FVector4*)RHILockVertexBuffer(GAnisotropyResources.AnisoBuffer1.Buffer, 0, GAnisotropyResources.AnisoBuffer1.NumBytes, RLM_WriteOnly);
					FVector4* AnisoBuffer2 = (FVector4*)RHILockVertexBuffer(GAnisotropyResources.AnisoBuffer2.Buffer, 0, GAnisotropyResources.AnisoBuffer2.NumBytes, RLM_WriteOnly);
					FVector4* AnisoBuffer3 = (FVector4*)RHILockVertexBuffer(GAnisotropyResources.AnisoBuffer3.Buffer, 0, GAnisotropyResources.AnisoBuffer3.NumBytes, RLM_WriteOnly);

					for (int32 i = 0; i < UserData->ParticleCount; i++)
					{
						AnisoBuffer1[i] = SceneProxy->Anisotropy1[i + UserData->ParticleOffset];
						AnisoBuffer2[i] = SceneProxy->Anisotropy2[i + UserData->ParticleOffset];
						AnisoBuffer3[i] = SceneProxy->Anisotropy3[i + UserData->ParticleOffset];
					}
					RHIUnlockVertexBuffer(GAnisotropyResources.AnisoBuffer1.Buffer);
					RHIUnlockVertexBuffer(GAnisotropyResources.AnisoBuffer2.Buffer);
					RHIUnlockVertexBuffer(GAnisotropyResources.AnisoBuffer3.Buffer);
					bHasAnisotropy = true;
				}
			}

			// Draw screen space surface
			FFlexFluidSurfaceDrawingPolicyFactory::ContextType DrawingContext(ESceneRenderTargetsMode::DontSet, false,
				bHasAnisotropy, SurfaceSceneProxy->DepthParticleScale);

			FFlexFluidSurfaceDrawingPolicyFactory::DrawDynamicMesh(RHICmdList, View, DrawingContext,
				*ParticleMesh.Mesh, false, true, ParticleMesh.PSysSceneProxy, ParticleMesh.Mesh->BatchHitProxyId);
		}
	}

	//render thickness
	if (SurfaceSceneProxy->ThicknessParticleScale > 0.0f)
	{
		FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);
		
		SetRenderTarget(RHICmdList, SurfaceSceneProxy->Textures->GetThicknessSurface(), SceneContext.GetSceneDepthSurface(), ESimpleRenderTargetMode::EExistingColorAndDepth);
		RHICmdList.SetBlendState(TStaticBlendState<CW_RGB, BO_Add, BF_One, BF_One>::GetRHI());
		// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
		RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_GreaterEqual>::GetRHI());
		RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1);
		RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
		RHICmdList.SetScissorRect(false, 0, 0, 0, 0);

		// Draw screen space surface. Maybe the right solution is to extend the FBasePassOpaqueDrawingPolicy instead
		FFlexFluidSurfaceDrawingPolicyFactory::ContextType DrawingContext(ESceneRenderTargetsMode::DontSet, true,
			false, SurfaceSceneProxy->ThicknessParticleScale);

		for (int i = 0; i < SurfaceSceneProxy->VisibleParticleMeshes.Num(); i++)
		{
			const FSurfaceParticleMesh& ParticleMesh = SurfaceSceneProxy->VisibleParticleMeshes[i];
			FFlexFluidSurfaceDrawingPolicyFactory::DrawDynamicMesh(RHICmdList, View, DrawingContext,
				*ParticleMesh.Mesh, false, true, ParticleMesh.PSysSceneProxy, ParticleMesh.Mesh->BatchHitProxyId);
		}
	}

	GRenderTargetPool.VisualizeTexture.SetCheckPoint(RHICmdList, SurfaceSceneProxy->Textures->Depth);
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(RHICmdList, SurfaceSceneProxy->Textures->DepthStencil);
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(RHICmdList, SurfaceSceneProxy->Textures->Thickness);

	//No particle emitter data needed anymore
	SurfaceSceneProxy->ClearDynamicEmitterData_RenderThread();
}

void SmoothDepth(FRHICommandList& RHICmdList, const FViewInfo& View, FFlexFluidSurfaceSceneProxy* SurfaceSceneProxy)
{
	check(SurfaceSceneProxy && SurfaceSceneProxy->Textures)

	//Clear depth stencil to 0.0: reversed Z depth surface (0=far, 1=near).
	SetRenderTarget(RHICmdList, SurfaceSceneProxy->Textures->GetSmoothDepthSurface(), FTextureRHIRef(), ESimpleRenderTargetMode::EUninitializedColorAndDepth);

	auto ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());

	RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());
	RHICmdList.SetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
	RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1);
	RHICmdList.SetScissorRect(false, 0, 0, 0, 0);

	TShaderMapRef<FFlexFluidSurfaceScreenVS> VertexShader(ShaderMap);
	TShaderMapRef<FFlexFluidSurfaceDepthSmoothPS> PixelShader(ShaderMap);
	static FGlobalBoundShaderState BoundShaderState;
	SetGlobalBoundShaderState(RHICmdList, View.GetFeatureLevel(), BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	PixelShader->SetParameters(RHICmdList, View, *SurfaceSceneProxy->Textures,
		SurfaceSceneProxy->SmoothingRadius, SurfaceSceneProxy->MaxRadialSamples, SurfaceSceneProxy->DepthEdgeFalloff);

	DrawRectangle(
		RHICmdList,
		0, 0,
		View.ViewRect.Width(), View.ViewRect.Height(),
		0, 0,
		View.ViewRect.Width(), View.ViewRect.Height(),
		FIntPoint(View.ViewRect.Width(), View.ViewRect.Height()),
		SurfaceSceneProxy->Textures->BufferSize,
		*VertexShader,
		EDRF_UseTriangleOptimization);

	GRenderTargetPool.VisualizeTexture.SetCheckPoint(RHICmdList, SurfaceSceneProxy->Textures->SmoothDepth);
}

/*=============================================================================
FFlexFluidSurfaceRenderer
=============================================================================*/

void FFlexFluidSurfaceRenderer::UpdateProxiesAndResources(FRHICommandList& RHICmdList, TArray<FMeshBatchAndRelevance, SceneRenderingAllocator>& DynamicMeshElements)
{
	//refresh SurfaceSceneProxies from DynamicMeshElements
	SurfaceSceneProxies.Empty(SurfaceSceneProxies.Num());
	for (int32 MeshBatchIndex = 0; MeshBatchIndex < DynamicMeshElements.Num(); MeshBatchIndex++)
	{
		const FMeshBatchAndRelevance& MeshBatchAndRelevance = DynamicMeshElements[MeshBatchIndex];
		if (MeshBatchAndRelevance.PrimitiveSceneProxy->IsFlexFluidSurface())
		{
			SurfaceSceneProxies.Add((FFlexFluidSurfaceSceneProxy*)MeshBatchAndRelevance.PrimitiveSceneProxy);
		}
	}

	//for each FFlexFluidSurfaceSceneProxy, get all corresponding particle system proxies and allocate textures if neccessary
	for (int32 i = 0; i < SurfaceSceneProxies.Num(); i++)
	{
		FFlexFluidSurfaceSceneProxy* Proxy = SurfaceSceneProxies[i];
		if (Proxy && Proxy->SurfaceMaterial)
		{
			Proxy->VisibleParticleMeshes.Empty(Proxy->VisibleParticleMeshes.Num());

			for (int32 MeshBatchIndex = 0; MeshBatchIndex < DynamicMeshElements.Num(); MeshBatchIndex++)
			{
				const FMeshBatchAndRelevance& MeshBatchAndRelevance = DynamicMeshElements[MeshBatchIndex];

				if (!MeshBatchAndRelevance.Mesh->bFlexFluidParticles)
				{
					FDynamicEmitterDataBase* DynamicEmitterData = NULL;
					for (int32 EmitterIndex = 0; EmitterIndex < Proxy->EmitterCount; EmitterIndex++)
					{
						FParticleSystemSceneProxy* PSysSceneProxy = Proxy->ParticleSystemSceneProxyArray[EmitterIndex];
						if (MeshBatchAndRelevance.PrimitiveSceneProxy == PSysSceneProxy)
						{
							for (int32 CandidateIndex = 0; CandidateIndex < PSysSceneProxy->GetDynamicData()->DynamicEmitterDataArray.Num(); CandidateIndex++)
							{
								if (PSysSceneProxy->GetDynamicData()->DynamicEmitterDataArray[CandidateIndex] == Proxy->DynamicEmitterDataArray[EmitterIndex])
								{
									DynamicEmitterData = Proxy->DynamicEmitterDataArray[EmitterIndex];
								}
							}
						}
					}

					if (DynamicEmitterData)
					{
						FSurfaceParticleMesh ParticleMesh;
						ParticleMesh.DynamicEmitterData = DynamicEmitterData;
						ParticleMesh.PSysSceneProxy = MeshBatchAndRelevance.PrimitiveSceneProxy;
						ParticleMesh.Mesh = MeshBatchAndRelevance.Mesh;

						Proxy->VisibleParticleMeshes.Add(ParticleMesh);
					}
				}
				else
				{
					FSurfaceParticleMesh ParticleMesh;
					ParticleMesh.DynamicEmitterData = nullptr;
					ParticleMesh.PSysSceneProxy = MeshBatchAndRelevance.PrimitiveSceneProxy;
					ParticleMesh.Mesh = MeshBatchAndRelevance.Mesh;

					Proxy->VisibleParticleMeshes.Add(ParticleMesh);
				}
			}

			FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);
			AllocateTexturesIfNecessary(*Proxy->Textures, SceneContext.GetBufferSizeXY());
		}
	}

}

void FFlexFluidSurfaceRenderer::RenderParticles(FRHICommandList& RHICmdList, const FViewInfo& View)
{
	for (int32 i = 0; i < SurfaceSceneProxies.Num(); i++)
	{
		FFlexFluidSurfaceSceneProxy* Proxy = SurfaceSceneProxies[i];
		if (Proxy && Proxy->SurfaceMaterial)
		{
			ClearTextures(RHICmdList, *Proxy->Textures, View);
			RenderParticleDepthAndThickness(RHICmdList, Proxy, View);
			SmoothDepth(RHICmdList, View, Proxy);
		}
	}
}

void FFlexFluidSurfaceRenderer::RenderBasePass(FRHICommandList& RHICmdList, const FViewInfo& View)
{
	for (int32 i = 0; i < SurfaceSceneProxies.Num(); i++)
	{
		FFlexFluidSurfaceSceneProxy* Proxy = SurfaceSceneProxies[i];

		if (Proxy && Proxy->SurfaceMaterial)
		{
			EBlendMode BlendMode = Proxy->SurfaceMaterial->GetBlendMode();

			if (BlendMode == BLEND_Opaque || BlendMode == BLEND_Masked)
			{
				//SetupBasePassView in DeferredShadingRenderer.cpp
				{
					// Opaque blending for all G buffer targets, depth tests and writes.
					RHICmdList.SetBlendState(TStaticBlendStateWriteMask<CW_RGBA, CW_RGBA, CW_RGBA, CW_RGBA>::GetRHI());
					// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
					RHICmdList.SetDepthStencilState(TStaticDepthStencilState<true, CF_GreaterEqual>::GetRHI());
					RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1);
					RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
					RHICmdList.SetScissorRect(false, 0, 0, 0, 0);
				}

				FBasePassOpaqueDrawingPolicyFactory::ContextType DrawingContext(false, ESceneRenderTargetsMode::DontSet);
				FBasePassOpaqueDrawingPolicyFactory::DrawDynamicMesh(RHICmdList, View, DrawingContext, *Proxy->MeshBatch, false, true, Proxy, Proxy->MeshBatch->BatchHitProxyId);
			}
		}
	}
}

bool FFlexFluidSurfaceRenderer::IsDepthMaskingRequired(const FPrimitiveSceneProxy* SceneProxy)
{
	if (SceneProxy->IsFlexFluidSurface())
	{
		return true;
	}

	if (!SceneProxy->IsOftenMoving() || !SceneProxy->CastsDynamicShadow())
	{
		return false;
	}

	for (int32 i = 0; i < SurfaceSceneProxies.Num(); i++)
	{
		FFlexFluidSurfaceSceneProxy* SurfaceProxy = SurfaceSceneProxies[i];
		if (SurfaceProxy && SurfaceProxy->SurfaceMaterial)
		{
			for (int32 i = 0; i < SurfaceProxy->VisibleParticleMeshes.Num(); i++)
			{
				if (SurfaceProxy->VisibleParticleMeshes[i].PSysSceneProxy == SceneProxy)
				{
					return true;
				}
			}
		}
	}
	return false;
}

void FFlexFluidSurfaceRenderer::RenderDepth(FRHICommandList& RHICmdList, FPrimitiveSceneProxy* SceneProxy, const FViewInfo& View)
{
	if (SceneProxy->IsFlexFluidSurface())
	{
		FFlexFluidSurfaceSceneProxy* SurfaceProxy = (FFlexFluidSurfaceSceneProxy*)SceneProxy;
		if (SurfaceProxy && SurfaceProxy->SurfaceMaterial)
		{
			EBlendMode BlendMode = SurfaceProxy->SurfaceMaterial->GetBlendMode();

			//TODO fix shadowing for translucent
			if (BlendMode == BLEND_Translucent)
				return;

			FDepthDrawingPolicyFactory::ContextType DrawingContext(DDM_AllOccluders);
			FDepthDrawingPolicyFactory::DrawDynamicMesh(RHICmdList, View, DrawingContext, *SurfaceProxy->MeshBatch,
				false, true, SurfaceProxy, SurfaceProxy->MeshBatch->BatchHitProxyId);
		}
	}
}

void FFlexFluidSurfaceRenderer::Cleanup()
{
	SurfaceSceneProxies.Empty(SurfaceSceneProxies.Num());
}
