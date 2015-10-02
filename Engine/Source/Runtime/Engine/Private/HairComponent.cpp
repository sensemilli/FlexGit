#include "EnginePrivate.h"
#include "Hair.h"
#include "HairSceneProxy.h"
#include "HairComponent.h"

void UHairComponent::SyncHairParameters(GFSDK_HairInstanceDescriptor& HairDescriptor, TArray<FTexture2DRHIRef>& HairTextures, bool bDescriptorToComponent)
{
#define SyncParameter(Parameter, Property)	\
	if(bDescriptorToComponent)	\
		Property = HairDescriptor.Parameter;	\
	else	\
		HairDescriptor.Parameter = Property;

	if (!bDescriptorToComponent)
		HairTextures.SetNumZeroed(GFSDK_HAIR_NUM_TEXTURES);

	auto SyncTexture = [&](GFSDK_HAIR_TEXTURE_TYPE TextureType, UTexture2D* Texture)
	{
		if (!Texture || bDescriptorToComponent)
			return;

		HairTextures[TextureType] = static_cast<FTexture2DResource*>(Texture->Resource)->GetTexture2DRHI();
	};

#pragma region General
	SyncParameter(m_splineMultiplier, SplineMultiplier);
#pragma endregion

#pragma region Physical
	SyncParameter(m_simulate, bSimulate);
	FVector GravityDir = FVector(0, 0, -1) * MassScale;
	SyncParameter(m_gravityDir, (gfsdk_float3&)GravityDir);
	if (bDescriptorToComponent)
		MassScale = GravityDir.Size();
	SyncParameter(m_damping, Damping);
	SyncParameter(m_inertiaScale, InertiaScale);
	SyncParameter(m_inertiaLimit, InertiaLimit);
#pragma endregion

#pragma region Wind
	FVector WindVector = WindDirection.Vector() * Wind;
	SyncParameter(m_wind, (gfsdk_float3&)WindVector);
	if (bDescriptorToComponent)
	{
		Wind = WindVector.Size();
		WindDirection = FRotator(FQuat(FRotationMatrix::MakeFromX(WindVector)));
	}
	SyncParameter(m_windNoise, WindNoise);
#pragma endregion

#pragma region Stiffness
	SyncParameter(m_stiffness, StiffnessGlobal);
	SyncTexture(GFSDK_HAIR_TEXTURE_STIFFNESS, StiffnessGlobalMap);
	SyncParameter(m_stiffnessCurve, (gfsdk_float4&)StiffnessGlobalCurve);
	SyncParameter(m_stiffnessStrength, StiffnessStrength);
	SyncParameter(m_stiffnessStrengthCurve, (gfsdk_float4&)StiffnessStrengthCurve);
	SyncParameter(m_stiffnessDamping, StiffnessDamping);
	SyncParameter(m_stiffnessDampingCurve, (gfsdk_float4&)StiffnessDampingCurve);
	SyncParameter(m_rootStiffness, StiffnessRoot);
	SyncTexture(GFSDK_HAIR_TEXTURE_ROOT_STIFFNESS, StiffnessRootMap);
	SyncParameter(m_tipStiffness, StiffnessTip);
	SyncParameter(m_bendStiffness, StiffnessBend);
	SyncParameter(m_bendStiffnessCurve, (gfsdk_float4&)StiffnessBendCurve);
#pragma endregion

#pragma region Collision
	SyncParameter(m_backStopRadius, Backstop);
	SyncParameter(m_friction, Friction);
	SyncParameter(m_useCollision, bCapsuleCollision);
	SyncParameter(m_interactionStiffness, StiffnessInteraction);
	SyncParameter(m_interactionStiffnessCurve, (gfsdk_float4&)StiffnessInteractionCurve);
#pragma endregion

#pragma region Pin
	SyncParameter(m_pinStiffness, PinStiffness);
#pragma endregion

#pragma region Volume
	SyncParameter(m_density, Density);
	SyncTexture(GFSDK_HAIR_TEXTURE_DENSITY, DensityMap);
	SyncParameter(m_usePixelDensity, bUsePixelDensity);
	SyncParameter(m_lengthScale, LengthScale);
	SyncTexture(GFSDK_HAIR_TEXTURE_LENGTH, LengthScaleMap);
	SyncParameter(m_lengthNoise, LengthNoise);
#pragma endregion

#pragma region Strand Width
	SyncParameter(m_width, WidthScale);
	SyncTexture(GFSDK_HAIR_TEXTURE_WIDTH, WidthScaleMap);
	SyncParameter(m_widthRootScale, WidthRootScale);
	SyncParameter(m_widthTipScale, WidthTipScale);
	SyncParameter(m_widthNoise, WidthNoise);
#pragma endregion

#pragma region Clumping
	SyncParameter(m_clumpScale, ClumpingScale);
	SyncTexture(GFSDK_HAIR_TEXTURE_CLUMP_SCALE, ClumpingScaleMap);
	SyncParameter(m_clumpRoundness, ClumpingRoundness);
	SyncTexture(GFSDK_HAIR_TEXTURE_CLUMP_ROUNDNESS, ClumpingRoundnessMap);
	SyncParameter(m_clumpNoise, ClumpingNoise);
#pragma endregion

#pragma region Waveness
	SyncParameter(m_waveScale, WavinessScale);
	SyncTexture(GFSDK_HAIR_TEXTURE_WAVE_SCALE, WavinessScaleMap);
	SyncParameter(m_waveScaleNoise, WavinessScaleNoise);
	SyncParameter(m_waveScaleStrand, WavinessScaleStrand);
	SyncParameter(m_waveScaleClump, WavinessScaleClump);
	SyncParameter(m_waveFreq, WavinessFreq);
	SyncTexture(GFSDK_HAIR_TEXTURE_WAVE_FREQ, WavinessFreqMap);
	SyncParameter(m_waveFreqNoise, WavinessFreqNoise);
	SyncParameter(m_waveRootStraighten, WavinessRootStraigthen);
#pragma endregion

#pragma region Color
	SyncParameter(m_rootColor, (gfsdk_float4&)RootColor);
	SyncTexture(GFSDK_HAIR_TEXTURE_ROOT_COLOR, RootColorMap);
	SyncParameter(m_tipColor, (gfsdk_float4&)TipColor);
	SyncTexture(GFSDK_HAIR_TEXTURE_TIP_COLOR, TipColorMap);
	SyncParameter(m_rootTipColorWeight, RootTipColorWeight);
	SyncParameter(m_rootTipColorFalloff, RootTipColorFalloff);
	SyncParameter(m_rootAlphaFalloff, RootAlphaFalloff);
#pragma endregion

#pragma region Diffuse
	SyncParameter(m_diffuseBlend, DiffuseBlend);
	SyncParameter(m_hairNormalWeight, HairNormalWeight);

	if (bDescriptorToComponent)
	{
		for (auto& Ele : HairBoneToIdxMap)
		{
			if (Ele.Value == HairDescriptor.m_hairNormalBoneIndex)
			{
				HairNormalCenter = Ele.Key;
				break;
			}
		}
	}
	else
	{
		if (HairNormalCenter.IsNone())
			HairDescriptor.m_hairNormalWeight = 0;
		else
		{
			auto* BoneIdx = HairBoneToIdxMap.Find(HairNormalCenter);
			if (BoneIdx)
				HairDescriptor.m_hairNormalBoneIndex = *BoneIdx;
		}
	}
#pragma endregion

#pragma region Specular
	SyncParameter(m_specularColor, (gfsdk_float4&)SpecularColor);
	SyncTexture(GFSDK_HAIR_TEXTURE_SPECULAR, SpecularColorMap);
	SyncParameter(m_specularPrimary, PrimaryScale);
	SyncParameter(m_specularPowerPrimary, PrimaryShininess);
	SyncParameter(m_specularPrimaryBreakup, PrimaryBreakup);
	SyncParameter(m_specularSecondary, SecondaryScale);
	SyncParameter(m_specularPowerSecondary, SecondaryShininess);
	SyncParameter(m_specularSecondaryOffset, SecondaryOffset);
#pragma endregion

#pragma region Glint
	SyncParameter(m_glintStrength, GlintStrength);
	SyncParameter(m_glintCount, GlintSize);
	SyncParameter(m_glintExponent, GlintPowerExponent);
#pragma endregion

#pragma region Shadow
	SyncParameter(m_shadowDensityScale, ShadowDensityScale);
#pragma endregion

#pragma region Culling
	SyncParameter(m_useBackfaceCulling, bBackfaceCulling);
	SyncParameter(m_backfaceCullingThreshold, BackfaceCullingThreshold);
#pragma endregion

#pragma region LOD
	if (!bDescriptorToComponent)
		HairDescriptor.m_enableLOD = true;
#pragma endregion

#pragma region Distance LOD
	SyncParameter(m_enableDistanceLOD, bDistanceLodEnable);
	SyncParameter(m_distanceLODStart, DistanceLodStart);
	SyncParameter(m_distanceLODEnd, DistanceLodEnd);
	SyncParameter(m_distanceLODFadeStart, FadeStartDistance);
	SyncParameter(m_distanceLODWidth, DistanceLodBaseWidthScale);
	SyncParameter(m_distanceLODDensity, DistanceLodBaseDensityScale);
#pragma endregion

#pragma region Detail LOD
	SyncParameter(m_enableDetailLOD, bDetailLodEnable);
	SyncParameter(m_detailLODStart, DetailLodStart);
	SyncParameter(m_detailLODEnd, DetailLodEnd);
	SyncParameter(m_detailLODWidth, DetailLodBaseWidthScale);
	SyncParameter(m_detailLODDensity, DetailLodBaseDensityScale);
#pragma endregion
}

UHairComponent::UHairComponent(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	// No need to select
	bSelectable = false;

	// Simplify shadow
	CastShadow = true;
	bAffectDynamicIndirectLighting = false;
	bAffectDistanceFieldLighting = false;

	// Setup tick
	bAutoActivate = true;
	bTickInEditor = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;	// Just mark render data dirty every frame

	// Set properties' default values
	GFSDK_HairInstanceDescriptor HairDescriptor;
	TArray<FTexture2DRHIRef> HairTextures;
	SyncHairParameters(HairDescriptor, HairTextures, true);
}

FPrimitiveSceneProxy* UHairComponent::CreateSceneProxy()
{
	if (!Hair)
		return nullptr;

	return new FHairSceneProxy(this, Hair);
}

void UHairComponent::OnAttachmentChanged()
{
	// Parent as skeleton
	ParentSkeleton = Cast<USkinnedMeshComponent>(AttachParent);

	// Update proxy
	SetupBoneMapping();
	SendHairDynamicData();	// For correct initial visual effect
}

FBoxSphereBounds UHairComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (!SceneProxy)
		return FBoxSphereBounds(EForceInit::ForceInit);

	auto& HairProxy = static_cast<FHairSceneProxy&>(*SceneProxy);

	FBoxSphereBounds HairBounds(EForceInit::ForceInit);
	HairProxy.GetHairBounds_GameThread(HairBounds);

	return HairBounds.TransformBy(LocalToWorld);
}

void UHairComponent::SendRenderDynamicData_Concurrent()
{
	Super::SendRenderDynamicData_Concurrent();

	// Send data for rendering
	SendHairDynamicData();
}

void UHairComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Send data every frame
	if (SceneProxy)
	{
#if WITH_EDITOR
		if (!(GetWorld() && GetWorld()->bPostTickComponentUpdate))
			MarkRenderTransformDirty();	// Update scene cached bounds.
#endif

		MarkRenderDynamicDataDirty();
	}
}

void UHairComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

	// Update static bound
	if (SceneProxy)
	{
		FlushRenderingCommands();	// Ensure hair is created.

		auto& HairProxy = static_cast<FHairSceneProxy&>(*SceneProxy);

		if (Hair)
			HairProxy.GetHairInfo_GameThread(DefaultHairDesc, HairBoneToIdxMap, *Hair);

		UpdateBounds();

		if (!(GetWorld() && GetWorld()->bPostTickComponentUpdate))
			MarkRenderTransformDirty();	// Update scene cached bounds.
	}

	// Update proxy
	SetupBoneMapping();
	SendHairDynamicData();	// Ensure correct visual effect at first frame.
}

void UHairComponent::SendHairDynamicData()const
{
	// Update parameters
	if (!SceneProxy)
		return;

	auto& HairSceneProxy = static_cast<FHairSceneProxy&>(*SceneProxy);

	if (ParentSkeleton)
		HairSceneProxy.UpdateBones_GameThread(*ParentSkeleton);

	GFSDK_HairInstanceDescriptor HairDesc = DefaultHairDesc;
	TArray<FTexture2DRHIRef> HairTextures;

	const_cast<UHairComponent&>(*this).SyncHairParameters(HairDesc, HairTextures, false);

	// Send paramters
	HairSceneProxy.UpdateHairParams_GameThread(HairDesc, HairTextures);
}

void UHairComponent::SetupBoneMapping()const
{
	// Setup bone mapping
	if (!SceneProxy || !ParentSkeleton || !ParentSkeleton->SkeletalMesh)
		return;

	auto& HairSceneProxy = static_cast<FHairSceneProxy&>(*SceneProxy);
	HairSceneProxy.SetupBoneMapping_GameThread(ParentSkeleton->SkeletalMesh->RefSkeleton.GetRefBoneInfo());
}

#if WITH_EDITOR
void UHairComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Copy parameters from a hair asset.
	if (bLoadParametersFromAsset 
		&& PropertyChangedEvent.MemberProperty 
		&& PropertyChangedEvent.MemberProperty->GetName() == "bLoadParametersFromAsset"
		)
	{
		bLoadParametersFromAsset = false;

		if (Hair)
		{
			if (FHairSceneProxy::GetHairInfo_GameThread(DefaultHairDesc, HairBoneToIdxMap, *Hair))	// Proxy may have not been created yet. So we get information from asset.
			{
				TArray<FTexture2DRHIRef> HairTextures;
				SyncHairParameters(DefaultHairDesc, HairTextures, true);
			}
		}
	}
}
#endif