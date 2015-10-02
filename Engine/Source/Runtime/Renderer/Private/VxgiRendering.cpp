// Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.

#include "RendererPrivate.h"

// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI

#include "VxgiRendering.h"
#include "DeferredShadingRenderer.h"
#include "ScreenRendering.h"
#include "SceneFilterRendering.h"
#include "SceneUtils.h"

static TAutoConsoleVariable<int32> CVarVxgiMapSize(
	TEXT("r.VXGI.MapSize"),
	128,
	TEXT("Size of every VXGI clipmap level, in voxels.\n")
	TEXT("Valid values are 16, 32, 64, 128, 256."),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiStackLevels(
	TEXT("r.VXGI.StackLevels"),
	5,
	TEXT("Number of clip stack levels in VXGI clipmap (1-5)."),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiOpacity6D(
	TEXT("r.VXGI.Opacity6D"),
	1,
	TEXT("Whether to use 6 opacity projections per voxel.\n")
	TEXT("0: 3 projections, 1: 6 projections"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiEmittance6D(
	TEXT("r.VXGI.Emittance6D"),
	1,
	TEXT("Whether to use 6 emittance projections per voxel.\n")
	TEXT("0: 3 projections, 1: 6 projections"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiNvidiaExtensionsEnable(
	TEXT("r.VXGI.NvidiaExtensionsEnable"),
	1,
	TEXT("Controls the use of NVIDIA specific D3D extensions by VXGI.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiGSPassthroughEnable(
    TEXT("r.VXGI.GSPassthroughEnable"),
    1,
    TEXT("Enables the use of Maxwell Geometry Shader Pass-Through feature for voxelization.\n")
    TEXT("Only effective when r.VXGI.NvidiaExtensionsEnable = 1.\n")
    TEXT("Sometimes pass-through shaders do not work properly (like wrong parts of emissive objects emit light)\n")
    TEXT("while other Maxwell features do, so this flag is to work around the issues at a small performance cost.")
    TEXT("0: Disable, 1: Enable"),
    ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiStoreEmittanceInHdrFormat(
	TEXT("r.VXGI.StoreEmittanceInHdrFormat"),
	1,
	TEXT("Sets the format of VXGI emittance voxel textures.\n")
	TEXT("0: UNORM8, 1: FP16 (on Maxwell) or FP32 (on other GPUs)."),
	ECVF_Cheat);

static TAutoConsoleVariable<float> CVarVxgiEmittanceStorageScale(
	TEXT("r.VXGI.EmittanceStorageScale"),
	1.0f,
	TEXT("Multiplier for the values stored in VXGI emittance textures (any value greater than 0).\n")
	TEXT("If you observe emittance clamping (e.g. white voxels on colored objects)\n") 
	TEXT("or quantization (color distortion in dim areas), try to change this parameter."),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiEmittanceInterpolationEnable(
	TEXT("r.VXGI.EmittanceInterpolationEnable"),
	0,
	TEXT("Whether to interpolate between downsampled and directly voxelized emittance in coarse levels of detail.\n")
	TEXT("Sometimes this interpolation makes illumination smoother when the camera moves.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiHighQualityEmittanceDownsamplingEnable(
	TEXT("r.VXGI.HighQualityEmittanceDownsamplingEnable"),
	0,
	TEXT("Whether to use a larger triangular filter for emittance downsampling.\n")
	TEXT("This filter improves stability of indirect lighting caused by moving objects, but has a negative effect on performance.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiDiffuseTracingEnable(
	TEXT("r.VXGI.DiffuseTracingEnable"),
	1,
	TEXT("Whether to enable VXGI indirect lighting.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiSpecularTracingEnable(
	TEXT("r.VXGI.SpecularTracingEnable"),
	1,
	TEXT("Whether to enable VXGI reflections.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiTemporalReprojectionEnable(
	TEXT("r.VXGI.TemporalReprojectionEnable"),
	1,
	TEXT("Whether to enable temporal reprojection.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiMultiBounceEnable(
	TEXT("r.VXGI.MultiBounceEnable"),
	0,
	TEXT("Whether to enable multi-bounce diffuse VXGI.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiMultiBounceNormalizationEnable(
	TEXT("r.VXGI.MultiBounceNormalizationEnable"),
	1,
	TEXT("Whether to try preventing the indirect irradiance from blowing up exponentially due to high feedback.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

static TAutoConsoleVariable<float> CVarVxgiRange(
	TEXT("r.VXGI.Range"),
	400.0f,
	TEXT("Size of the finest clipmap level, in world units."),
	ECVF_Cheat);

static TAutoConsoleVariable<float> CVarVxgiViewOffsetScale(
	TEXT("r.VXGI.ViewOffsetScale"),
	1.f,
	TEXT("Scale factor for the distance between the camera and the VXGI clipmap anchor point"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiDiffuseMaterialsEnable(
	TEXT("r.VXGI.DiffuseMaterialsEnable"),
	1,
	TEXT("Whether to include diffuse lighting in the VXGI voxelized emittance.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiEmissiveMaterialsEnable(
	TEXT("r.VXGI.EmissiveMaterialsEnable"),
	1,
	TEXT("Whether to include emissive materials in the VXGI voxelized emittance.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiEmittanceShadingMode(
	TEXT("r.VXGI.EmittanceShadingMode"),
	0,
	TEXT("0: Use DiffuseColor = BaseColor - BaseColor * Metallic")
	TEXT("1: Use DiffuseColor = BaseColor"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiEmittanceShadowEnable(
	TEXT("r.VXGI.EmittanceShadowEnable"),
	1,
	TEXT("[Debug] Whether to enable the emittance shadow term.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiEmittanceShadowCascade(
	TEXT("r.VXGI.EmittanceShadowCascade"),
	-1,
	TEXT("[Debug] Restrict the emittance shadowing to a single cascade.\n")
	TEXT("<0: Use all cascades. Otherwise the index of the cascade to use."),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiEmittanceShadowQuality(
	TEXT("r.VXGI.EmittanceShadowQuality"),
	1,
	TEXT("0: no filtering\n")
	TEXT("1: 2x2 samples"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiDebugClipmapLevel(
	TEXT("r.VXGI.DebugClipmapLevel"),
	15,
	TEXT("Current clipmap level visualized (for the opacity and emittance debug modes).\n")
	TEXT("15: visualize all levels at once"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiDebugVoxelsToSkip(
	TEXT("r.VXGI.DebugVoxelsToSkip"),
	0,
	TEXT("Number of initial voxels to skip in the ray casting if r.VXGI.DebugMode != 0"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiDebugBlendOutput(
	TEXT("r.VXGI.DebugBlendOutput"),
	0,
	TEXT("Alpha blend debug output\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiCompositingMode(
	TEXT("r.VXGI.CompositingMode"),
	0,
	TEXT("0: add the VXGI diffuse result over the UE lighting using additive blending (default)\n")
	TEXT("1: visualize the VXGI indirect lighting only, with no albedo and no AO\n")
	TEXT("2: visualize the direct lighting only"),
	ECVF_Cheat);

static TAutoConsoleVariable<float> CVarVxgiRoughnessOverride(
	TEXT("r.VXGI.RoughnessOverride"),
	0.f,
	TEXT("Override the GBuffer roughness"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiAmbientOcclusionMode(
	TEXT("r.VXGI.AmbientOcclusionMode"),
	0,
	TEXT("0: Default\n")
	TEXT("1: Replace lighting with Voxel AO"),
	ECVF_Cheat);

static TAutoConsoleVariable<float> CVarVxgiAmbientOcclusionScale(
	TEXT("r.VXGI.AmbientOcclusionScale"),
	1.f,
	TEXT("Scale factor for the Voxel AO result"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarVxgiForceTwoSided(TEXT("r.VXGI.ForceTwoSided"), 0, TEXT(""), ECVF_Cheat);
static TAutoConsoleVariable<int32> CVarVxgiForceFrontCounterClockwise(TEXT("r.VXGI.ForceFrontCounterClockwise"), 0, TEXT(""), ECVF_Cheat);
static TAutoConsoleVariable<int32> CVarVxgiForceDisableTonemapper(TEXT("r.VXGI.ForceDisableTonemapper"), 0, TEXT(""), ECVF_Cheat);

// With reverse infinite projections, the near plane is at Z=1 and the far plane is at Z=0.
// The VXGI library uses these 2 values along with the ViewProjMatrix to compute the ray directions.
#define VXGI_HARDWARE_DEPTH_NEAR 1.f
#define VXGI_HARDWARE_DEPTH_FAR 0.f

void FDeferredShadingSceneRenderer::InitVxgiRenderingState(const FSceneViewFamily* InViewFamily)
{
	bVxgiPerformOpacityVoxelization = false;
	bVxgiPerformEmittanceVoxelization = false;

	//This must be done on the game thread
	const auto& PrimaryView = *(InViewFamily->Views[0]);
	bVxgiDebugRendering = ViewFamily.EngineShowFlags.VxgiOpacityVoxels || ViewFamily.EngineShowFlags.VxgiEmittanceVoxels || ViewFamily.EngineShowFlags.VxgiIrradianceVoxels;
	VxgiRange = CVarVxgiRange.GetValueOnGameThread();
	VxgiAnchorPoint = PrimaryView.ViewMatrices.ViewOrigin + PrimaryView.GetViewDirection() * VxgiRange * CVarVxgiViewOffsetScale.GetValueOnGameThread();

	bVxgiUseDiffuseMaterials = !!CVarVxgiDiffuseMaterialsEnable.GetValueOnGameThread();
	bVxgiUseEmissiveMaterials = !!CVarVxgiEmissiveMaterialsEnable.GetValueOnGameThread();
	bVxgiTemporalReprojectionEnable = !!CVarVxgiTemporalReprojectionEnable.GetValueOnGameThread();
	bVxgiAmbientOcclusionMode = !!CVarVxgiAmbientOcclusionMode.GetValueOnGameThread();
	bVxgiMultiBounceEnable = !bVxgiAmbientOcclusionMode && !!CVarVxgiMultiBounceEnable.GetValueOnGameThread();
}

bool FDeferredShadingSceneRenderer::IsVxgiEnabled(const FViewInfo& View)
{
	if (!View.State)
	{
		return false; //some editor panel or something
	}

	if (!View.IsPerspectiveProjection())
	{
		return false;
	}

	if (bVxgiDebugRendering)
	{
		return true;
	}

	const auto& PostSettings = View.FinalPostProcessSettings;
	return PostSettings.VxgiDiffuseTracingEnabled || PostSettings.VxgiSpecularTracingEnabled;
}

bool FDeferredShadingSceneRenderer::IsVxgiEnabled()
{
	check(Views.Num() > 0);
	const FViewInfo& PrimaryView = Views[0];
	return IsVxgiEnabled(PrimaryView);
}

void FDeferredShadingSceneRenderer::SetVxgiVoxelizationParameters(VXGI::VoxelizationParameters& Params)
{
	Params.mapSize = CVarVxgiMapSize.GetValueOnRenderThread();
	Params.stackLevels = CVarVxgiStackLevels.GetValueOnRenderThread();
	Params.allocationMapLodBias = uint32(FMath::Max(2 - int(Params.stackLevels), (Params.mapSize == 256) ? 1 : 0));
	Params.indirectIrradianceMapLodBias = Params.allocationMapLodBias;
	Params.mipLevels = log2(Params.mapSize) - 2;
	Params.persistentVoxelData = false;
	Params.opacityDirectionCount = (CVarVxgiOpacity6D.GetValueOnRenderThread() != 0)
		? VXGI::OpacityDirections::SIX_DIMENSIONAL
		: VXGI::OpacityDirections::THREE_DIMENSIONAL;
	Params.enableNvidiaExtensions = !!CVarVxgiNvidiaExtensionsEnable.GetValueOnRenderThread();
    Params.enableGeometryShaderPassthrough = !!CVarVxgiGSPassthroughEnable.GetValueOnRenderThread();
	Params.emittanceFormat = bVxgiAmbientOcclusionMode
		? VXGI::EmittanceFormat::NONE
		: (CVarVxgiStoreEmittanceInHdrFormat.GetValueOnRenderThread() != 0)
			? VXGI::EmittanceFormat::QUALITY
			: VXGI::EmittanceFormat::UNORM8;
	Params.emittanceStorageScale = CVarVxgiEmittanceStorageScale.GetValueOnRenderThread();
	Params.useEmittanceInterpolation = !!CVarVxgiEmittanceInterpolationEnable.GetValueOnRenderThread();
	Params.useHighQualityEmittanceDownsampling = !!CVarVxgiHighQualityEmittanceDownsamplingEnable.GetValueOnRenderThread();
	Params.enableMultiBounce = bVxgiMultiBounceEnable;
}

void FDeferredShadingSceneRenderer::PrepareForVxgiOpacityVoxelization(FRHICommandList& RHICmdList)
{
	VXGI::IGlobalIllumination* VxgiInterface = GDynamicRHI->RHIVXGIGetInterface();
	auto Status = VXGI::VFX_VXGI_VerifyInterfaceVersion();
	check(VXGI_SUCCEEDED(Status));

	SCOPED_DRAW_EVENT(RHICmdList, VXGIPrepareForVxgiOpacityVoxelization);

	{
		const VXGI::Box3f VxgiBox = VxgiInterface->calculateHypotheticalWorldRegion(VXGI::Vector3f(VxgiAnchorPoint.X, VxgiAnchorPoint.Y, VxgiAnchorPoint.Z), VxgiRange);
		VxgiClipmapBounds = FBoxSphereBounds(FBox(VxgiBox));

		VXGI::UpdateVoxelizationParameters Parameters;
		Parameters.clipmapAnchor = VXGI::Vector3f(VxgiAnchorPoint.X, VxgiAnchorPoint.Y, VxgiAnchorPoint.Z);
		Parameters.sceneExtents = GetVxgiWorldSpaceSceneBounds();
		Parameters.giRange = VxgiRange;
		Parameters.indirectIrradianceMapTracingParameters.irradianceScale = Views[0].FinalPostProcessSettings.VxgiMultiBounceIrradianceScale;
		Parameters.indirectIrradianceMapTracingParameters.useAutoNormalization = !!CVarVxgiMultiBounceNormalizationEnable.GetValueOnRenderThread();

		Status = VxgiInterface->prepareForOpacityVoxelization(
			Parameters,
			bVxgiPerformOpacityVoxelization,
			bVxgiPerformEmittanceVoxelization);

		check(VXGI_SUCCEEDED(Status));
	}

	{
		Status = VxgiInterface->getVoxelizationViewMatrix(VxgiViewMatrix);
		check(VXGI_SUCCEEDED(Status));
	}
}

void FDeferredShadingSceneRenderer::PrepareForVxgiEmittanceVoxelization(FRHICommandList& RHICmdList)
{
	SCOPED_DRAW_EVENT(RHICmdList, VXGI);

	{
		SCOPED_DRAW_EVENT(RHICmdList, PrepareForVxgiEmittanceVoxelization);

		auto Status = GDynamicRHI->RHIVXGIGetInterface()->prepareForEmittanceVoxelization();
		check(VXGI_SUCCEEDED(Status));
	}
}

void FDeferredShadingSceneRenderer::VoxelizeVxgiOpacity(FRHICommandList& RHICmdList)
{
	SCOPE_CYCLE_COUNTER(STAT_VxgiVoxelizeOpacity);
	SCOPED_DRAW_EVENT(RHICmdList, VXGIOpacity);

	VXGI::EmittanceVoxelizationArgs Args;
	RenderForVxgiVoxelization(RHICmdList, VXGI::VoxelizationPass::OPACITY, Args);
}

void FDeferredShadingSceneRenderer::VoxelizeVxgiEmissiveAndIndirectIrradiance(FRHICommandList& RHICmdList)
{
	SCOPE_CYCLE_COUNTER(STAT_VxgiVoxelizeEmissiveAndIndirectIrradiance);
	SCOPED_DRAW_EVENT(RHICmdList, VXGIEmissiveAndIndirectIrradiance);

	VXGI::EmittanceVoxelizationArgs Args;
	RenderForVxgiVoxelization(RHICmdList, VXGI::VoxelizationPass::EMISSIVE_AND_IRRADIANCE, Args);
}

void FDeferredShadingSceneRenderer::InitializeVxgiVoxelization(FRHICommandList& RHICmdList)
{
	// Fill the voxelization params structure to latch the console vars, specifically AmbientOcclusionMode
	SetVxgiVoxelizationParameters(VxgiVoxelizationParameters);

	// Clamp the parameters first because they might affect the output of IsVxgiEnabled
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		EndVxgiFinalPostProcessSettings(Views[ViewIndex].FinalPostProcessSettings, VxgiVoxelizationParameters);
	}

	if (!IsVxgiEnabled())
	{
		return;
	}

	// Reset the VxgiLastVoxelizationPass values for all primitives
	for (int32 Index = 0; Index < Scene->Primitives.Num(); ++Index)
	{
		FPrimitiveSceneInfo* PrimitiveSceneInfo = Scene->Primitives[Index];
		PrimitiveSceneInfo->VxgiLastVoxelizationPass = VXGI::VoxelizationPass::OPACITY;
	}

	GDynamicRHI->RHIVXGISetVoxelizationParameters(VxgiVoxelizationParameters);

	PrepareForVxgiOpacityVoxelization(RHICmdList);

	if (bVxgiPerformOpacityVoxelization)
	{
		VoxelizeVxgiOpacity(RHICmdList);
	}

	if (bVxgiPerformEmittanceVoxelization)
	{
		PrepareForVxgiEmittanceVoxelization(RHICmdList);
	}
}

void FDeferredShadingSceneRenderer::FinalizeVxgiVoxelization(FRHICommandList& RHICmdList)
{
	if (!IsVxgiEnabled())
	{
		return;
	}

	if (bVxgiPerformEmittanceVoxelization)
	{
		VoxelizeVxgiEmissiveAndIndirectIrradiance(RHICmdList);
	}

	SCOPED_DRAW_EVENT(RHICmdList, VXGIFinalizeVxgiVoxelization);

	auto Status = GDynamicRHI->RHIVXGIGetInterface()->finalizeVoxelization();
	check(VXGI_SUCCEEDED(Status));
}

void FDeferredShadingSceneRenderer::EndVxgiFinalPostProcessSettings(FFinalPostProcessSettings& FinalPostProcessSettings, const VXGI::VoxelizationParameters& VParams)
{
	if (!CVarVxgiDiffuseTracingEnable.GetValueOnRenderThread() || !ViewFamily.EngineShowFlags.VxgiDiffuse)
	{
		FinalPostProcessSettings.VxgiDiffuseTracingEnabled = false;
	}
	if (!CVarVxgiSpecularTracingEnable.GetValueOnRenderThread() || !ViewFamily.EngineShowFlags.VxgiSpecular)
	{
		FinalPostProcessSettings.VxgiSpecularTracingEnabled = false;
	}
	if (!bVxgiTemporalReprojectionEnable)
	{
		FinalPostProcessSettings.bVxgiDiffuseTracingTemporalReprojectionEnabled = false;
	}

	switch (CVarVxgiCompositingMode.GetValueOnRenderThread())
	{
	case 1: // Indirect Diffuse Only
		FinalPostProcessSettings.VxgiDiffuseTracingEnabled = true;
		FinalPostProcessSettings.VxgiSpecularTracingEnabled = false;
		FinalPostProcessSettings.ScreenSpaceReflectionIntensity = 0.f;
		break;
	case 2: // Direct Only
		FinalPostProcessSettings.VxgiDiffuseTracingEnabled = false;
		FinalPostProcessSettings.VxgiSpecularTracingEnabled = false;
		FinalPostProcessSettings.ScreenSpaceReflectionIntensity = 0.f;
		break;
	}

	if (VParams.emittanceFormat == VXGI::EmittanceFormat::NONE)
	{
		// Ambient occlusion mode

		FinalPostProcessSettings.VxgiDiffuseTracingIntensity = 0.f;
		FinalPostProcessSettings.VxgiSpecularTracingIntensity = 0.f;
		FinalPostProcessSettings.VxgiSpecularTracingEnabled = false;

		const float A = CVarVxgiAmbientOcclusionScale.GetValueOnRenderThread();
		FinalPostProcessSettings.VxgiDiffuseTracingAmbientColor = FLinearColor(A, A, A);
	}
}

void FDeferredShadingSceneRenderer::RenderForVxgiVoxelization(
	FRHICommandList& RHICmdList,
	int32 VoxelizationPass,
	const VXGI::EmittanceVoxelizationArgs& Args)
{
	if (Args.LightSceneInfo && !Args.LightSceneInfo->Proxy->CastVxgiIndirectLighting())
	{
		return;
	}

	VXGI::RenderMeshFilter::Enum RenderFilter = VXGI::RenderMeshFilter::ALL_MESHES;

	if (VoxelizationPass == VXGI::VoxelizationPass::EMISSIVE_AND_IRRADIANCE)
	{
		if (!bVxgiMultiBounceEnable)
		{
			if (!bVxgiUseEmissiveMaterials)
				return;

			RenderFilter = VXGI::RenderMeshFilter::ONLY_EMISSIVE;
		}
	}

	SCOPED_DRAW_EVENT(RHICmdList, VXGIVoxelization);

	FSceneViewInitOptions ViewInitOptions;
	ViewInitOptions.ViewFamily = &ViewFamily;
	ViewInitOptions.SetViewRectangle(FIntRect(0, 0, 1, 1));
	
	FMatrix ViewMatrix;
	FMemory::Memcpy(&ViewMatrix, &VxgiViewMatrix, sizeof(FMatrix));
	ViewInitOptions.ProjectionMatrix = FMatrix::Identity;

	ViewInitOptions.ViewOrigin = ViewMatrix.GetOrigin();
	ViewInitOptions.ViewRotationMatrix = ViewMatrix;

	TArray<FViewInfo> VxgiViews;
	VxgiViews.Empty();
	VxgiViews.Add(FViewInfo(ViewInitOptions));
	auto& View = VxgiViews[0];

	{
		SCOPE_CYCLE_COUNTER(STAT_VxgiVoxelizationVisibility);

		View.FinalPostProcessSettings.AntiAliasingMethod = AAM_None; //Turn off temporal AA jitter
		View.bIsVxgiVoxelization = true;
		View.bDisableDistanceBasedFadeTransitions = true;
		View.VxgiEmittanceVoxelizationArgs = Args;
		View.VxgiEmittanceVoxelizationArgs.bEnableEmissiveMaterials = bVxgiUseEmissiveMaterials;
		View.VxgiVoxelizationPass = VoxelizationPass;
		View.InitRHIResources(nullptr);

		FRHICommandListImmediate& RHICmdListImmediate = FRHICommandListExecutor::GetImmediateCommandList(); 
		PreVisibilityFrameSetup(RHICmdListImmediate, &VxgiViews);
		ComputeViewVisibility(RHICmdListImmediate, &VxgiViews);
		PostVisibilityFrameSetup(&VxgiViews);
	}

	VXGI::IGlobalIllumination* VxgiInterface = GDynamicRHI->RHIVXGIGetInterface();
	VxgiInterface->beginVoxelizationDrawCallGroup();

	{
		SCOPE_CYCLE_COUNTER(STAT_VxgiVoxelizationStaticGeometry);
		SCOPED_DRAW_EVENT(RHICmdList, StaticGeometry);

		for (int32 ListType = 0; ListType < FScene::EVoxelizationPass_MAX; ListType++)
		{
			if (RenderFilter == VXGI::RenderMeshFilter::ONLY_EMISSIVE && ListType == FScene::EVoxelizationPass_Default)
			{
				continue;
			}

			Scene->VxgiVoxelizationNoLightMapDrawList[ListType].DrawVisible(RHICmdList, View, View.StaticMeshVisibilityMap, View.StaticMeshBatchVisibility);
		}
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_VxgiVoxelizationDynamicGeometry);
		SCOPED_DRAW_EVENT(RHICmdList, DynamicGeometry);

		TVXGIVoxelizationDrawingPolicyFactory::ContextType Context(RenderFilter);

		// Copied from FDeferredShadingSceneRenderer::RenderBasePassDynamicData
		for (int32 MeshBatchIndex = 0; MeshBatchIndex < View.DynamicMeshElements.Num(); MeshBatchIndex++)
		{
			const FMeshBatchAndRelevance& MeshBatchAndRelevance = View.DynamicMeshElements[MeshBatchIndex];

			if ((MeshBatchAndRelevance.bHasOpaqueOrMaskedMaterial || ViewFamily.EngineShowFlags.Wireframe)
				&& MeshBatchAndRelevance.bRenderInMainPass)
			{
				const FMeshBatch& MeshBatch = *MeshBatchAndRelevance.Mesh;
				TVXGIVoxelizationDrawingPolicyFactory::DrawDynamicMesh(RHICmdList, View, Context, MeshBatch, false, true, MeshBatchAndRelevance.PrimitiveSceneProxy, MeshBatch.BatchHitProxyId);
			}
		}

		View.SimpleElementCollector.DrawBatchedElements(RHICmdList, View, FTexture2DRHIRef(), EBlendModeFilter::OpaqueAndMasked);

		if (!View.Family->EngineShowFlags.CompositeEditorPrimitives)
		{
			bool bDirty = false;
			bDirty = DrawViewElements<TVXGIVoxelizationDrawingPolicyFactory>(RHICmdList, View, Context, SDPG_World, true) || bDirty;
			bDirty = DrawViewElements<TVXGIVoxelizationDrawingPolicyFactory>(RHICmdList, View, Context, SDPG_Foreground, true) || bDirty;
		}
	}

	VxgiInterface->endVoxelizationDrawCallGroup();

	RHICmdList.VXGICleanupAfterVoxelization();
}

VXGI::Box3f FDeferredShadingSceneRenderer::GetVxgiWorldSpaceSceneBounds()
{
	FVector SceneMinPos = FVector(FLT_MAX, FLT_MAX, FLT_MAX);
	FVector SceneMaxPos = FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	for (int32 Index = 0; Index < Scene->PrimitiveBounds.Num(); ++Index)
	{
		const FPrimitiveSceneInfo* PrimitiveSceneInfo = Scene->Primitives[Index];
		const FString OwnerName = PrimitiveSceneInfo->Proxy->GetOwnerName().ToString();
		const FPrimitiveBounds& Bounds = Scene->PrimitiveBounds[Index];

		const FVector MinPoint = Bounds.Origin - Bounds.BoxExtent;
		const FVector MaxPoint = Bounds.Origin + Bounds.BoxExtent;
		SceneMinPos = SceneMinPos.ComponentMin(MinPoint);
		SceneMaxPos = SceneMaxPos.ComponentMax(MaxPoint);
	}
	VXGI::Box3f SceneExtents(VXGI::Vector3f(&SceneMinPos.X), VXGI::Vector3f(&SceneMaxPos.X));
	return SceneExtents;
}

void FDeferredShadingSceneRenderer::SetVxgiDiffuseTracingParameters(const FViewInfo& View, VXGI::DiffuseTracingParameters &TracingParams)
{
	const auto& PostSettings = View.FinalPostProcessSettings;
	
	TracingParams.irradianceScale = PostSettings.VxgiDiffuseTracingIntensity;
	TracingParams.numCones = PostSettings.VxgiDiffuseTracingNumCones;
	TracingParams.autoConeAngle = !!PostSettings.bVxgiDiffuseTracingAutoAngle;
	TracingParams.tracingSparsity = PostSettings.VxgiDiffuseTracingSparsity;
	TracingParams.coneAngle = PostSettings.VxgiDiffuseTracingConeAngle;
	TracingParams.enableConeRotation = !!PostSettings.bVxgiDiffuseTracingConeRotation;
	TracingParams.enableRandomConeOffsets = !!PostSettings.bVxgiDiffuseTracingRandomConeOffsets;
	TracingParams.coneNormalGroupingFactor = PostSettings.VxgiDiffuseTracingConeNormalGroupingFactor;
	TracingParams.maxSamples = PostSettings.VxgiDiffuseTracingMaxSamples;
	TracingParams.tracingStep = PostSettings.VxgiDiffuseTracingStep;
	TracingParams.opacityCorrectionFactor = PostSettings.VxgiDiffuseTracingOpacityCorrectionFactor;
	TracingParams.normalOffsetFactor = PostSettings.VxgiDiffuseTracingNormalOffsetFactor;
	TracingParams.ambientRange = PostSettings.VxgiDiffuseTracingAmbientRange;
	TracingParams.environmentMapTint = VXGI::Vector3f(
		PostSettings.VxgiDiffuseTracingEnvironmentMapTint.R,
		PostSettings.VxgiDiffuseTracingEnvironmentMapTint.G,
		PostSettings.VxgiDiffuseTracingEnvironmentMapTint.B);
	TracingParams.flipOpacityDirections = false;
	TracingParams.initialOffsetBias = PostSettings.VxgiDiffuseTracingInitialOffsetBias;
	TracingParams.initialOffsetDistanceFactor = PostSettings.VxgiDiffuseTracingInitialOffsetDistanceFactor;
	TracingParams.enableScreenSpaceCorrection = false;
	TracingParams.nearClipZ = VXGI_HARDWARE_DEPTH_NEAR;
	TracingParams.farClipZ = VXGI_HARDWARE_DEPTH_FAR;
	TracingParams.enableTemporalReprojection = !!PostSettings.bVxgiDiffuseTracingTemporalReprojectionEnabled;
	TracingParams.temporalReprojectionWeight = PostSettings.VxgiDiffuseTracingTemporalReprojectionPreviousFrameWeight;
	TracingParams.temporalReprojectionMaxDistanceInVoxels = PostSettings.VxgiDiffuseTracingTemporalReprojectionMaxDistanceInVoxels;
	TracingParams.temporalReprojectionNormalWeightExponent = PostSettings.VxgiDiffuseTracingTemporalReprojectionNormalWeightExponent;

	TracingParams.ambientColor = VXGI::Vector3f(
		PostSettings.VxgiDiffuseTracingAmbientColor.R,
		PostSettings.VxgiDiffuseTracingAmbientColor.G,
		PostSettings.VxgiDiffuseTracingAmbientColor.B);
	
			
	if (View.FinalPostProcessSettings.VxgiDiffuseTracingEnvironmentMap && View.FinalPostProcessSettings.VxgiDiffuseTracingEnvironmentMap->Resource)
	{
		FTextureResource* TextureResource = View.FinalPostProcessSettings.VxgiDiffuseTracingEnvironmentMap->Resource;
		FRHITexture* Texture = TextureResource->TextureRHI->GetTextureCube();
		TracingParams.environmentMap = GDynamicRHI->GetVXGITextureFromRHI(Texture);
	} 
}

void FDeferredShadingSceneRenderer::SetVxgiSpecularTracingParameters(const FViewInfo& View, VXGI::SpecularTracingParameters &TracingParams)
{
	const auto& PostSettings = View.FinalPostProcessSettings;

	TracingParams.irradianceScale = PostSettings.VxgiSpecularTracingIntensity;
	TracingParams.maxSamples = PostSettings.VxgiSpecularTracingMaxSamples;
	TracingParams.tracingStep = PostSettings.VxgiSpecularTracingTracingStep;
	TracingParams.opacityCorrectionFactor = PostSettings.VxgiSpecularTracingOpacityCorrectionFactor;
	TracingParams.flipOpacityDirections = false;
	TracingParams.initialOffsetBias = PostSettings.VxgiSpecularTracingInitialOffsetBias;
	TracingParams.initialOffsetDistanceFactor = PostSettings.VxgiSpecularTracingInitialOffsetDistanceFactor;
	TracingParams.enableScreenSpaceCorrection = false;
	TracingParams.environmentMapTint = VXGI::Vector3f(
		PostSettings.VxgiSpecularTracingEnvironmentMapTint.R,
		PostSettings.VxgiSpecularTracingEnvironmentMapTint.G,
		PostSettings.VxgiSpecularTracingEnvironmentMapTint.B);
	TracingParams.nearClipZ = VXGI_HARDWARE_DEPTH_NEAR;
	TracingParams.farClipZ = VXGI_HARDWARE_DEPTH_FAR;
	TracingParams.tangentJitterScale = PostSettings.VxgiSpecularTracingTangentJitterScale;

	switch(PostSettings.VxgiSpecularTracingFilter)
	{
	case VXGISTF_Temporal:      TracingParams.filter = VXGI::SpecularTracingParameters::FILTER_TEMPORAL; break;
	case VXGISTF_Simple:        TracingParams.filter = VXGI::SpecularTracingParameters::FILTER_SIMPLE; break;
	case VXGISTF_EdgeFollowing: TracingParams.filter = VXGI::SpecularTracingParameters::FILTER_EDGE; break;
	default:                    TracingParams.filter = VXGI::SpecularTracingParameters::FILTER_NONE; break;
	}
	
	if (View.FinalPostProcessSettings.VxgiSpecularTracingEnvironmentMap && View.FinalPostProcessSettings.VxgiSpecularTracingEnvironmentMap->Resource)
	{
		FTextureResource* TextureResource = View.FinalPostProcessSettings.VxgiSpecularTracingEnvironmentMap->Resource;
		FRHITexture* Texture = TextureResource->TextureRHI->GetTextureCube();
		TracingParams.environmentMap = GDynamicRHI->GetVXGITextureFromRHI(Texture);
	} 
}

void FDeferredShadingSceneRenderer::SetVxgiInputBuffers(FSceneRenderTargets& SceneContext, const FViewInfo& View, VXGI::IViewTracer::InputBuffers& InputBuffers, VXGI::IViewTracer::InputBuffers& InputBuffersPreviousFrame)
{
	InputBuffers.gbufferDepth = SceneContext.GetCurrentVxgiSceneDepthTextureHandle();
	InputBuffers.gbufferNormal = SceneContext.GetCurrentVxgiNormalAndRoughnessTextureHandle();

	FMemory::Memcpy(&InputBuffers.viewMatrix, &View.ViewMatrices.ViewMatrix, sizeof(VXGI::Matrix4f));
	FMemory::Memcpy(&InputBuffers.projMatrix, &View.ViewMatrices.ProjMatrix, sizeof(VXGI::Matrix4f));

	InputBuffers.gbufferViewport = NVRHI::Viewport(
		(float)View.ViewRect.Min.X, (float)View.ViewRect.Max.X,
		(float)View.ViewRect.Min.Y, (float)View.ViewRect.Max.Y, 0.f, 1.0f);

	// VXGI uses N = FetchedNormal.xyz * Scale + Bias
	InputBuffers.gbufferNormalScale = 1.f;
	InputBuffers.gbufferNormalBias = 0.f;

	InputBuffersPreviousFrame = InputBuffers;
	InputBuffersPreviousFrame.gbufferDepth = SceneContext.GetPreviousVxgiSceneDepthTextureHandle();
	InputBuffersPreviousFrame.gbufferNormal = SceneContext.GetPreviousVxgiNormalAndRoughnessTextureHandle();

	FMemory::Memcpy(&InputBuffersPreviousFrame.viewMatrix, &View.PrevViewMatrices.ViewMatrix, sizeof(VXGI::Matrix4f));
	FMemory::Memcpy(&InputBuffersPreviousFrame.projMatrix, &View.PrevViewMatrices.ProjMatrix, sizeof(VXGI::Matrix4f));
}

class FComposeVxgiGBufferPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FComposeVxgiGBufferPS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	/** Default constructor. */
	FComposeVxgiGBufferPS() {}

	FDeferredPixelShaderParameters DeferredParameters;
public:
	// we need:  specular intensity in albedo.w and specular roughness in normal.w

	/** Initialization constructor. */
	FComposeVxgiGBufferPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View)
	{
		FGlobalShader::SetParameters(RHICmdList, GetPixelShader(), View);
		DeferredParameters.Set(RHICmdList, GetPixelShader(), View);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(, FComposeVxgiGBufferPS, TEXT("VXGICompositing"), TEXT("ComposeVxgiGBufferPS"), SF_Pixel);

void FDeferredShadingSceneRenderer::PrepareVxgiGBuffer(FRHICommandListImmediate& RHICmdList, const FViewInfo& View)
{
	if (!IsVxgiEnabled(View) || bVxgiDebugRendering)
	{
		return;
	}

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

	// we need  specular roughness in normal.w
	SCOPED_DRAW_EVENT(RHICmdList, PrepareTracingInputs);

	// set the state
	RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());
	RHICmdList.SetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());

	TShaderMapRef<FScreenVS> VertexShader(View.ShaderMap);
	TShaderMapRef<FComposeVxgiGBufferPS> ComposeVxgiGBufferPS(View.ShaderMap);

	static FGlobalBoundShaderState BoundShaderState0;
	SetGlobalBoundShaderState(RHICmdList, FeatureLevel, BoundShaderState0, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *ComposeVxgiGBufferPS);

	FTextureRHIParamRef Targets[] = { SceneContext.VxgiNormalAndRoughness->GetRenderTargetItem().TargetableTexture };
	SetRenderTargets(RHICmdList, 1, Targets, NULL, 0, NULL);

	RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

	VertexShader->SetParameters(RHICmdList, View);
	ComposeVxgiGBufferPS->SetParameters(RHICmdList, View);

	// Draw a quad mapping scene color to the view's render target
	DrawRectangle(
		RHICmdList,
		0, 0,
		View.ViewRect.Width(), View.ViewRect.Height(),
		View.ViewRect.Min.X, View.ViewRect.Min.Y,
		View.ViewRect.Width(), View.ViewRect.Height(),
		FIntPoint(View.ViewRect.Width(), View.ViewRect.Height()),
		SceneContext.GetBufferSizeXY(),
		*VertexShader);
}

void FDeferredShadingSceneRenderer::RenderVxgiTracing(FRHICommandListImmediate& RHICmdList, const FViewInfo& View)
{
	if (!IsVxgiEnabled(View) || bVxgiDebugRendering)
	{
		return;
	}

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

	VXGI::IViewTracer* VxgiViewTracer = ((FSceneViewState*)View.State)->GetVXGITracer();
	VXGI::IGlobalIllumination* VxgiInterface = GDynamicRHI->RHIVXGIGetInterface();

	VXGI::IViewTracer::InputBuffers InputBuffers, InputBuffersPreviousFrame;
	SetVxgiInputBuffers(SceneContext, View, InputBuffers, InputBuffersPreviousFrame);

	SCOPED_DRAW_EVENT(RHICmdList, VXGITracing);

	{
		SCOPED_DRAW_EVENT(RHICmdList, DiffuseConeTracing);
		NVRHI::TextureHandle IlluminationDiffuseHandle = NULL;

		VXGI::DiffuseTracingParameters DiffuseTracingParams;
		SetVxgiDiffuseTracingParameters(View, DiffuseTracingParams);

		if (View.FinalPostProcessSettings.VxgiDiffuseTracingEnabled)
		{
			auto Status = VxgiViewTracer->computeDiffuseChannel(DiffuseTracingParams, IlluminationDiffuseHandle, InputBuffers, !View.bPrevTransformsReset ? &InputBuffersPreviousFrame : NULL);
			check(VXGI_SUCCEEDED(Status));
		}

		FTextureRHIRef IlluminationDiffuse(GDynamicRHI->GetRHITextureFromVXGI(IlluminationDiffuseHandle));
		if (IlluminationDiffuse)
		{
			SceneContext.VxgiOutputDiffuse[View.VxgiViewIndex] = IlluminationDiffuse->GetTexture2D();
		}
	}

	{
		SCOPED_DRAW_EVENT(RHICmdList, SpecularConeTracing);
		NVRHI::TextureHandle IlluminationSpecHandle = NULL;

		VXGI::SpecularTracingParameters SpecularTracingParams;
		SetVxgiSpecularTracingParameters(View, SpecularTracingParams);

		if (View.FinalPostProcessSettings.VxgiSpecularTracingEnabled)
		{
			auto Status = VxgiViewTracer->computeSpecularChannel(SpecularTracingParams, IlluminationSpecHandle, InputBuffers, !View.bPrevTransformsReset ? &InputBuffersPreviousFrame : NULL);
			check(VXGI_SUCCEEDED(Status));
		}

		FTextureRHIRef IlluminationSpec(GDynamicRHI->GetRHITextureFromVXGI(IlluminationSpecHandle));
		if (IlluminationSpec)
		{
			SceneContext.VxgiOutputSpec[View.VxgiViewIndex] = IlluminationSpec->GetTexture2D();
		}
	}
}

void FDeferredShadingSceneRenderer::RenderVxgiDebug(FRHICommandListImmediate& RHICmdList, const FViewInfo& View)
{
	if (!IsVxgiEnabled(View))
	{
		return;
	}

	VXGI::DebugRenderParameters Params;

	if (ViewFamily.EngineShowFlags.VxgiOpacityVoxels)
		Params.debugMode = VXGI::DebugRenderMode::OPACITY_TEXTURE;
	else if (ViewFamily.EngineShowFlags.VxgiEmittanceVoxels)
		Params.debugMode = VXGI::DebugRenderMode::EMITTANCE_TEXTURE;
	else if (ViewFamily.EngineShowFlags.VxgiIrradianceVoxels)
		Params.debugMode = VXGI::DebugRenderMode::INDIRECT_IRRADIANCE_TEXTURE;
	else
		return;


	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

	SCOPED_DRAW_EVENT(RHICmdList, VXGI);

	VXGI::IGlobalIllumination* VxgiInterface = GDynamicRHI->RHIVXGIGetInterface();


	// With reverse infinite projections, the near plane is at Z=1 and the far plane is at Z=0
	// The lib uses these 2 values along with the ViewProjMatrix to compute the ray directions
	const float nearClipZ = 1.0f;
	const float farClipZ = 0.0f;

	Params.viewport = NVRHI::Viewport(View.ViewRect.Min.X, View.ViewRect.Max.X, View.ViewRect.Min.Y, View.ViewRect.Max.Y, 0.f, 1.f);

	Params.destinationTexture = GDynamicRHI->GetVXGITextureFromRHI((FRHITexture*)SceneContext.GetSceneColorSurface().GetReference());
	
	SCOPED_DRAW_EVENT(RHICmdList, RenderDebug);

	const int32 BlendDebug = CVarVxgiDebugBlendOutput.GetValueOnRenderThread();

	FTexture2DRHIRef DepthTarget = SceneContext.GetSceneDepthSurface();
	SetRenderTarget(RHICmdList, SceneContext.GetSceneColorSurface(), DepthTarget);
	RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);
	RHICmdList.Clear(!BlendDebug, FColor(0, 0, 0, 0), true, farClipZ, false, 0, FIntRect()); //clear it so multi-pass debug mode works
	Params.destinationDepth = GDynamicRHI->GetVXGITextureFromRHI((FRHITexture*)DepthTarget.GetReference());

	VXGI::Matrix4f ViewMatrix, ProjMatrix;
	FMemory::Memcpy(&Params.viewMatrix, &View.ViewMatrices.ViewMatrix, sizeof(VXGI::Matrix4f));
	FMemory::Memcpy(&Params.projMatrix, &View.ViewMatrices.ProjMatrix, sizeof(VXGI::Matrix4f));

	if (Params.debugMode == VXGI::DebugRenderMode::OPACITY_TEXTURE || Params.debugMode == VXGI::DebugRenderMode::EMITTANCE_TEXTURE)
	{
		Params.level = CVarVxgiDebugClipmapLevel.GetValueOnRenderThread();
		Params.level = FMath::Min(Params.level, VxgiVoxelizationParameters.stackLevels * 2 + VxgiVoxelizationParameters.mipLevels);
	}
	else
	{
		Params.level = 0;
	}

	Params.bitToDisplay = 0;
	Params.voxelsToSkip = CVarVxgiDebugVoxelsToSkip.GetValueOnRenderThread();
	Params.nearClipZ = nearClipZ;
	Params.farClipZ = farClipZ;

	Params.depthStencilState.depthEnable = true;
	Params.depthStencilState.depthFunc = NVRHI::DepthStencilState::COMPARISON_GREATER;

	auto Status = VxgiInterface->renderDebug(Params);

	check(VXGI_SUCCEEDED(Status));
}

/** Encapsulates the post processing ambient occlusion pixel shader. */
template<bool bRawDiffuse>
class FAddVxgiDiffusePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FAddVxgiDiffusePS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	/** Default constructor. */
	FAddVxgiDiffusePS() {}

	FDeferredPixelShaderParameters DeferredParameters;

public:
	/** Initialization constructor. */
	FAddVxgiDiffusePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View)
	{
		FGlobalShader::SetParameters(RHICmdList, GetPixelShader(), View);
		DeferredParameters.Set(RHICmdList, GetPixelShader(), View);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		return bShaderHasOutdatedParameters;
	}
};

typedef FAddVxgiDiffusePS<false> FAddVxgiCompositedDiffusePS;
typedef FAddVxgiDiffusePS<true>  FAddVxgiRawDiffusePS;

IMPLEMENT_SHADER_TYPE(, FAddVxgiCompositedDiffusePS, TEXT("VXGICompositing"), TEXT("AddVxgiDiffusePS"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(, FAddVxgiRawDiffusePS, TEXT("VXGICompositing"), TEXT("AddVxgiRawDiffusePS"), SF_Pixel);

void FDeferredShadingSceneRenderer::CompositeVxgiDiffuseTracing(FRHICommandListImmediate& RHICmdList, const FViewInfo& View)
{
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

	//Make sure this after tracing always
	if (IsValidRef(SceneContext.VxgiOutputDiffuse[View.VxgiViewIndex])) //if it's on and we outputted something
	{
		SCOPED_DRAW_EVENT(RHICmdList, VXGICompositeDiffuse);

		SceneContext.BeginRenderingSceneColor(RHICmdList);

		//Blend in the results
		const FSceneRenderTargetItem& DestRenderTarget = SceneContext.GetSceneColor()->GetRenderTargetItem();
		SetRenderTarget(RHICmdList, DestRenderTarget.TargetableTexture, FTextureRHIRef());

		RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);
		RHICmdList.SetRasterizerState(TStaticRasterizerState<>::GetRHI());
		RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());

		TShaderMapRef<FScreenVS> VertexShader(View.ShaderMap);

		if (CVarVxgiCompositingMode.GetValueOnRenderThread() || bVxgiAmbientOcclusionMode)
		{
			RHICmdList.SetBlendState(TStaticBlendState<CW_RGBA>::GetRHI());
			TShaderMapRef<FAddVxgiRawDiffusePS> PixelShader(View.ShaderMap);

			static FGlobalBoundShaderState BoundShaderState;
			SetGlobalBoundShaderState(RHICmdList, FeatureLevel, BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

			PixelShader->SetParameters(RHICmdList, View);
		}
		else
		{
			RHICmdList.SetBlendState(TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI());
			TShaderMapRef<FAddVxgiCompositedDiffusePS> PixelShader(View.ShaderMap);

			static FGlobalBoundShaderState BoundShaderState;
			SetGlobalBoundShaderState(RHICmdList, FeatureLevel, BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

			PixelShader->SetParameters(RHICmdList, View);
		}

		VertexShader->SetParameters(RHICmdList, View);

		// Draw a quad mapping scene color to the view's render target
		DrawRectangle(
			RHICmdList,
			0, 0,
			View.ViewRect.Width(), View.ViewRect.Height(),
			View.ViewRect.Min.X, View.ViewRect.Min.Y,
			View.ViewRect.Width(), View.ViewRect.Height(),
			View.ViewRect.Size(),
			SceneContext.GetBufferSizeXY(),
			*VertexShader);

		SceneContext.FinishRenderingSceneColor(RHICmdList, false);
	}
}

void FVXGIVoxelizationNoLightMapPolicy::PixelParametersType::Bind(const FShaderParameterMap& ParameterMap)
{
}

void FVXGIVoxelizationNoLightMapPolicy::PixelParametersType::Serialize(FArchive& Ar)
{
}

bool FVXGIVoxelizationNoLightMapPolicy::ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
{
	return true;
}

void FVXGIVoxelizationNoLightMapPolicy::ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
{
	OutEnvironment.SetDefine(TEXT("SIMPLE_DYNAMIC_LIGHTING"),TEXT("1"));
	FNoLightMapPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
}

void FVXGIVoxelizationNoLightMapPolicy::SetMesh(
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
	) const
{
	FNoLightMapPolicy::SetMesh(RHICmdList, View, PrimitiveSceneProxy, VertexShaderParameters, PixelShaderParameters, VertexShader, PixelShader, VertexFactory, MaterialRenderProxy, ElementData);
}

//Override this and add the VXGI internal hash to trigger recompiles if the version changes
const FSHAHash& FVXGIVoxelizationMeshMaterialShaderType::GetSourceHash() const
{
	if (HashWithVXGIHash == FSHAHash())
	{
		FSHA1 HashState;
		{
			FSHAHash FileHash = FMeshMaterialShaderType::GetSourceHash();
			HashState.Update(&FileHash.Hash[0], sizeof(FileHash.Hash));
		}
		{
			LoadVxgiModule(); //Might not be loaded with a no-op RHI
			auto Status = VXGI::VFX_VXGI_VerifyInterfaceVersion();
			check(VXGI_SUCCEEDED(Status));
			uint64_t VXGIHash = VXGI::VFX_VXGI_GetInternalShaderHash();
			UnloadVxgiModule(); //Do we want to bother unloading? Might be slower if we get called a bunch of times
			HashState.Update((uint8*)&VXGIHash, sizeof(VXGIHash));
		}
		HashState.Final();
		HashState.GetHash(&HashWithVXGIHash.Hash[0]);
	}
	return HashWithVXGIHash;
}

#define IMPLEMENT_VXGI_LIGHTMAPPED_SHADER_TYPE(LightMapPolicyType,LightMapPolicyName) \
	typedef TVXGIVoxelizationHS< LightMapPolicyType > TVXGIVoxelizationHS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TVXGIVoxelizationHS##LightMapPolicyName,TEXT("BasePassForForwardShadingVertexShader"),TEXT("MainHull"),SF_Hull); \
	typedef TVXGIVoxelizationDS< LightMapPolicyType > TVXGIVoxelizationDS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TVXGIVoxelizationDS##LightMapPolicyName,TEXT("BasePassForForwardShadingVertexShader"),TEXT("MainDomain"),SF_Domain); \
	typedef TVXGIVoxelizationVS< LightMapPolicyType > TVXGIVoxelizationVS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TVXGIVoxelizationVS##LightMapPolicyName,TEXT("BasePassForForwardShadingVertexShader"),TEXT("Main"),SF_Vertex); \
	typedef TVXGIVoxelizationPS< LightMapPolicyType > TVXGIVoxelizationPS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TVXGIVoxelizationPS##LightMapPolicyName,TEXT("VXGIVoxelizationPixelShader"),TEXT("Main"),SF_Pixel); \
	typedef TVXGIVoxelizationShaderPermutationPS< LightMapPolicyType > TVXGIVoxelizationShaderPermutationPS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TVXGIVoxelizationShaderPermutationPS##LightMapPolicyName,TEXT("VXGIVoxelizationPixelShader"),TEXT("Main"),SF_Pixel);

// Implement shader types per lightmap policy
IMPLEMENT_VXGI_LIGHTMAPPED_SHADER_TYPE(FVXGIVoxelizationNoLightMapPolicy, FVXGIVoxelizationNoLightMapPolicy);

//This is based on ProcessBasePassMeshForForwardShading
template<typename ProcessActionType>
void ProcessVxgiVoxelizationMeshForForwardShading(
	FRHICommandList& RHICmdList,
	const FProcessBasePassMeshParameters& Parameters,
	const ProcessActionType& Action
	)
{
	Action.template Process<FVXGIVoxelizationNoLightMapPolicy>(RHICmdList, Parameters, FVXGIVoxelizationNoLightMapPolicy(), FVXGIVoxelizationNoLightMapPolicy::ElementDataType());
}

/** The action used to draw a base pass static mesh element. */
class FDrawVXGIVoxelizationStaticMeshAction
{
public:

	FScene* Scene;
	FStaticMesh* StaticMesh;

	/** Initialization constructor. */
	FDrawVXGIVoxelizationStaticMeshAction(FScene* InScene, FStaticMesh* InStaticMesh) :
		Scene(InScene),
		StaticMesh(InStaticMesh)
	{}

	inline bool ShouldPackAmbientSH() const
	{
		return false;
	}

	const FLightSceneInfo* GetSimpleDirectionalLight() const
	{
		return Scene->SimpleDirectionalLight;
	}

	template<typename LightMapPolicyType>
	void Process(
		FRHICommandList& RHICmdList,
		const FProcessBasePassMeshParameters& Parameters,
		const LightMapPolicyType& LightMapPolicy,
		const typename LightMapPolicyType::ElementDataType& LightMapElementData
		) const
	{
		// Find the appropriate draw list for the static mesh based on the light-map policy type.
		FScene::EVoxelizationPassDrawListType ListType = Parameters.Material->HasEmissiveColorConnected() ? FScene::EVoxelizationPass_EmissiveMaterial : FScene::EVoxelizationPass_Default;
		TStaticMeshDrawList<TVXGIVoxelizationDrawingPolicy<LightMapPolicyType> >& DrawList = Scene->GetVoxelizationDrawList<LightMapPolicyType>(ListType);

		// Add the static mesh to the draw list.
		DrawList.AddMesh(
			StaticMesh,
			typename TVXGIVoxelizationDrawingPolicy<LightMapPolicyType>::ElementDataType(LightMapElementData, StaticMesh->PrimitiveSceneInfo),
			TVXGIVoxelizationDrawingPolicy<LightMapPolicyType>(
			StaticMesh->VertexFactory,
			StaticMesh->MaterialRenderProxy,
			*Parameters.Material,
			LightMapPolicy
			),
			Scene->GetFeatureLevel()
			);
	}
};

void TVXGIVoxelizationDrawingPolicyFactory::AddStaticMesh(FRHICommandList& RHICmdList, FScene* Scene, FStaticMesh* StaticMesh)
{
	const FMaterial* Material = StaticMesh->MaterialRenderProxy->GetMaterial(Scene->GetFeatureLevel());
	if (!IsMaterialVoxelized(Material))
	{
		return;
	}

	ProcessVxgiVoxelizationMeshForForwardShading(
		RHICmdList,
		FProcessBasePassMeshParameters(
			*StaticMesh,
			Material,
			StaticMesh->PrimitiveSceneInfo->Proxy,
			true,
			false,
			ESceneRenderTargetsMode::SetTextures,
			Scene->GetFeatureLevel()
			),
		FDrawVXGIVoxelizationStaticMeshAction(Scene, StaticMesh)
		);
}

/** The action used to draw a base pass dynamic mesh element. */
class FDrawVXGIVoxelizationDynamicMeshAction
{
public:

	const FSceneView& View;
	bool bBackFace;
	FHitProxyId HitProxyId;

	inline bool ShouldPackAmbientSH() const
	{
		return false;
	}

	const FLightSceneInfo* GetSimpleDirectionalLight() const
	{
		return ((FScene*)View.Family->Scene)->SimpleDirectionalLight;
	}

	/** Initialization constructor. */
	FDrawVXGIVoxelizationDynamicMeshAction(
		const FSceneView& InView,
		const bool bInBackFace,
		const FHitProxyId InHitProxyId
		)
		: View(InView)
		, bBackFace(bInBackFace)
		, HitProxyId(InHitProxyId)
	{}

	template<typename LightMapPolicyType>
	void Process(
		FRHICommandList& RHICmdList,
		const FProcessBasePassMeshParameters& Parameters,
		const LightMapPolicyType& LightMapPolicy,
		const typename LightMapPolicyType::ElementDataType& LightMapElementData
		) const
	{
		const bool bIsLitMaterial = Parameters.ShadingModel != MSM_Unlit;

		TVXGIVoxelizationDrawingPolicy<LightMapPolicyType> DrawingPolicy(
			Parameters.Mesh.VertexFactory,
			Parameters.Mesh.MaterialRenderProxy,
			*Parameters.Material,
			LightMapPolicy
			);

		DrawingPolicy.SetSharedState(RHICmdList, &View, typename TVXGIVoxelizationDrawingPolicy<LightMapPolicyType>::ContextDataType());

		for (int32 BatchElementIndex=0; BatchElementIndex < Parameters.Mesh.Elements.Num(); BatchElementIndex++)
		{
			DrawingPolicy.SetMeshRenderState(
				RHICmdList,
				View,
				Parameters.PrimitiveSceneProxy,
				Parameters.Mesh,
				BatchElementIndex,
				bBackFace,
				Parameters.Mesh.DitheredLODTransitionAlpha,
				typename TVXGIVoxelizationDrawingPolicy<LightMapPolicyType>::ElementDataType(LightMapElementData, nullptr),
				typename TVXGIVoxelizationDrawingPolicy<LightMapPolicyType>::ContextDataType()
				);
			DrawingPolicy.DrawMesh(RHICmdList, Parameters.Mesh, BatchElementIndex);
		}
	}
};

bool TVXGIVoxelizationDrawingPolicyFactory::DrawDynamicMesh(
	FRHICommandList& RHICmdList,
	const FSceneView& View,
	ContextType DrawingContext,
	const FMeshBatch& Mesh,
	bool bBackFace,
	bool bPreFog,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	FHitProxyId HitProxyId
	)
{
	const FMaterial* Material = Mesh.MaterialRenderProxy->GetMaterial(View.GetFeatureLevel());
	if (!IsMaterialVoxelized(Material))
	{
		return false;
	}

	if (DrawingContext.MeshFilter == VXGI::RenderMeshFilter::ONLY_EMISSIVE && !Material->HasEmissiveColorConnected())
	{
		return false;
	}

	ProcessVxgiVoxelizationMeshForForwardShading(
		RHICmdList,
		FProcessBasePassMeshParameters(
		Mesh,
		Material,
		PrimitiveSceneProxy,
		true,
		false,
		ESceneRenderTargetsMode::SetTextures,
		View.GetFeatureLevel()
		),
		FDrawVXGIVoxelizationDynamicMeshAction(
		View,
		bBackFace,
		HitProxyId
		)
		);
	return true;
}

#endif
// NVCHANGE_END: Add VXGI
