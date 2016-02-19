#pragma once

#include "AllowWindowsPlatformTypes.h"

#pragma warning(push)
#pragma warning(disable: 4191)	// For DLL function pointer conversion
#include "../../../ThirdParty/HairWorks/GFSDK_HairWorks.h"
#pragma warning(pop)

#include "HideWindowsPlatformTypes.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHairWorks, Log, All);

class UHair;

/**
* Hair compoenent scene proxy.
*/
class ENGINE_API FHairSceneProxy : public FPrimitiveSceneProxy
{
public:
	FHairSceneProxy(const UPrimitiveComponent* InComponent, UHair* Hair);
	~FHairSceneProxy();

	uint32 GetMemoryFootprint(void) const override;
	void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, class FMeshElementCollector& Collector) const override;
	void CreateRenderThreadResources() override;
	FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) override;

	void SetupBoneMapping_GameThread(const TArray<FMeshBoneInfo>& Bones);
	bool GetHairBounds_GameThread(FBoxSphereBounds& Bounds)const;
	void UpdateBones_GameThread(USkinnedMeshComponent& ParentSkeleton);
	void UpdateHairParams_GameThread(const GFSDK_HairInstanceDescriptor& HairDesc, const TArray<FTexture2DRHIRef>& HairTextures);

	void DrawBasePass(const FSceneView& View);
	void DrawShadow(const FViewMatrices& ViewMatrices, float DepthBias, float Scale);
	void DrawTranslucency(const FSceneView& View, const FVector& LightDir, const FLinearColor& LightColor, FTextureRHIRef LightAttenuation, const FVector4 IndirectLight[3]);

	static bool IsHair_GameThread(const void* AssetData, unsigned DataSize);
	static bool GetHairInfo_GameThread(GFSDK_HairInstanceDescriptor& HairDescriptor, TMap<FName, int32>& BoneToIdxMap, const UHair& Hair);
	static void ReleaseHair_GameThread(GFSDK_HairAssetID AssetId);

	static void StartMsaa();
	static void FinishMsaa();
	static void DrawColorAndDepth();

protected:
	void SetupBoneMapping_RenderThread(const TArray<FMeshBoneInfo>& Bones);
	void UpdateBones_RenderThread(const TArray<FMatrix>& RefMatrices);
	void UpdateHairParams_RenderThread(const GFSDK_HairInstanceDescriptor& HairDesc, const TArray<FTexture2DRHIRef>& HairTextures);
	void UpdateShaderCache();
	static void StepSimulation();

	// The APEX asset data
	UHair* Hair;

	// The hair
	GFSDK_HairInstanceID HairInstanceId = GFSDK_HairInstanceID_NULL;

	// Hair parameters
	GFSDK_HairInstanceDescriptor HairDesc;

	// Control textures
	TArray<FTexture2DRHIRef> HairTextures;

	// Bone mapping indices
	TArray<int> BoneMapping;
};