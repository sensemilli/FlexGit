// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// Scene - script exposed scene enums
//=============================================================================

#pragma once
#include "BlendableInterface.h"
#include "Scene.generated.h"

// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI
#include "GFSDK_VXGI.h"
#endif
// NVCHANGE_END: Add VXGI

/** used by FPostProcessSettings Depth of Fields */
UENUM()
enum EDepthOfFieldMethod
{
	DOFM_BokehDOF UMETA(DisplayName="BokehDOF"),
	DOFM_Gaussian UMETA(DisplayName="GaussianDOF"),
	DOFM_CircleDOF UMETA(DisplayName="CircleDOF"),
	DOFM_MAX,
};

/** Used by FPostProcessSettings Anti-aliasings */
UENUM()
enum EAntiAliasingMethod
{
	AAM_None UMETA(DisplayName="None"),
	AAM_FXAA UMETA(DisplayName="FXAA"),
	AAM_TemporalAA UMETA(DisplayName="TemporalAA"),
	AAM_MAX,
};

USTRUCT()
struct FWeightedBlendable
{
	GENERATED_USTRUCT_BODY()

	/** 0:no effect .. 1:full effect */
	UPROPERTY(interp, BlueprintReadWrite, Category=FWeightedBlendable, meta=(ClampMin = "0.0", ClampMax = "1.0", Delta = "0.01"))
	float Weight;

	/** should be of the IBlendableInterface* type but UProperties cannot express that */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=FWeightedBlendable, meta=( AllowedClasses="BlendableInterface", Keywords="PostProcess" ))
	UObject* Object;

	// default constructor
	FWeightedBlendable()
		: Weight(-1)
		, Object(0)
	{
	}

	// constructor
	// @param InWeight -1 is used to hide the weight and show the "Choose" UI, 0:no effect .. 1:full effect
	FWeightedBlendable(float InWeight, UObject* InObject)
		: Weight(InWeight)
		, Object(InObject)
	{
	}
};

// for easier detail customization, needed?
USTRUCT()
struct FWeightedBlendables
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PostProcessSettings", meta=( Keywords="PostProcess" ))
	TArray<FWeightedBlendable> Array;
};

// NVCHANGE_BEGIN: Add VXGI
/** used by FPostProcessSettings VXGI */
UENUM()
enum EVxgiSpecularTracingFilter
{
	VXGISTF_None UMETA(DisplayName = "None"),
	VXGISTF_Temporal UMETA(DisplayName = "Temporal filter"),
	VXGISTF_Simple UMETA(DisplayName = "Bilateral box filter"),
	VXGISTF_EdgeFollowing UMETA(DisplayName = "Edge-following filter"),
	VXGISTF_MAX,
};
// NVCHANGE_END: Add VXGI


// NVCHANGE_BEGIN: Add HBAO+

UENUM()
enum EHBAOBlurRadius
{
	AOBR_BlurRadius0 UMETA(DisplayName = "Disabled"),
	AOBR_BlurRadius4 UMETA(DisplayName = "4 pixels"),
	AOBR_BlurRadius8 UMETA(DisplayName = "8 pixels"),
	AOBR_MAX,
};

// NVCHANGE_END: Add HBAO+

/** To be able to use struct PostProcessSettings. */
// Each property consists of a bool to enable it (by default off),
// the variable declaration and further down the default value for it.
// The comment should include the meaning and usable range.
USTRUCT(BlueprintType, meta=(HiddenByDefault))
struct FPostProcessSettings
{
	GENERATED_USTRUCT_BODY()

	// first all bOverride_... as they get grouped together into bitfields

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_WhiteTemp:1;
	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_WhiteTint:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_ColorSaturation:1;
	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_ColorContrast:1;
	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_ColorGamma:1;
	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_ColorGain:1;
	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_ColorOffset:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_FilmWhitePoint:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_FilmSaturation:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_FilmChannelMixerRed:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_FilmChannelMixerGreen:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_FilmChannelMixerBlue:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_FilmContrast:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_FilmDynamicRange:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_FilmHealAmount:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_FilmToeAmount:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_FilmShadowTint:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_FilmShadowTintBlend:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_FilmShadowTintAmount:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_FilmSlope:1;
	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_FilmToe:1;
	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_FilmShoulder:1;
	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_FilmBlackClip:1;
	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_FilmWhiteClip:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_SceneColorTint:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_SceneFringeIntensity:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_SceneFringeSaturation:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AmbientCubemapTint:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AmbientCubemapIntensity:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_BloomIntensity:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_BloomThreshold:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_Bloom1Tint:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_Bloom1Size:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_Bloom2Size:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_Bloom2Tint:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_Bloom3Tint:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_Bloom3Size:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_Bloom4Tint:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_Bloom4Size:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_Bloom5Tint:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_Bloom5Size:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_Bloom6Tint:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_Bloom6Size:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_BloomSizeScale:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_BloomDirtMaskIntensity:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_BloomDirtMaskTint:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_BloomDirtMask:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AutoExposureLowPercent:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AutoExposureHighPercent:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AutoExposureMinBrightness:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AutoExposureMaxBrightness:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AutoExposureSpeedUp:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AutoExposureSpeedDown:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AutoExposureBias:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_HistogramLogMin:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_HistogramLogMax:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_LensFlareIntensity:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_LensFlareTint:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_LensFlareTints:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_LensFlareBokehSize:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_LensFlareBokehShape:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_LensFlareThreshold:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_VignetteIntensity:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_GrainIntensity:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_GrainJitter:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AmbientOcclusionIntensity:1;

	// NVCHANGE_BEGIN: Add HBAO+

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_HBAOPowerExponent : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_HBAORadius : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_HBAOBias : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_HBAODetailAO : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_HBAOBlurRadius : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_HBAOBlurSharpness : 1;

	// NVCHANGE_END: Add HBAO+

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AmbientOcclusionStaticFraction:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AmbientOcclusionRadius:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AmbientOcclusionFadeDistance:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AmbientOcclusionFadeRadius:1;

	UPROPERTY()
	uint32 bOverride_AmbientOcclusionDistance_DEPRECATED:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AmbientOcclusionRadiusInWS:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AmbientOcclusionPower:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AmbientOcclusionBias:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AmbientOcclusionQuality:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AmbientOcclusionMipBlend:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AmbientOcclusionMipScale:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AmbientOcclusionMipThreshold:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_LPVIntensity:1;

	UPROPERTY()
	uint32 bOverride_LPVDirectionalOcclusionIntensity:1;

	UPROPERTY()
	uint32 bOverride_LPVDirectionalOcclusionRadius:1;

	UPROPERTY()
	uint32 bOverride_LPVDiffuseOcclusionExponent:1;

	UPROPERTY()
	uint32 bOverride_LPVSpecularOcclusionExponent:1;

	UPROPERTY()
	uint32 bOverride_LPVDiffuseOcclusionIntensity:1;

	UPROPERTY()
	uint32 bOverride_LPVSpecularOcclusionIntensity:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_LPVSize:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_LPVSecondaryOcclusionIntensity:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_LPVSecondaryBounceIntensity:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_LPVGeometryVolumeBias:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_LPVVplInjectionBias:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_LPVEmissiveInjectionIntensity:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_IndirectLightingColor:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_IndirectLightingIntensity:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_ColorGradingIntensity:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_ColorGradingLUT:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_DepthOfFieldFocalDistance:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_DepthOfFieldFstop:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_DepthOfFieldDepthBlurRadius:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_DepthOfFieldDepthBlurAmount:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_DepthOfFieldFocalRegion:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_DepthOfFieldNearTransitionRegion:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_DepthOfFieldFarTransitionRegion:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_DepthOfFieldScale:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_DepthOfFieldMaxBokehSize:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_DepthOfFieldNearBlurSize:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_DepthOfFieldFarBlurSize:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_DepthOfFieldMethod:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_DepthOfFieldBokehShape:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_DepthOfFieldOcclusion:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_DepthOfFieldColorThreshold:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_DepthOfFieldSizeThreshold:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_DepthOfFieldSkyFocusDistance:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_MotionBlurAmount:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_MotionBlurMax:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_MotionBlurPerObjectSize:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_ScreenPercentage:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_AntiAliasingMethod:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_ScreenSpaceReflectionIntensity:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_ScreenSpaceReflectionQuality:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_ScreenSpaceReflectionMaxRoughness:1;

	UPROPERTY(BlueprintReadWrite, Category=Overrides, meta=(PinHiddenByDefault))
	uint32 bOverride_ScreenSpaceReflectionRoughnessScale:1;

	// NVCHANGE_BEGIN: Add VXGI

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingEnabled : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiSpecularTracingEnabled : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingIntensity : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiSpecularTracingIntensity : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiMultiBounceIrradianceScale : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingSparsity : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingNumCones : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_bVxgiDiffuseTracingAutoAngle : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingConeAngle : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingConeNormalGroupingFactor : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingMaxSamples : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiSpecularTracingMaxSamples : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingStep : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiSpecularTracingTracingStep : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingOpacityCorrectionFactor : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiSpecularTracingOpacityCorrectionFactor : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_bVxgiDiffuseTracingConeRotation : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_bVxgiDiffuseTracingRandomConeOffsets : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingNormalOffsetFactor : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingAmbientColor : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingAmbientRange : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingEnvironmentMapTint : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingEnvironmentMap : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingInitialOffsetBias : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingInitialOffsetDistanceFactor : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiSpecularTracingInitialOffsetBias : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiSpecularTracingInitialOffsetDistanceFactor : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiSpecularTracingEnvironmentMapTint : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiSpecularTracingFilter : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiSpecularTracingEnvironmentMap : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiSpecularTracingTangentJitterScale : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_bVxgiDiffuseTracingTemporalReprojectionEnabled : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingTemporalReprojectionPreviousFrameWeight : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingTemporalReprojectionMaxDistanceInVoxels : 1;

	UPROPERTY(BlueprintReadWrite, Category = Overrides, meta = (PinHiddenByDefault))
	uint32 bOverride_VxgiDiffuseTracingTemporalReprojectionNormalWeightExponent : 1;

	// NVCHANGE_END: Add VXGI

	// -----------------------------------------------------------------------

	UPROPERTY(interp, BlueprintReadWrite, Category=WhiteBalance, meta=(UIMin = "1500.0", UIMax = "15000.0", editcondition = "bOverride_WhiteTemp", DisplayName = "Temp"))
	float WhiteTemp;
	UPROPERTY(interp, BlueprintReadWrite, Category=WhiteBalance, meta=(UIMin = "-1.0", UIMax = "1.0", editcondition = "bOverride_WhiteTint", DisplayName = "Tint"))
	float WhiteTint;

	UPROPERTY(interp, BlueprintReadWrite, Category=ColorGrading, meta=(UIMin = "0.0", UIMax = "2.0", editcondition = "bOverride_ColorSaturation", DisplayName = "Saturation"))
	FVector ColorSaturation;
	UPROPERTY(interp, BlueprintReadWrite, Category=ColorGrading, meta=(UIMin = "0.0", UIMax = "2.0", editcondition = "bOverride_ColorContrast", DisplayName = "Contrast"))
	FVector ColorContrast;
	UPROPERTY(interp, BlueprintReadWrite, Category=ColorGrading, meta=(UIMin = "0.0", UIMax = "2.0", editcondition = "bOverride_ColorGamma", DisplayName = "Gamma"))
	FVector ColorGamma;
	UPROPERTY(interp, BlueprintReadWrite, Category=ColorGrading, meta=(UIMin = "0.0", UIMax = "2.0", editcondition = "bOverride_ColorGain", DisplayName = "Gain"))
	FVector ColorGain;
	UPROPERTY(interp, BlueprintReadWrite, Category=ColorGrading, meta=(UIMin = "-1.0", UIMax = "1.0", editcondition = "bOverride_ColorOffset", DisplayName = "Offset"))
	FVector ColorOffset;

	UPROPERTY(interp, BlueprintReadWrite, Category=Film, meta=(editcondition = "bOverride_FilmWhitePoint", DisplayName = "Tint", HideAlphaChannel))
	FLinearColor FilmWhitePoint;
	UPROPERTY(interp, BlueprintReadWrite, Category=Film, AdvancedDisplay, meta=(editcondition = "bOverride_FilmShadowTint", DisplayName = "Tint Shadow", HideAlphaChannel))
	FLinearColor FilmShadowTint;
	UPROPERTY(interp, BlueprintReadWrite, Category=Film, AdvancedDisplay, meta=(UIMin = "0.0", UIMax = "1.0", editcondition = "bOverride_FilmShadowTintBlend", DisplayName = "Tint Shadow Blend"))
	float FilmShadowTintBlend;
	UPROPERTY(interp, BlueprintReadWrite, Category=Film, AdvancedDisplay, meta=(UIMin = "0.0", UIMax = "1.0", editcondition = "bOverride_FilmShadowTintAmount", DisplayName = "Tint Shadow Amount"))
	float FilmShadowTintAmount;

	UPROPERTY(interp, BlueprintReadWrite, Category=Film, meta=(UIMin = "0.0", UIMax = "2.0", editcondition = "bOverride_FilmSaturation", DisplayName = "Saturation"))
	float FilmSaturation;
	UPROPERTY(interp, BlueprintReadWrite, Category=Film, AdvancedDisplay, meta=(editcondition = "bOverride_FilmChannelMixerRed", DisplayName = "Channel Mixer Red", HideAlphaChannel))
	FLinearColor FilmChannelMixerRed;
	UPROPERTY(interp, BlueprintReadWrite, Category=Film, AdvancedDisplay, meta=(editcondition = "bOverride_FilmChannelMixerGreen", DisplayName = "Channel Mixer Green", HideAlphaChannel))
	FLinearColor FilmChannelMixerGreen;
	UPROPERTY(interp, BlueprintReadWrite, Category=Film, AdvancedDisplay, meta=(editcondition = "bOverride_FilmChannelMixerBlue", DisplayName = " Channel Mixer Blue", HideAlphaChannel))
	FLinearColor FilmChannelMixerBlue;

	UPROPERTY(interp, BlueprintReadWrite, Category=Film, meta=(UIMin = "0.0", UIMax = "1.0", editcondition = "bOverride_FilmContrast", DisplayName = "Contrast"))
	float FilmContrast;
	UPROPERTY(interp, BlueprintReadWrite, Category=Film, AdvancedDisplay, meta=(UIMin = "0.0", UIMax = "1.0", editcondition = "bOverride_FilmToeAmount", DisplayName = "Crush Shadows"))
	float FilmToeAmount;
	UPROPERTY(interp, BlueprintReadWrite, Category=Film, AdvancedDisplay, meta=(UIMin = "0.0", UIMax = "1.0", editcondition = "bOverride_FilmHealAmount", DisplayName = "Crush Highlights"))
	float FilmHealAmount;
	UPROPERTY(interp, BlueprintReadWrite, Category=Film, AdvancedDisplay, meta=(UIMin = "1.0", UIMax = "4.0", editcondition = "bOverride_FilmDynamicRange", DisplayName = "Dynamic Range"))
	float FilmDynamicRange;

	UPROPERTY(interp, BlueprintReadWrite, Category=Film, AdvancedDisplay, meta=(UIMin = "0.0", UIMax = "1.0", editcondition = "bOverride_FilmSlope", DisplayName = "Slope"))
	float FilmSlope;
	UPROPERTY(interp, BlueprintReadWrite, Category=Film, AdvancedDisplay, meta=(UIMin = "0.0", UIMax = "1.0", editcondition = "bOverride_FilmToe", DisplayName = "Toe"))
	float FilmToe;
	UPROPERTY(interp, BlueprintReadWrite, Category=Film, AdvancedDisplay, meta=(UIMin = "0.0", UIMax = "1.0", editcondition = "bOverride_FilmShoulder", DisplayName = "Shoulder"))
	float FilmShoulder;
	UPROPERTY(interp, BlueprintReadWrite, Category=Film, AdvancedDisplay, meta=(UIMin = "0.0", UIMax = "1.0", editcondition = "bOverride_FilmBlackClip", DisplayName = "Black clip"))
	float FilmBlackClip;
	UPROPERTY(interp, BlueprintReadWrite, Category=Film, AdvancedDisplay, meta=(UIMin = "0.0", UIMax = "1.0", editcondition = "bOverride_FilmWhiteClip", DisplayName = "White clip"))
	float FilmWhiteClip;
	
	/** Scene tint color */
	UPROPERTY(interp, BlueprintReadWrite, Category=SceneColor, AdvancedDisplay, meta=(editcondition = "bOverride_SceneColorTint", HideAlphaChannel))
	FLinearColor SceneColorTint;
	
	/** in percent, Scene chromatic aberration / color fringe (camera imperfection) to simulate an artifact that happens in real-world lens, mostly visible in the image corners. */
	UPROPERTY(interp, BlueprintReadWrite, Category=SceneColor, meta=(UIMin = "0.0", UIMax = "5.0", editcondition = "bOverride_SceneFringeIntensity", DisplayName = "Fringe Intensity"))
	float SceneFringeIntensity;

	/** 0..1, Scene chromatic aberration / color fringe (camera imperfection) to simulate an artifact that happens in real-world lens, mostly visible in the image corners. */
	UPROPERTY(interp, BlueprintReadWrite, Category=SceneColor, AdvancedDisplay, meta=(ClampMin = "0.0", UIMax = "1.0", editcondition = "bOverride_SceneFringeSaturation", DisplayName = "Fringe Saturation"))
	float SceneFringeSaturation;

	/** Multiplier for all bloom contributions >=0: off, 1(default), >1 brighter */
	UPROPERTY(interp, BlueprintReadWrite, Category=Bloom, meta=(ClampMin = "0.0", UIMax = "8.0", editcondition = "bOverride_BloomIntensity", DisplayName = "Intensity"))
	float BloomIntensity;

	/**
	 * minimum brightness the bloom starts having effect
	 * -1:all pixels affect bloom equally (dream effect), 0:all pixels affect bloom brights more, 1(default), >1 brighter
	 */
	UPROPERTY(interp, BlueprintReadWrite, Category=Bloom, AdvancedDisplay, meta=(ClampMin = "-1.0", UIMin = "0.0", UIMax = "8.0", editcondition = "bOverride_BloomThreshold", DisplayName = "Threshold"))
	float BloomThreshold;

	/**
	 * Scale for all bloom sizes
	 */
	UPROPERTY(interp, BlueprintReadWrite, Category=Bloom, AdvancedDisplay, meta=(ClampMin = "0.0", UIMax = "64.0", editcondition = "bOverride_BloomSizeScale", DisplayName = "Size scale"))
	float BloomSizeScale;

	/**
	 * Diameter size for the Bloom1 in percent of the screen width
	 * (is done in 1/2 resolution, larger values cost more performance, good for high frequency details)
	 * >=0: can be clamped because of shader limitations
	 */
	UPROPERTY(interp, BlueprintReadWrite, Category=Bloom, AdvancedDisplay, meta=(ClampMin = "0.0", UIMax = "4.0", editcondition = "bOverride_Bloom1Size", DisplayName = "#1 Size"))
	float Bloom1Size;
	/**
	 * Diameter size for Bloom2 in percent of the screen width
	 * (is done in 1/4 resolution, larger values cost more performance)
	 * >=0: can be clamped because of shader limitations
	 */
	UPROPERTY(interp, BlueprintReadWrite, Category=Bloom, AdvancedDisplay, meta=(ClampMin = "0.0", UIMax = "8.0", editcondition = "bOverride_Bloom2Size", DisplayName = "#2 Size"))
	float Bloom2Size;
	/**
	 * Diameter size for Bloom3 in percent of the screen width
	 * (is done in 1/8 resolution, larger values cost more performance)
	 * >=0: can be clamped because of shader limitations
	 */
	UPROPERTY(interp, BlueprintReadWrite, Category=Bloom, AdvancedDisplay, meta=(ClampMin = "0.0", UIMax = "16.0", editcondition = "bOverride_Bloom3Size", DisplayName = "#3 Size"))
	float Bloom3Size;
	/**
	 * Diameter size for Bloom4 in percent of the screen width
	 * (is done in 1/16 resolution, larger values cost more performance, best for wide contributions)
	 * >=0: can be clamped because of shader limitations
	 */
	UPROPERTY(interp, BlueprintReadWrite, Category=Bloom, AdvancedDisplay, meta=(ClampMin = "0.0", UIMax = "32.0", editcondition = "bOverride_Bloom4Size", DisplayName = "#4 Size"))
	float Bloom4Size;
	/**
	 * Diameter size for Bloom5 in percent of the screen width
	 * (is done in 1/32 resolution, larger values cost more performance, best for wide contributions)
	 * >=0: can be clamped because of shader limitations
	 */
	UPROPERTY(interp, BlueprintReadWrite, Category=Bloom, AdvancedDisplay, meta=(ClampMin = "0.0", UIMax = "64.0", editcondition = "bOverride_Bloom5Size", DisplayName = "#5 Size"))
	float Bloom5Size;
	/**
	 * Diameter size for Bloom6 in percent of the screen width
	 * (is done in 1/64 resolution, larger values cost more performance, best for wide contributions)
	 * >=0: can be clamped because of shader limitations
	 */
	UPROPERTY(interp, BlueprintReadWrite, Category=Bloom, AdvancedDisplay, meta=(ClampMin = "0.0", UIMax = "128.0", editcondition = "bOverride_Bloom6Size", DisplayName = "#6 Size"))
	float Bloom6Size;

	/** Bloom1 tint color */
	UPROPERTY(interp, BlueprintReadWrite, Category=Bloom, AdvancedDisplay, meta=(editcondition = "bOverride_Bloom1Tint", DisplayName = "#1 Tint", HideAlphaChannel))
	FLinearColor Bloom1Tint;
	/** Bloom2 tint color */
	UPROPERTY(interp, BlueprintReadWrite, Category=Bloom, AdvancedDisplay, meta=(editcondition = "bOverride_Bloom2Tint", DisplayName = "#2 Tint", HideAlphaChannel))
	FLinearColor Bloom2Tint;
	/** Bloom3 tint color */
	UPROPERTY(interp, BlueprintReadWrite, Category=Bloom, AdvancedDisplay, meta=(editcondition = "bOverride_Bloom3Tint", DisplayName = "#3 Tint", HideAlphaChannel))
	FLinearColor Bloom3Tint;
	/** Bloom4 tint color */
	UPROPERTY(interp, BlueprintReadWrite, Category=Bloom, AdvancedDisplay, meta=(editcondition = "bOverride_Bloom4Tint", DisplayName = "#4 Tint", HideAlphaChannel))
	FLinearColor Bloom4Tint;
	/** Bloom5 tint color */
	UPROPERTY(interp, BlueprintReadWrite, Category=Bloom, AdvancedDisplay, meta=(editcondition = "bOverride_Bloom5Tint", DisplayName = "#5 Tint", HideAlphaChannel))
	FLinearColor Bloom5Tint;
	/** Bloom6 tint color */
	UPROPERTY(interp, BlueprintReadWrite, Category=Bloom, AdvancedDisplay, meta=(editcondition = "bOverride_Bloom6Tint", DisplayName = "#6 Tint", HideAlphaChannel))
	FLinearColor Bloom6Tint;

	/** BloomDirtMask intensity */
	UPROPERTY(interp, BlueprintReadWrite, Category=Bloom, meta=(ClampMin = "0.0", UIMax = "8.0", editcondition = "bOverride_BloomDirtMaskIntensity", DisplayName = "Dirt Mask Intensity"))
	float BloomDirtMaskIntensity;

	/** BloomDirtMask tint color */
	UPROPERTY(interp, BlueprintReadWrite, Category=Bloom, AdvancedDisplay, meta=(editcondition = "bOverride_BloomDirtMaskTint", DisplayName = "Dirt Mask Tint", HideAlphaChannel))
	FLinearColor BloomDirtMaskTint;

	/**
	 * Texture that defines the dirt on the camera lens where the light of very bright objects is scattered.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Bloom, meta=(editcondition = "bOverride_BloomDirtMask", DisplayName = "Dirt Mask"))
	class UTexture* BloomDirtMask;	// The plan is to replace this texture with many small textures quads for better performance, more control and to animate the effect.

	/** How strong the dynamic GI from the LPV should be. 0.0 is off, 1.0 is the "normal" value, but higher values can be used to boost the effect*/
	UPROPERTY(interp, BlueprintReadWrite, Category=LightPropagationVolume, meta=(editcondition = "bOverride_LPVIntensity", UIMin = "0", UIMax = "20", DisplayName = "Intensity") )
	float LPVIntensity;

	/** Bias applied to light injected into the LPV in cell units. Increase to reduce bleeding through thin walls*/
	UPROPERTY(interp, BlueprintReadWrite, Category=LightPropagationVolume, AdvancedDisplay, meta=(editcondition = "bOverride_LPVVplInjectionBias", UIMin = "0", UIMax = "2", DisplayName = "Light Injection Bias") )
	float LPVVplInjectionBias;

	/** The size of the LPV volume, in Unreal units*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=LightPropagationVolume, meta=(editcondition = "bOverride_LPVSize", UIMin = "100", UIMax = "20000", DisplayName = "Size") )
	float LPVSize;

	/** Secondary occlusion strength (bounce light shadows). Set to 0 to disable*/
	UPROPERTY(interp, BlueprintReadWrite, Category=LightPropagationVolume, meta=(editcondition = "bOverride_LPVSecondaryOcclusionIntensity", UIMin = "0", UIMax = "1", DisplayName = "Secondary Occlusion Intensity") )
	float LPVSecondaryOcclusionIntensity;

	/** Secondary bounce light strength (bounce light shadows). Set to 0 to disable*/
	UPROPERTY(interp, BlueprintReadWrite, Category=LightPropagationVolume, AdvancedDisplay, meta=(editcondition = "bOverride_LPVSecondaryBounceIntensity", UIMin = "0", UIMax = "1", DisplayName = "Secondary Bounce Intensity") )
	float LPVSecondaryBounceIntensity;

	/** Bias applied to the geometry volume in cell units. Increase to reduce darkening due to secondary occlusion */
	UPROPERTY(interp, BlueprintReadWrite, Category=LightPropagationVolume, AdvancedDisplay, meta=(editcondition = "bOverride_LPVGeometryVolumeBias", UIMin = "0", UIMax = "2", DisplayName = "Geometry Volume Bias"))
	float LPVGeometryVolumeBias;

	UPROPERTY(interp, BlueprintReadWrite, Category=LightPropagationVolume, AdvancedDisplay, meta=(editcondition = "bOverride_LPVEmissiveInjectionIntensity", UIMin = "0", UIMax = "20", DisplayName = "Emissive Injection Intensity") )
	float LPVEmissiveInjectionIntensity;

	/** Controls the amount of directional occlusion. Requires LPV. Values very close to 1.0 are recommended */
	UPROPERTY(interp, BlueprintReadWrite, Category=LightPropagationVolume, meta=(editcondition = "bOverride_LPVDirectionalOcclusionIntensity", UIMin = "0", UIMax = "1", DisplayName = "Occlusion Intensity") )
	float LPVDirectionalOcclusionIntensity;

	/** Occlusion Radius - 16 is recommended for most scenes */
	UPROPERTY(interp, BlueprintReadWrite, Category=LightPropagationVolume, AdvancedDisplay, meta=(editcondition = "bOverride_LPVDirectionalOcclusionRadius", UIMin = "1", UIMax = "16", DisplayName = "Occlusion Radius") )
	float LPVDirectionalOcclusionRadius;

	/** Diffuse occlusion exponent - increase for more contrast. 1 to 2 is recommended */
	UPROPERTY(interp, BlueprintReadWrite, Category=LightPropagationVolume, meta=(editcondition = "bOverride_LPVDiffuseOcclusionExponent", UIMin = "0.5", UIMax = "5", DisplayName = "Diffuse occlusion exponent") )
	float LPVDiffuseOcclusionExponent;

	/** Specular occlusion exponent - increase for more contrast. 6 to 9 is recommended */
	UPROPERTY(interp, BlueprintReadWrite, Category=LightPropagationVolume, meta=(editcondition = "bOverride_LPVSpecularOcclusionExponent", UIMin = "1", UIMax = "16", DisplayName = "Specular occlusion exponent") )
	float LPVSpecularOcclusionExponent;

	/** Diffuse occlusion intensity - higher values provide increased diffuse occlusion.*/
	UPROPERTY(interp, BlueprintReadWrite, Category=LightPropagationVolume, AdvancedDisplay, meta=(editcondition = "bOverride_LPVDiffuseOcclusionIntensity", UIMin = "0", UIMax = "4", DisplayName = "Diffuse occlusion intensity") )
	float LPVDiffuseOcclusionIntensity;

	/** Specular occlusion intensity - higher values provide increased specular occlusion.*/
	UPROPERTY(interp, BlueprintReadWrite, Category=LightPropagationVolume, AdvancedDisplay, meta=(editcondition = "bOverride_LPVSpecularOcclusionIntensity", UIMin = "0", UIMax = "4", DisplayName = "Specular occlusion intensity") )
	float LPVSpecularOcclusionIntensity;
	
	/** AmbientCubemap tint color */
	UPROPERTY(interp, BlueprintReadWrite, Category=AmbientCubemap, AdvancedDisplay, meta=(editcondition = "bOverride_AmbientCubemapTint", DisplayName = "Tint", HideAlphaChannel))
	FLinearColor AmbientCubemapTint;
	/**
	 * To scale the Ambient cubemap brightness
	 * >=0: off, 1(default), >1 brighter
	 */
	UPROPERTY(interp, BlueprintReadWrite, Category=AmbientCubemap, meta=(ClampMin = "0.0", UIMax = "4.0", editcondition = "bOverride_AmbientCubemapIntensity", DisplayName = "Intensity"))
	float AmbientCubemapIntensity;
	/** The Ambient cubemap (Affects diffuse and specular shading), blends additively which if different from all other settings here */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AmbientCubemap, meta=(DisplayName = "Cubemap Texture"))
	class UTextureCube* AmbientCubemap;

	/**
	 * The eye adaptation will adapt to a value extracted from the luminance histogram of the scene color.
	 * The value is defined as having x percent below this brightness. Higher values give bright spots on the screen more priority
	 * but can lead to less stable results. Lower values give the medium and darker values more priority but might cause burn out of
	 * bright spots.
	 * >0, <100, good values are in the range 70 .. 80
	 */
	UPROPERTY(interp, BlueprintReadWrite, Category=AutoExposure, AdvancedDisplay, meta=(ClampMin = "0.0", ClampMax = "100.0", editcondition = "bOverride_AutoExposureLowPercent", DisplayName = "Low Percent"))
	float AutoExposureLowPercent;

	/**
	 * The eye adaptation will adapt to a value extracted from the luminance histogram of the scene color.
	 * The value is defined as having x percent below this brightness. Higher values give bright spots on the screen more priority
	 * but can lead to less stable results. Lower values give the medium and darker values more priority but might cause burn out of
	 * bright spots.
	 * >0, <100, good values are in the range 80 .. 95
	 */
	UPROPERTY(interp, BlueprintReadWrite, Category=AutoExposure, AdvancedDisplay, meta=(ClampMin = "0.0", ClampMax = "100.0", editcondition = "bOverride_AutoExposureHighPercent", DisplayName = "High Percent"))
	float AutoExposureHighPercent;

	/**
	 * A good value should be positive near 0. This is the minimum brightness the auto exposure can adapt to.
	 * It should be tweaked in a dark lighting situation (too small: image appears too bright, too large: image appears too dark).
	 * Note: Tweaking emissive materials and lights or tweaking auto exposure can look the same. Tweaking auto exposure has global
	 * effect and defined the HDR range - you don't want to change that late in the project development.
	 * Eye Adaptation is disabled if MinBrightness = MaxBrightness
	 */
	UPROPERTY(interp, BlueprintReadWrite, Category=AutoExposure, meta=(ClampMin = "0.0", UIMax = "10.0", editcondition = "bOverride_AutoExposureMinBrightness", DisplayName = "Min Brightness"))
	float AutoExposureMinBrightness;

	/**
	 * A good value should be positive (2 is a good value). This is the maximum brightness the auto exposure can adapt to.
	 * It should be tweaked in a bright lighting situation (too small: image appears too bright, too large: image appears too dark).
	 * Note: Tweaking emissive materials and lights or tweaking auto exposure can look the same. Tweaking auto exposure has global
	 * effect and defined the HDR range - you don't want to change that late in the project development.
	 * Eye Adaptation is disabled if MinBrightness = MaxBrightness
	 */
	UPROPERTY(interp, BlueprintReadWrite, Category=AutoExposure, meta=(ClampMin = "0.0", UIMax = "10.0", editcondition = "bOverride_AutoExposureMaxBrightness", DisplayName = "Max Brightness"))
	float AutoExposureMaxBrightness;

	/** >0 */
	UPROPERTY(interp, BlueprintReadWrite, Category=AutoExposure, AdvancedDisplay, meta=(ClampMin = "0.02", UIMax = "20.0", editcondition = "bOverride_AutoExposureSpeedUp", DisplayName = "Speed Up"))
	float AutoExposureSpeedUp;

	/** >0 */
	UPROPERTY(interp, BlueprintReadWrite, Category=AutoExposure, AdvancedDisplay, meta=(ClampMin = "0.02", UIMax = "20.0", editcondition = "bOverride_AutoExposureSpeedDown", DisplayName = "Speed Down"))
	float AutoExposureSpeedDown;

	/**
	 * Logarithmic adjustment for the exposure. Only used if a tonemapper is specified.
	 * 0: no adjustment, -1:2x darker, -2:4x darker, 1:2x brighter, 2:4x brighter, ...
	 */
	UPROPERTY(interp, BlueprintReadWrite, Category = AutoExposure, meta = (UIMin = "-8.0", UIMax = "8.0", editcondition = "bOverride_AutoExposureBias", DisplayName = "Exposure Bias"))
	float AutoExposureBias;

	/** temporary exposed until we found good values, -8: 1/256, -10: 1/1024 */
	UPROPERTY(interp, BlueprintReadWrite, Category=AutoExposure, AdvancedDisplay, meta=(UIMin = "-16", UIMax = "0.0", editcondition = "bOverride_HistogramLogMin"))
	float HistogramLogMin;

	/** temporary exposed until we found good values 4: 16, 8: 256 */
	UPROPERTY(interp, BlueprintReadWrite, Category=AutoExposure, AdvancedDisplay, meta=(UIMin = "0.0", UIMax = "16.0", editcondition = "bOverride_HistogramLogMax"))
	float HistogramLogMax;

	/** Brightness scale of the image cased lens flares (linear) */
	UPROPERTY(interp, BlueprintReadWrite, Category=LensFlares, meta=(UIMin = "0.0", UIMax = "16.0", editcondition = "bOverride_LensFlareIntensity", DisplayName = "Intensity"))
	float LensFlareIntensity;

	/** Tint color for the image based lens flares. */
	UPROPERTY(interp, BlueprintReadWrite, Category=LensFlares, AdvancedDisplay, meta=(editcondition = "bOverride_LensFlareTint", DisplayName = "Tint", HideAlphaChannel))
	FLinearColor LensFlareTint;

	/** Size of the Lens Blur (in percent of the view width) that is done with the Bokeh texture (note: performance cost is radius*radius) */
	UPROPERTY(interp, BlueprintReadWrite, Category=LensFlares, meta=(UIMin = "0.0", UIMax = "32.0", editcondition = "bOverride_LensFlareBokehSize", DisplayName = "BokehSize"))
	float LensFlareBokehSize;

	/** Minimum brightness the lens flare starts having effect (this should be as high as possible to avoid the performance cost of blurring content that is too dark too see) */
	UPROPERTY(interp, BlueprintReadWrite, Category=LensFlares, AdvancedDisplay, meta=(UIMin = "0.1", UIMax = "32.0", editcondition = "bOverride_LensFlareThreshold", DisplayName = "Threshold"))
	float LensFlareThreshold;

	/** Defines the shape of the Bokeh when the image base lens flares are blurred, cannot be blended */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=LensFlares, meta=(editcondition = "bOverride_LensFlareBokehShape", DisplayName = "BokehShape"))
	class UTexture* LensFlareBokehShape;

	/** RGB defines the lens flare color, A it's position. This is a temporary solution. */
	UPROPERTY(EditAnywhere, Category=LensFlares, meta=(editcondition = "bOverride_LensFlareTints", DisplayName = "Tints"))
	FLinearColor LensFlareTints[8];

	/** 0..1 0=off/no vignette .. 1=strong vignette */
	UPROPERTY(interp, BlueprintReadWrite, Category=SceneColor, meta=(UIMin = "0.0", UIMax = "1.0", editcondition = "bOverride_VignetteIntensity"))
	float VignetteIntensity;

	/** 0..1 grain jitter */
	UPROPERTY(interp, BlueprintReadWrite, Category = SceneColor, AdvancedDisplay, meta=(UIMin = "0.0", UIMax = "1.0", editcondition = "bOverride_GrainJitter"))
	float GrainJitter;

	/** 0..1 grain intensity */
	UPROPERTY(interp, BlueprintReadWrite, Category=SceneColor, meta=(UIMin = "0.0", UIMax = "1.0", editcondition = "bOverride_GrainIntensity"))
	float GrainIntensity;

	// NVCHANGE_BEGIN: Add HBAO+

	/** 0..4 >0 to enable HBAO+ (DX11/Windows only) .. the greater this parameter, the darker is the HBAO */
	UPROPERTY(interp, BlueprintReadWrite, Category = "HBAO+", meta = (ClampMin = "0.0", UIMax = "4.0", editcondition = "bOverride_HBAOPowerExponent", DisplayName = "Power Exponent"))
	float HBAOPowerExponent;

	/** 0..2 in meters, bigger values means even distant surfaces affect the ambient occlusion */
	UPROPERTY(interp, BlueprintReadWrite, Category = "HBAO+", meta = (ClampMin = "0.1", UIMax = "2.0", editcondition = "bOverride_HBAORadius", DisplayName = "Radius"))
	float HBAORadius;

	/** 0.0..0.2 increase to hide tesselation artifacts */
	UPROPERTY(interp, BlueprintReadWrite, Category = "HBAO+", meta = (ClampMin = "0.0", UIMax = "0.2", editcondition = "bOverride_HBAOBias", DisplayName = "Bias"))
	float HBAOBias;

	/** 0..1 strength of the low-range occlusion .. set to 0.0 to improve performance */
	UPROPERTY(interp, BlueprintReadWrite, Category = "HBAO+", meta = (ClampMin = "0.0", UIMax = "1.0", editcondition = "bOverride_HBAODetailAO", DisplayName = "Detail AO"))
	float HBAODetailAO;

	/** The HBAO blur is needed to hide noise artifacts .. Blur radius = 4 pixels is recommended */
	UPROPERTY(interp, BlueprintReadWrite, Category = "HBAO+", meta = (editcondition = "bOverride_HBAOBlurRadius", DisplayName = "Blur Radius"))
	TEnumAsByte<enum EHBAOBlurRadius> HBAOBlurRadius;

	/** 0..32 the larger, the more the HBAO blur preserves edges */
	UPROPERTY(interp, BlueprintReadWrite, Category = "HBAO+", meta = (ClampMin = "0.0", UIMax = "32.0", editcondition = "bOverride_HBAOBlurSharpness", DisplayName = "Blur Sharpness"))
	float HBAOBlurSharpness;

	// NVCHANGE_END: Add HBAO+

	/** 0..1 0=off/no ambient occlusion .. 1=strong ambient occlusion, defines how much it affects the non direct lighting after base pass */
	UPROPERTY(interp, BlueprintReadWrite, Category=AmbientOcclusion, meta=(ClampMin = "0.0", ClampMax = "1.0", editcondition = "bOverride_AmbientOcclusionIntensity", DisplayName = "Intensity"))
	float AmbientOcclusionIntensity;

	/** 0..1 0=no effect on static lighting .. 1=AO affects the stat lighting, 0 is free meaning no extra rendering pass */
	UPROPERTY(interp, BlueprintReadWrite, Category=AmbientOcclusion, AdvancedDisplay, meta=(ClampMin = "0.0", ClampMax = "1.0", editcondition = "bOverride_AmbientOcclusionStaticFraction", DisplayName = "Static Fraction"))
	float AmbientOcclusionStaticFraction;

	/** >0, in unreal units, bigger values means even distant surfaces affect the ambient occlusion */
	UPROPERTY(interp, BlueprintReadWrite, Category=AmbientOcclusion, meta=(ClampMin = "0.1", UIMax = "500.0", editcondition = "bOverride_AmbientOcclusionRadius", DisplayName = "Radius"))
	float AmbientOcclusionRadius;

	/** true: AO radius is in world space units, false: AO radius is locked the view space in 400 units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AmbientOcclusion, AdvancedDisplay, meta=(editcondition = "bOverride_AmbientOcclusionRadiusInWS", DisplayName = "Radius in WorldSpace"))
	uint32 AmbientOcclusionRadiusInWS:1;

	/** >0, in unreal units, at what distance the AO effect disppears in the distance (avoding artifacts and AO effects on huge object) */
	UPROPERTY(interp, BlueprintReadWrite, Category=AmbientOcclusion, AdvancedDisplay, meta=(ClampMin = "0.0", UIMax = "20000.0", editcondition = "bOverride_AmbientOcclusionFadeDistance", DisplayName = "Fade Out Distance"))
	float AmbientOcclusionFadeDistance;
	
	/** >0, in unreal units, how many units before AmbientOcclusionFadeOutDistance it starts fading out */
	UPROPERTY(interp, BlueprintReadWrite, Category=AmbientOcclusion, AdvancedDisplay, meta=(ClampMin = "0.0", UIMax = "20000.0", editcondition = "bOverride_AmbientOcclusionFadeRadius", DisplayName = "Fade Out Radius"))
	float AmbientOcclusionFadeRadius;

	/** >0, in unreal units, how wide the ambient occlusion effect should affect the geometry (in depth), will be removed - only used for non normal method which is not exposed */
	UPROPERTY()
	float AmbientOcclusionDistance_DEPRECATED;

	/** >0, in unreal units, bigger values means even distant surfaces affect the ambient occlusion */
	UPROPERTY(interp, BlueprintReadWrite, Category=AmbientOcclusion, AdvancedDisplay, meta=(ClampMin = "0.1", UIMax = "8.0", editcondition = "bOverride_AmbientOcclusionPower", DisplayName = "Power"))
	float AmbientOcclusionPower;

	/** >0, in unreal units, default (3.0) works well for flat surfaces but can reduce details */
	UPROPERTY(interp, BlueprintReadWrite, Category=AmbientOcclusion, AdvancedDisplay, meta=(ClampMin = "0.0", UIMax = "10.0", editcondition = "bOverride_AmbientOcclusionBias", DisplayName = "Bias"))
	float AmbientOcclusionBias;

	/** 0=lowest quality..100=maximum quality, only a few quality levels are implemented, no soft transition */
	UPROPERTY(interp, BlueprintReadWrite, Category=AmbientOcclusion, AdvancedDisplay, meta=(ClampMin = "0.0", UIMax = "100.0", editcondition = "bOverride_AmbientOcclusionQuality", DisplayName = "Quality"))
	float AmbientOcclusionQuality;

	/** Affects the blend over the multiple mips (lower resolution versions) , 0:fully use full resolution, 1::fully use low resolution, around 0.6 seems to be a good value */
	UPROPERTY(interp, BlueprintReadWrite, Category=AmbientOcclusion, AdvancedDisplay, meta=(ClampMin = "0.1", UIMax = "1.0", editcondition = "bOverride_AmbientOcclusionMipBlend", DisplayName = "Mip Blend"))
	float AmbientOcclusionMipBlend;

	/** Affects the radius AO radius scale over the multiple mips (lower resolution versions) */
	UPROPERTY(interp, BlueprintReadWrite, Category=AmbientOcclusion, AdvancedDisplay, meta=(ClampMin = "0.5", UIMax = "4.0", editcondition = "bOverride_AmbientOcclusionMipScale", DisplayName = "Mip Scale"))
	float AmbientOcclusionMipScale;

	/** to tweak the bilateral upsampling when using multiple mips (lower resolution versions) */
	UPROPERTY(interp, BlueprintReadWrite, Category=AmbientOcclusion, AdvancedDisplay, meta=(ClampMin = "0.0", UIMax = "0.1", editcondition = "bOverride_AmbientOcclusionMipThreshold", DisplayName = "Mip Threshold"))
	float AmbientOcclusionMipThreshold;

	/** Adjusts indirect lighting color. (1,1,1) is default. (0,0,0) to disable GI. The show flag 'Global Illumination' must be enabled to use this property. */
	UPROPERTY(interp, BlueprintReadWrite, Category=GlobalIllumination, meta=(editcondition = "bOverride_IndirectLightingColor", DisplayName = "Indirect Lighting Color", HideAlphaChannel))
	FLinearColor IndirectLightingColor;

	/** Scales the indirect lighting contribution. A value of 0 disables GI. Default is 1. The show flag 'Global Illumination' must be enabled to use this property. */
	UPROPERTY(interp, BlueprintReadWrite, Category=GlobalIllumination, meta=(ClampMin = "0", UIMax = "4.0", editcondition = "bOverride_IndirectLightingIntensity", DisplayName = "Indirect Lighting Intensity"))
	float IndirectLightingIntensity;

	/** 0..1=full intensity */
	UPROPERTY(interp, BlueprintReadWrite, Category=SceneColor, meta=(ClampMin = "0", ClampMax = "1.0", editcondition = "bOverride_ColorGradingIntensity", DisplayName = "Color Grading Intensity"))
	float ColorGradingIntensity;

	/** Name of the LUT texture e.g. MyPackage01.LUTNeutral, empty if not used */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SceneColor, meta=(editcondition = "bOverride_ColorGradingLUT", DisplayName = "Color Grading"))
	class UTexture* ColorGradingLUT;

	/** BokehDOF, Simple gaussian, ... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=DepthOfField, meta=(editcondition = "bOverride_DepthOfFieldMethod", DisplayName = "Method"))
	TEnumAsByte<enum EDepthOfFieldMethod> DepthOfFieldMethod;

	/** CircleDOF only: Depth blur km for 50% */
	UPROPERTY(interp, BlueprintReadWrite, Category=DepthOfField, meta=(ClampMin = "0.000001", ClampMax = "100.0", editcondition = "bOverride_DepthOfFieldDepthBlurAmount", DisplayName = "Depth Blur km for 50%"))
	float DepthOfFieldDepthBlurAmount;

	/** CircleDOF only: Depth blur radius in pixels at 1920x */
	UPROPERTY(interp, BlueprintReadWrite, Category=DepthOfField, meta=(ClampMin = "0.0", ClampMax = "4.0", editcondition = "bOverride_DepthOfFieldDepthBlurRadius", DisplayName = "Depth Blur Radius"))
	float DepthOfFieldDepthBlurRadius;
	
	/** CircleDOF only: Defines the opening of the camera lens, Aperture is 1/fstop, typical lens go down to f/1.2 (large opening), larger numbers reduce the DOF effect */
	UPROPERTY(interp, BlueprintReadWrite, Category=DepthOfField, meta=(ClampMin = "1.0", ClampMax = "32.0", editcondition = "bOverride_DepthOfFieldFstop", DisplayName = "Aperture F-stop"))
	float DepthOfFieldFstop;

	/** Distance in which the Depth of Field effect should be sharp, in unreal units (cm) */
	UPROPERTY(interp, BlueprintReadWrite, Category=DepthOfField, meta=(ClampMin = "0.0", UIMin = "1.0", UIMax = "10000.0", editcondition = "bOverride_DepthOfFieldFocalDistance", DisplayName = "Focal Distance"))
	float DepthOfFieldFocalDistance;

	/** Artificial region where all content is in focus, starting after DepthOfFieldFocalDistance, in unreal units  (cm) */
	UPROPERTY(interp, BlueprintReadWrite, Category=DepthOfField, meta=(UIMin = "0.0", UIMax = "10000.0", editcondition = "bOverride_DepthOfFieldFocalRegion", DisplayName = "Focal Region"))
	float DepthOfFieldFocalRegion;

	/** To define the width of the transition region next to the focal region on the near side (cm) */
	UPROPERTY(interp, BlueprintReadWrite, Category=DepthOfField, meta=(UIMin = "0.0", UIMax = "10000.0", editcondition = "bOverride_DepthOfFieldNearTransitionRegion", DisplayName = "Near Transition Region"))
	float DepthOfFieldNearTransitionRegion;

	/** To define the width of the transition region next to the focal region on the near side (cm) */
	UPROPERTY(interp, BlueprintReadWrite, Category=DepthOfField, meta=(UIMin = "0.0", UIMax = "10000.0", editcondition = "bOverride_DepthOfFieldFarTransitionRegion", DisplayName = "Far Transition Region"))
	float DepthOfFieldFarTransitionRegion;

	/** BokehDOF only: To amplify the depth of field effect (like aperture)  0=off */
	UPROPERTY(interp, BlueprintReadWrite, Category=DepthOfField, meta=(ClampMin = "0.0", ClampMax = "2.0", editcondition = "bOverride_DepthOfFieldScale", DisplayName = "Scale"))
	float DepthOfFieldScale;

	/** BokehDOF only: Maximum size of the Depth of Field blur (in percent of the view width) (note: performance cost scales with size*size) */
	UPROPERTY(interp, BlueprintReadWrite, Category=DepthOfField, meta=(UIMin = "0.0", UIMax = "32.0", editcondition = "bOverride_DepthOfFieldMaxBokehSize", DisplayName = "Max Bokeh Size"))
	float DepthOfFieldMaxBokehSize;

	/** Gaussian only: Maximum size of the Depth of Field blur (in percent of the view width) (note: performance cost scales with size) */
	UPROPERTY(interp, BlueprintReadWrite, Category=DepthOfField, meta=(UIMin = "0.0", UIMax = "32.0", editcondition = "bOverride_DepthOfFieldNearBlurSize", DisplayName = "Near Blur Size"))
	float DepthOfFieldNearBlurSize;

	/** Gaussian only: Maximum size of the Depth of Field blur (in percent of the view width) (note: performance cost scales with size) */
	UPROPERTY(interp, BlueprintReadWrite, Category=DepthOfField, meta=(UIMin = "0.0", UIMax = "32.0", editcondition = "bOverride_DepthOfFieldFarBlurSize", DisplayName = "Far Blur Size"))
	float DepthOfFieldFarBlurSize;

	/** Defines the shape of the Bokeh when object get out of focus, cannot be blended */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category=DepthOfField, meta=(editcondition = "bOverride_DepthOfFieldBokehShape", DisplayName = "Shape"))
	class UTexture* DepthOfFieldBokehShape;

	/** Occlusion tweak factor 1 (0.18 to get natural occlusion, 0.4 to solve layer color leaking issues) */
	UPROPERTY(interp, BlueprintReadWrite, Category=DepthOfField, AdvancedDisplay, meta=(ClampMin = "0.0", ClampMax = "1.0", editcondition = "bOverride_DepthOfFieldOcclusion", DisplayName = "Occlusion"))
	float DepthOfFieldOcclusion;
	
	/** Color threshold to do full quality DOF (BokehDOF only) */
	UPROPERTY(interp, BlueprintReadWrite, Category=DepthOfField, AdvancedDisplay, meta=(ClampMin = "0.0", ClampMax = "10.0", editcondition = "bOverride_DepthOfFieldColorThreshold", DisplayName = "Color Threshold"))
	float DepthOfFieldColorThreshold;

	/** Size threshold to do full quality DOF (BokehDOF only) */
	UPROPERTY(interp, BlueprintReadWrite, Category=DepthOfField, AdvancedDisplay, meta=(ClampMin = "0.0", ClampMax = "1.0", editcondition = "bOverride_DepthOfFieldSizeThreshold", DisplayName = "Size Threshold"))
	float DepthOfFieldSizeThreshold;
	
	/** Artificial distance to allow the skybox to be in focus (e.g. 200000), <=0 to switch the feature off, only for GaussianDOF, can cost performance */
	UPROPERTY(interp, BlueprintReadWrite, Category=DepthOfField, AdvancedDisplay, meta=(ClampMin = "0.0", ClampMax = "200000.0", editcondition = "bOverride_DepthOfFieldSkyFocusDistance", DisplayName = "Sky Distance"))
	float DepthOfFieldSkyFocusDistance;
	
	/** Strength of motion blur, 0:off, should be renamed to intensity */
	UPROPERTY(interp, BlueprintReadWrite, Category=MotionBlur, meta=(ClampMin = "0.0", ClampMax = "1.0", editcondition = "bOverride_MotionBlurAmount", DisplayName = "Amount"))
	float MotionBlurAmount;
	/** max distortion caused by motion blur, in percent of the screen width, 0:off */
	UPROPERTY(interp, BlueprintReadWrite, Category=MotionBlur, meta=(ClampMin = "0.0", ClampMax = "100.0", editcondition = "bOverride_MotionBlurMax", DisplayName = "Max"))
	float MotionBlurMax;
	/** The minimum projected screen radius for a primitive to be drawn in the velocity pass, percentage of screen width. smaller numbers cause more draw calls, default: 4% */
	UPROPERTY(interp, BlueprintReadWrite, Category=MotionBlur, AdvancedDisplay, meta=(ClampMin = "0.0", UIMax = "100.0", editcondition = "bOverride_MotionBlurPerObjectSize", DisplayName = "Per Object Size"))
	float MotionBlurPerObjectSize;

	/**
	 * To render with lower or high resolution than it is presented, 
	 * controlled by console variable, 
	 * 100:off, needs to be <99 to get upsampling and lower to get performance,
	 * >100 for super sampling (slower but higher quality), 
	 * only applied in game
	 */
	UPROPERTY(interp, BlueprintReadWrite, Category=Misc, AdvancedDisplay, meta=(ClampMin = "0.0", ClampMax = "400.0", editcondition = "bOverride_ScreenPercentage"))
	float ScreenPercentage;

	/** TemporalAA, FXAA, ... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category=Misc, meta=(editcondition = "bOverride_AntiAliasingMethod", DisplayName = "AA Method"))
	TEnumAsByte<enum EAntiAliasingMethod> AntiAliasingMethod;

	/** Enable/Fade/disable the Screen Space Reflection feature, in percent, avoid numbers between 0 and 1 fo consistency */
	UPROPERTY(interp, BlueprintReadWrite, Category=ScreenSpaceReflections, meta=(ClampMin = "0.0", ClampMax = "100.0", editcondition = "bOverride_ScreenSpaceReflectionIntensity", DisplayName = "Intensity"))
	float ScreenSpaceReflectionIntensity;

	/** 0=lowest quality..100=maximum quality, only a few quality levels are implemented, no soft transition, 50 is the default for better performance. */
	UPROPERTY(interp, BlueprintReadWrite, Category=ScreenSpaceReflections, meta=(ClampMin = "0.0", UIMax = "100.0", editcondition = "bOverride_ScreenSpaceReflectionQuality", DisplayName = "Quality"))
	float ScreenSpaceReflectionQuality;

	/** Until what roughness we fade the screen space reflections, 0.8 works well, smaller can run faster */
	UPROPERTY(interp, BlueprintReadWrite, Category=ScreenSpaceReflections, meta=(ClampMin = "0.01", ClampMax = "1.0", editcondition = "bOverride_ScreenSpaceReflectionMaxRoughness", DisplayName = "Max Roughness"))
	float ScreenSpaceReflectionMaxRoughness;

	// NVCHANGE_BEGIN: Add VXGI

	/** To toggle VXGI Diffuse Tracing (adds fully-dynamic diffuse indirect lighting to the direct lighting) */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (editcondition = "bOverride_VxgiDiffuseTracingEnabled"), DisplayName = "Enable Diffuse Tracing")
		uint32 VxgiDiffuseTracingEnabled : 1;

	/** Intensity multiplier for the diffuse component. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (ClampMin = "0.0", editcondition = "bOverride_VxgiDiffuseTracingIntensity"), DisplayName = "Indirect Lighting Intensity")
		float VxgiDiffuseTracingIntensity;

	/** Intensity multiplier for multi-bounce tracing. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (ClampMin = "0.0", ClampMax = "2.0", editcondition = "bOverride_VxgiMultiBounceIrradianceScale"), DisplayName = "Multi-Bounce Irradiance Scale")
		float VxgiMultiBounceIrradianceScale;

	/** Number of diffuse cones to trace for each fragment, 4 or more. Balances Quality (more cones) vs Performance. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (ClampMin = "1", ClampMax = "128", editcondition = "bOverride_VxgiDiffuseTracingNumCones"), DisplayName = "Number of Cones")
		int32 VxgiDiffuseTracingNumCones;

	/** Automatic diffuse angle computation based on the number of cones. Overrides the value set in DiffuseConeAngle. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (editcondition = "bOverride_bVxgiDiffuseTracingAutoAngle"), DisplayName = "Auto Cone Angle")
		uint32 bVxgiDiffuseTracingAutoAngle : 1;

	/** Tracing sparsity. 1 = dense tracing, 2 or 3 = sparse tracing. Using sparse tracing greatly improves performance in exchange for fine detail quality. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (ClampMin = "1", ClampMax = "4", editcondition = "bOverride_VxgiDiffuseTracingSparsity"), DisplayName = "Tracing Sparsity")
		int32 VxgiDiffuseTracingSparsity;

	/** Cone angle for GI diffuse component evaluation. This value has no effect if autoDiffuseAngle == true. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (ClampMin = "1", ClampMax = "60", editcondition = "bOverride_VxgiDiffuseTracingConeAngle"), DisplayName = "Cone Angle")
		float VxgiDiffuseTracingConeAngle;

	/** Random per-pixel rotation of the diffuse cone set - it helps reduce banding but costs some performance. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (editcondition = "bOverride_bVxgiDiffuseTracingConeRotation"), DisplayName = "Cone Rotation")
		uint32 bVxgiDiffuseTracingConeRotation : 1;

	/** Random per-pixel adjustment of initial tracing offsets for diffuse tracing, also helps reduce banding. This flag is only effective if enableConeRotation == true. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (editcondition = "bOverride_bVxgiDiffuseTracingRandomConeOffsets"), DisplayName = "Random Cone Offsets")
		uint32 bVxgiDiffuseTracingRandomConeOffsets : 1;
	/** Maximum number of samples that can be fetched for each diffuse cone. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (ClampMin = "16", ClampMax = "1024", editcondition = "bOverride_VxgiDiffuseTracingMaxSamples"), DisplayName = "Max Sample Count")
		int32 VxgiDiffuseTracingMaxSamples;

	/** Tracing step for diffuse component. Reasonable values [0.5, 1]. Sampling with lower step produces more stable results at Performance cost. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (ClampMin = "0.01", ClampMax = "2.0", editcondition = "bOverride_VxgiDiffuseTracingStep"), DisplayName = "Tracing Step")
		float VxgiDiffuseTracingStep;

	/** Opacity correction factor for diffuse component. Reasonable values [0.1, 10]. Higher values produce more contrast rendering, overall picture looks darker. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (ClampMin = "0.01", ClampMax = "10.0", editcondition = "bOverride_VxgiDiffuseTracingOpacityCorrectionFactor"), DisplayName = "Opacity Correction Factor")
		float VxgiDiffuseTracingOpacityCorrectionFactor;

	/** A factor that controls linear interpolation between smoothNormal and ray direction. Accepted values are [0, 1]. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (ClampMin = "0.0", ClampMax = "1.0", editcondition = "bOverride_VxgiDiffuseTracingNormalOffsetFactor"), DisplayName = "Cone Offset Along Normal")
		float VxgiDiffuseTracingNormalOffsetFactor;

	/** Bigger factor would move the diffuse cones closer to the surface normal. Reasonable values in [0, 1]. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (ClampMin = "0", ClampMax = "1", editcondition = "bOverride_VxgiDiffuseTracingConeNormalGroupingFactor"), DisplayName = "Cone Grouping Around Normal")
		float VxgiDiffuseTracingConeNormalGroupingFactor;


	/**
	* Optional color for adding occluded directional ambient lighting.
	* The ambient component is added to the diffuse channel, and is NOT multiplied by the Intensity parameter.
	* To get a normalized ambient occlusion only rendering, set AmbientColor to (1,1,1) and Intensity to 0.
	*/
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (editcondition = "bOverride_VxgiDiffuseTracingAmbientColor"), DisplayName = "Ambient Color")
	FLinearColor VxgiDiffuseTracingAmbientColor;

	/** World-space distance at which the contribution of geometry to ambient occlusion will be 10x smaller than near the surface. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (ClampMin = "0.0", editcondition = "bOverride_VxgiDiffuseTracingAmbientRange"), DisplayName = "Ambient Range")
	float VxgiDiffuseTracingAmbientRange;

	/** Environment map to use for diffuse lighting of non-occluded surfaces. Optional. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (editcondition = "bOverride_VxgiDiffuseTracingEnvironmentMap"), DisplayName = "Environment Map")
	class UTextureCube* VxgiDiffuseTracingEnvironmentMap;

	/** Multiplier for environment map lighting in the diffuse channel. The environment map is multiplied by diffuse cones' final transmittance factors. Default value = 0.0. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (editcondition = "bOverride_VxgiDiffuseTracingEnvironmentMapTint", HideAlphaChannel), DisplayName = "Environment Map Tint")
	FLinearColor VxgiDiffuseTracingEnvironmentMapTint;

	/** Uniform bias to reduce false occlusion for diffuse tracing. Reasonable values in [1.0, 4.0]. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (ClampMin = "0.0", editcondition = "bOverride_VxgiDiffuseTracingInitialOffsetBias"), DisplayName = "Initial Offset Bias")
	float VxgiDiffuseTracingInitialOffsetBias;

	/** Bias factor to reduce false occlusion for diffuse tracing linearly with distance. Reasonable values in [1.0, 4.0]. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (ClampMin = "0.0", editcondition = "bOverride_VxgiDiffuseTracingInitialOffsetDistanceFactor"), DisplayName = "Initial Offset Distance Factor")
	float VxgiDiffuseTracingInitialOffsetDistanceFactor;

	/** Enables reuse of diffuse tracing results from the previous frame, to reduce any flickering artifacts. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (editcondition = "bOverride_bVxgiDiffuseTracingTemporalReprojectionEnabled"), DisplayName = "Use Temporal Filtering")
	uint32 bVxgiDiffuseTracingTemporalReprojectionEnabled : 1;

	/**
	* Weight of the reprojected irradiance data relative to newly computed data, Reasonable values in [0.5, 0.9].
	* where 0 means do not use reprojection, and values closer to 1 mean accumulate data over more previous frames.
	*/
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (ClampMin = "0.0", ClampMax = "1.0", editcondition = "bOverride_VxgiDiffuseTracingTemporalReprojectionPreviousFrameWeight"), DisplayName = "Temporal Filter Previous Frame Weight")
	float VxgiDiffuseTracingTemporalReprojectionPreviousFrameWeight;

	/** Maximum distance between two samples for which they're still considered to be the same surface, expressed in voxels. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (ClampMin = "0.0", editcondition = "bOverride_VxgiDiffuseTracingTemporalReprojectionMaxDistanceInVoxels"), DisplayName = "Temporal Filter Max Surface Distance")
	float VxgiDiffuseTracingTemporalReprojectionMaxDistanceInVoxels;

	/** The exponent used for the dot product of old and new normals in the temporal reprojection filter. Set to 0.0 to disable this weight factor (default). */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Diffuse", meta = (ClampMin = "0.0", editcondition = "bOverride_VxgiDiffuseTracingTemporalReprojectionNormalWeightExponent"), DisplayName = "Temporal Filter Normal Difference Exponent")
	float VxgiDiffuseTracingTemporalReprojectionNormalWeightExponent;

	/** To toggle VXGI Specular Tracing (replaces any Reflection Environment or SSR with VXGI Specular Tracing) */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Specular", meta = (editcondition = "bOverride_VxgiSpecularTracingEnabled"), DisplayName = "Enable Specular Tracing")
	uint32 VxgiSpecularTracingEnabled : 1;

	/** Intensity multiplier for the specular component. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Specular", meta = (editcondition = "bOverride_VxgiSpecularTracingIntensity"), DisplayName = "Indirect Lighting Intensity")
	float VxgiSpecularTracingIntensity;

	/** Maximum number of samples that can be fetched for each specular cone. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Specular", meta = (ClampMin = "16", ClampMax = "1024", editcondition = "bOverride_VxgiSpecularTracingMaxSamples"), DisplayName = "Max Sample Count")
	int32 VxgiSpecularTracingMaxSamples;

	/** Tracing step for specular component. Reasonable values [0.5, 1]. Sampling with lower step produces more stable results at Performance cost. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Specular", meta = (ClampMin = "0.01", ClampMax = "2.0", editcondition = "bOverride_VxgiSpecularTracingTracingStep"), DisplayName = "Tracing Step")
	float VxgiSpecularTracingTracingStep;

	/** Opacity correction factor for specular component. Reasonable values [0.1, 10]. Higher values prevent specular cones from tracing through solid objects. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Specular", meta = (ClampMin = "0.01", ClampMax = "10.0", editcondition = "bOverride_VxgiSpecularTracingOpacityCorrectionFactor"), DisplayName = "Opacity Correction Factor")
	float VxgiSpecularTracingOpacityCorrectionFactor;

	/** Uniform bias to avoid false occlusion for specular tracing. Reasonable values in [1.0, 4.0]. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Specular", meta = (ClampMin = "0.0", editcondition = "bOverride_VxgiSpecularTracingInitialOffsetBias"), DisplayName = "Initial Offset Bias")
	float VxgiSpecularTracingInitialOffsetBias;

	/** Bias factor to reduce false occlusion for specular tracing linearly with distance. Reasonable values in [1.0, 4.0]. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Specular", meta = (ClampMin = "0.0", editcondition = "bOverride_VxgiSpecularTracingInitialOffsetDistanceFactor"), DisplayName = "Initial Offset Distance Factor")
	float VxgiSpecularTracingInitialOffsetDistanceFactor;

	/** Enable simple filtering on the specular surface after tracing in order to reduce noise introduced by cone jitter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VXGI Specular", meta = (editcondition = "bOverride_VxgiSpecularTracingFilter"), DisplayName = "Filter")
	TEnumAsByte<enum EVxgiSpecularTracingFilter> VxgiSpecularTracingFilter;

	/** Environment map to use when specular cones don't hit any geometry. Optional. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VXGI Specular", meta = (editcondition = "bOverride_VxgiSpecularTracingEnvironmentMap"), DisplayName = "Environment Map")
	class UTextureCube* VxgiSpecularTracingEnvironmentMap;

	/** Multiplier for environment map reflections in the specular channel. The environment map will only be visible on pixels that do not reflect any solid geometry. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Specular", meta = (editcondition = "bOverride_VxgiSpecularTracingEnvironmentMapTint", HideAlphaChannel), DisplayName = "Environment Map Tint")
	FLinearColor VxgiSpecularTracingEnvironmentMapTint;

	/** [Experimental] Scale of the jitter that can be added to specular sample positions to reduce blockiness of the reflections, in the range [0,1]. */
	UPROPERTY(interp, BlueprintReadWrite, Category = "VXGI Specular", meta = (ClampMin = "0.0", ClampMax = "1.0", editcondition = "bOverride_VxgiSpecularTracingTangentJitterScale"), DisplayName = "Sample Position Jitter")
	float VxgiSpecularTracingTangentJitterScale;

	// NVCHANGE_END: Add VXGI

	// Note: Adding properties before this line require also changes to the OverridePostProcessSettings() function and 
	// FPostProcessSettings constructor and possibly the SetBaseValues() method.
	// -----------------------------------------------------------------------
	
	/**
	 * Allows custom post process materials to be defined, using a MaterialInstance with the same Material as its parent to allow blending.
	 * For materials this needs to be the "PostProcess" domain type. This can be used for any UObject object implementing the IBlendableInterface (e.g. could be used to fade weather settings).
	 */
	UPROPERTY(EditAnywhere, Category="PostProcessSettings", meta=( Keywords="PostProcess", DisplayName = "Blendables" ))
	FWeightedBlendables WeightedBlendables;

	// for backwards compatibility
	UPROPERTY()
	TArray<UObject*> Blendables_DEPRECATED;

	// for backwards compatibility
	void OnAfterLoad()
	{
		for(int32 i = 0, count = Blendables_DEPRECATED.Num(); i < count; ++i)
		{
			if(Blendables_DEPRECATED[i])
			{
				FWeightedBlendable Element(1.0f, Blendables_DEPRECATED[i]);
				WeightedBlendables.Array.Add(Element);
			}
		}
		Blendables_DEPRECATED.Empty();
	}

	// Adds an Blendable (implements IBlendableInterface) to the array of Blendables (if it doesn't exist) and update the weight
	// @param InBlendableObject silently ignores if no object is referenced
	// @param 0..1 InWeight, values outside of the range get clampled later in the pipeline
	void AddBlendable(TScriptInterface<IBlendableInterface> InBlendableObject, float InWeight)
	{
		// update weight, if the Blendable is already in the array
		if(UObject* Object = InBlendableObject.GetObject())
		{
			for (int32 i = 0, count = WeightedBlendables.Array.Num(); i < count; ++i)
			{
				if (WeightedBlendables.Array[i].Object == Object)
				{
					WeightedBlendables.Array[i].Weight = InWeight;
					// We assumes we only have one
					return;
				}
			}

			// add in the end
			WeightedBlendables.Array.Add(FWeightedBlendable(InWeight, Object));
		}
	}

	// removes one or multiple blendables from the array
	void RemoveBlendable(TScriptInterface<IBlendableInterface> InBlendableObject)
	{
		if(UObject* Object = InBlendableObject.GetObject())
		{
			for (int32 i = 0, count = WeightedBlendables.Array.Num(); i < count; ++i)
			{
				if (WeightedBlendables.Array[i].Object == Object)
				{
					// this might remove multiple
					WeightedBlendables.Array.RemoveAt(i);
					--i;
					--count;
				}
			}
		}
	}

	// good start values for a new volume, by default no value is overriding
	FPostProcessSettings()
	{
		// to set all bOverride_.. by default to false
		FMemory::Memzero(this, sizeof(FPostProcessSettings));

		WhiteTemp = 6500.0f;
		WhiteTint = 0.0f;

		ColorSaturation = FVector( 1.0f, 1.0f, 1.0f );
		ColorContrast = FVector( 1.0f, 1.0f, 1.0f );
		ColorGamma = FVector( 1.0f, 1.0f, 1.0f );
		ColorGain = FVector( 1.0f, 1.0f, 1.0f );
		ColorOffset = FVector( 0.0f, 0.0f, 0.0f );

		// default values:
		FilmWhitePoint = FLinearColor(1.0f,1.0f,1.0f);
		FilmSaturation = 1.0f;
		FilmChannelMixerRed = FLinearColor(1.0f,0.0f,0.0f);
		FilmChannelMixerGreen = FLinearColor(0.0f,1.0f,0.0f);
		FilmChannelMixerBlue = FLinearColor(0.0f,0.0f,1.0f);
		FilmContrast = 0.03f;
		FilmDynamicRange = 4.0f;
		FilmHealAmount = 0.18f;
		FilmToeAmount = 1.0f;
		FilmShadowTint = FLinearColor(1.0f,1.0f,1.0f);
		FilmShadowTintBlend = 0.5;
		FilmShadowTintAmount = 0.0;

		// ACES settings
		FilmSlope = 0.88f;
		FilmToe = 0.55f;
		FilmShoulder = 0.26f;
		FilmBlackClip = 0.0f;
		FilmWhiteClip = 0.04f;

		SceneColorTint = FLinearColor(1, 1, 1);
		SceneFringeIntensity = 0.0f;
		SceneFringeSaturation = 0.5f;
		// next value might get overwritten by r.DefaultFeature.Bloom
		BloomIntensity = 1.0f;
		BloomThreshold = 1.0f;
		Bloom1Tint = FLinearColor(0.5f, 0.5f, 0.5f);
		// default is 4 to maintain old settings after fixing something that caused a factor of 4
		BloomSizeScale = 4.0;
		Bloom1Size = 1.0f;
		Bloom2Tint = FLinearColor(0.5f, 0.5f, 0.5f);
		Bloom2Size = 4.0f;
		Bloom3Tint = FLinearColor(0.5f, 0.5f, 0.5f);
		Bloom3Size = 16.0f;
		Bloom4Tint = FLinearColor(0.5f, 0.5f, 0.5f);
		Bloom4Size = 32.0f;
		Bloom5Tint = FLinearColor(0.5f, 0.5f, 0.5f);
		Bloom5Size = 64.0f;
		Bloom6Tint = FLinearColor(0.5f, 0.5f, 0.5f);
		Bloom6Size = 64.0f;
		BloomDirtMaskIntensity = 1.0f;
		BloomDirtMaskTint = FLinearColor(0.5f, 0.5f, 0.5f);
		AmbientCubemapIntensity = 1.0f;
		AmbientCubemapTint = FLinearColor(1, 1, 1);
		LPVIntensity = 1.0f;
		LPVSize = 5312.0f;
		LPVSecondaryOcclusionIntensity = 0.0f;
		LPVSecondaryBounceIntensity = 0.0f;
		LPVVplInjectionBias = 0.64f;
		LPVGeometryVolumeBias = 0.384f;
		LPVEmissiveInjectionIntensity = 1.0f;
		AutoExposureLowPercent = 80.0f;
		AutoExposureHighPercent = 98.3f;
		// next value might get overwritten by r.DefaultFeature.AutoExposure
		AutoExposureMinBrightness = 0.03f;
		// next value might get overwritten by r.DefaultFeature.AutoExposure
		AutoExposureMaxBrightness = 2.0f;
		AutoExposureBias = 0.0f;
		AutoExposureSpeedUp = 3.0f;
		AutoExposureSpeedDown = 1.0f;
		LPVDirectionalOcclusionIntensity = 0.0f;
		LPVDirectionalOcclusionRadius = 8.0f;
		LPVDiffuseOcclusionExponent = 1.0f;
		LPVSpecularOcclusionExponent = 7.0f;
		LPVDiffuseOcclusionIntensity = 1.0f;
		LPVSpecularOcclusionIntensity = 1.0f;
		HistogramLogMin = -8.0f;
		HistogramLogMax = 4.0f;
		// next value might get overwritten by r.DefaultFeature.LensFlare
		LensFlareIntensity = 1.0f;
		LensFlareTint = FLinearColor(1.0f, 1.0f, 1.0f);
		LensFlareBokehSize = 3.0f;
		LensFlareThreshold = 8.0f;
		VignetteIntensity = 0.4f;
		GrainIntensity = 0.0f;
		GrainJitter = 0.0f;
		// NVCHANGE_BEGIN: Add HBAO+
		HBAOPowerExponent = 3.f;
		HBAORadius = 1.f;
		HBAOBias = 0.1f;
		HBAODetailAO = 1.f;
		HBAOBlurRadius = AOBR_BlurRadius4;
		HBAOBlurSharpness = 16.f;
		// NVCHANGE_END: Add HBAO+
		// next value might get overwritten by r.DefaultFeature.AmbientOcclusion
		AmbientOcclusionIntensity = .5f;
		// next value might get overwritten by r.DefaultFeature.AmbientOcclusionStaticFraction
		AmbientOcclusionStaticFraction = 1.0f;
		AmbientOcclusionRadius = 200.0f;
		AmbientOcclusionDistance_DEPRECATED = 80.0f;
		AmbientOcclusionFadeDistance = 8000.0f;
		AmbientOcclusionFadeRadius = 5000.0f;
		AmbientOcclusionPower = 2.0f;
		AmbientOcclusionBias = 3.0f;
		AmbientOcclusionQuality = 50.0f;
		AmbientOcclusionMipBlend = 0.6f;
		AmbientOcclusionMipScale = 1.7f;
		AmbientOcclusionMipThreshold = 0.01f;
		AmbientOcclusionRadiusInWS = false;
		IndirectLightingColor = FLinearColor(1.0f, 1.0f, 1.0f);
		IndirectLightingIntensity = 1.0f;
		ColorGradingIntensity = 1.0f;
		DepthOfFieldFocalDistance = 1000.0f;
		DepthOfFieldFstop = 4.0f;
		DepthOfFieldDepthBlurAmount = 1.0f;
		DepthOfFieldDepthBlurRadius = 0.0f;
		DepthOfFieldFocalRegion = 0.0f;
		DepthOfFieldNearTransitionRegion = 300.0f;
		DepthOfFieldFarTransitionRegion = 500.0f;
		DepthOfFieldScale = 0.0f;
		DepthOfFieldMaxBokehSize = 15.0f;
		DepthOfFieldNearBlurSize = 15.0f;
		DepthOfFieldFarBlurSize = 15.0f;
		DepthOfFieldOcclusion = 0.4f;
		DepthOfFieldColorThreshold = 1.0f;
		DepthOfFieldSizeThreshold = 0.08f;
		DepthOfFieldSkyFocusDistance = 0.0f;
		LensFlareTints[0] = FLinearColor(1.0f, 0.8f, 0.4f, 0.6f);
		LensFlareTints[1] = FLinearColor(1.0f, 1.0f, 0.6f, 0.53f);
		LensFlareTints[2] = FLinearColor(0.8f, 0.8f, 1.0f, 0.46f);
		LensFlareTints[3] = FLinearColor(0.5f, 1.0f, 0.4f, 0.39f);
		LensFlareTints[4] = FLinearColor(0.5f, 0.8f, 1.0f, 0.31f);
		LensFlareTints[5] = FLinearColor(0.9f, 1.0f, 0.8f, 0.27f);
		LensFlareTints[6] = FLinearColor(1.0f, 0.8f, 0.4f, 0.22f);
		LensFlareTints[7] = FLinearColor(0.9f, 0.7f, 0.7f, 0.15f);
		// next value might get overwritten by r.DefaultFeature.MotionBlur
		MotionBlurAmount = 0.5f;
		MotionBlurMax = 5.0f;
		MotionBlurPerObjectSize = 0.5f;
		ScreenPercentage = 100.0f;
		AntiAliasingMethod = AAM_TemporalAA;
		ScreenSpaceReflectionIntensity = 100.0f;
		ScreenSpaceReflectionQuality = 50.0f;
		ScreenSpaceReflectionMaxRoughness = 0.6f;
		// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI
		{
			VXGI::DiffuseTracingParameters DefaultParams;
			VxgiDiffuseTracingEnabled = false;
			VxgiDiffuseTracingIntensity = DefaultParams.irradianceScale;
			VxgiDiffuseTracingNumCones = DefaultParams.numCones;
			bVxgiDiffuseTracingAutoAngle = DefaultParams.autoConeAngle;
			VxgiDiffuseTracingSparsity = DefaultParams.tracingSparsity;
			VxgiDiffuseTracingConeAngle = DefaultParams.coneAngle;
			bVxgiDiffuseTracingConeRotation = DefaultParams.enableConeRotation;
			bVxgiDiffuseTracingRandomConeOffsets = DefaultParams.enableRandomConeOffsets;
			VxgiDiffuseTracingConeNormalGroupingFactor = DefaultParams.coneNormalGroupingFactor;
			VxgiDiffuseTracingMaxSamples = DefaultParams.maxSamples;
			VxgiDiffuseTracingStep = DefaultParams.tracingStep;
			VxgiDiffuseTracingOpacityCorrectionFactor = DefaultParams.opacityCorrectionFactor;
			VxgiDiffuseTracingNormalOffsetFactor = DefaultParams.normalOffsetFactor;
			VxgiDiffuseTracingAmbientColor = FLinearColor(0.f, 0.f, 0.f);
			VxgiDiffuseTracingAmbientRange = DefaultParams.ambientRange;
			VxgiDiffuseTracingInitialOffsetBias = DefaultParams.initialOffsetBias;
			VxgiDiffuseTracingInitialOffsetDistanceFactor = DefaultParams.initialOffsetDistanceFactor;
			bVxgiDiffuseTracingTemporalReprojectionEnabled = DefaultParams.enableTemporalReprojection;
			VxgiDiffuseTracingTemporalReprojectionPreviousFrameWeight = DefaultParams.temporalReprojectionWeight;
			VxgiDiffuseTracingTemporalReprojectionMaxDistanceInVoxels = 1.f;
			VxgiDiffuseTracingTemporalReprojectionNormalWeightExponent = 0.f;
			VxgiDiffuseTracingEnvironmentMapTint = FLinearColor(1.f, 1.f, 1.f, 1.f);
			VxgiDiffuseTracingEnvironmentMap = NULL;

			VXGI::UpdateVoxelizationParameters DefaultUpdateVoxelizationParams;
			VxgiMultiBounceIrradianceScale = DefaultUpdateVoxelizationParams.indirectIrradianceMapTracingParameters.irradianceScale;
		}
		{
			VXGI::SpecularTracingParameters DefaultParams;
			VxgiSpecularTracingEnabled = false;
			VxgiSpecularTracingIntensity = DefaultParams.irradianceScale;
			VxgiSpecularTracingMaxSamples = DefaultParams.maxSamples;
			VxgiSpecularTracingTracingStep = DefaultParams.tracingStep;
			VxgiSpecularTracingOpacityCorrectionFactor = DefaultParams.opacityCorrectionFactor;
			VxgiSpecularTracingInitialOffsetBias = DefaultParams.initialOffsetBias;
			VxgiSpecularTracingInitialOffsetDistanceFactor = DefaultParams.initialOffsetDistanceFactor;
			VxgiSpecularTracingFilter = (EVxgiSpecularTracingFilter)DefaultParams.filter;
			VxgiSpecularTracingEnvironmentMapTint = FLinearColor(1.f, 1.f, 1.f, 1.f);
			VxgiSpecularTracingEnvironmentMap = NULL;
		}
#endif
		// NVCHANGE_END: Add VXGI
	}

	/**
		* Used to define the values before any override happens.
		* Should be as neutral as possible.
		*/		
	void SetBaseValues()
	{
		*this = FPostProcessSettings();

		AmbientCubemapIntensity = 0.0f;
		ColorGradingIntensity = 0.0f;
	}
};

UCLASS()
class UScene : public UObject
{
	GENERATED_UCLASS_BODY()


	/** bits needed to store DPG value */
	#define SDPG_NumBits	3
};