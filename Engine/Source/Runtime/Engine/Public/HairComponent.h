#pragma once

#include "AllowWindowsPlatformTypes.h"
#pragma warning(push)
#pragma warning(disable: 4191)	// For DLL function pointer conversion
#include "../../../ThirdParty/HairWorks/GFSDK_HairWorks.h"
#pragma warning(pop)
#include "HideWindowsPlatformTypes.h"

#include "HairComponent.generated.h"

class UHair;

/**
* HairComponent manages and renders a hair asset.
*/
UCLASS(ClassGroup = Rendering, meta = (BlueprintSpawnableComponent), HideCategories = (Collision, Base, Object, PhysicsVolume))
class ENGINE_API UHairComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

#pragma region Asset
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Asset)
	UHair* Hair;

	UPROPERTY(EditAnywhere, Category = Asset)
	bool bLoadParametersFromAsset = false;
#pragma endregion

#pragma region General
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General", meta = (ClampMin = "1", ClampMax = "4"))
	int32 SplineMultiplier;
#pragma endregion

#pragma region Physical
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Physical)
	bool bSimulate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Physical, meta = (ClampMin = "-50", ClampMax = "50"))
	float MassScale = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Physical, meta = (ClampMin = "0", ClampMax = "1"))
	float Damping = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Physical, meta = (ClampMin = "0", ClampMax = "1"))
	float InertiaScale = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Physical, meta = (ClampMin = "0", ClampMax = "100"))
	float InertiaLimit = 100;
#pragma endregion

#pragma region Wind
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wind, meta = (DisplayName = "Direction"))
	FRotator WindDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wind, meta = (DisplayName = "Strangth"))
	float Wind = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wind, meta = (DisplayName = "Noise", ClampMin = "0", ClampMax = "1"))
	float WindNoise = 0;
#pragma endregion

#pragma region Stiffness
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stiffness, meta = (DisplayName = "Global", ClampMin = "0", ClampMax = "1"))
	float StiffnessGlobal = 0.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stiffness, meta = (DisplayName = "Global Map"))
	UTexture2D* StiffnessGlobalMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stiffness, meta = (DisplayName = "Global Curve"))
	FVector4 StiffnessGlobalCurve = FVector4(1, 1, 1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stiffness, meta = (DisplayName = "Strength", ClampMin = "0", ClampMax = "1"))
	float StiffnessStrength = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stiffness, meta = (DisplayName = "Strength Curve"))
		FVector4 StiffnessStrengthCurve = FVector4(1, 1, 1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stiffness, meta = (DisplayName = "Damping", ClampMin = "0", ClampMax = "1"))
	float StiffnessDamping = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stiffness, meta = (DisplayName = "Damping Curve"))
	FVector4 StiffnessDampingCurve = FVector4(1, 1, 1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stiffness, meta = (DisplayName = "Root", ClampMin = "0", ClampMax = "1"))
	float StiffnessRoot = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stiffness, meta = (DisplayName = "Root Map"))
	UTexture2D* StiffnessRootMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stiffness, meta = (DisplayName = "Tip", ClampMin = "0", ClampMax = "1"))
	float StiffnessTip = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stiffness, meta = (DisplayName = "Bend", ClampMin = "0", ClampMax = "1"))
	float StiffnessBend = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stiffness, meta = (DisplayName = "Bend Curve"))
	FVector4 StiffnessBendCurve = FVector4(1, 1, 1, 1);
#pragma endregion

#pragma region HairCollision
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HairCollision, meta = (ClampMin = "0", ClampMax = "1"))
	float Backstop = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HairCollision, meta = (ClampMin = "0", ClampMax = "10"))
	float Friction = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HairCollision)
	bool bCapsuleCollision = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HairCollision, meta = (DisplayName = "Interaction", ClampMin = "0", ClampMax = "1"))
	float StiffnessInteraction = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HairCollision, meta = (DisplayName = "Interaction Curve"))
	FVector4 StiffnessInteractionCurve = FVector4(1, 1, 1, 1);
#pragma endregion

#pragma region Pin
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pin, meta = (DisplayName = "Stiffness", ClampMin = "0", ClampMax = "1"))
	float PinStiffness = 1;
#pragma endregion

#pragma region Volume
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Volume, meta = (ClampMin = "0", ClampMax = "10"))
	float Density = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Volume)
	UTexture2D* DensityMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Volume)
	bool bUsePixelDensity = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Volume, meta = (ClampMin = "0", ClampMax = "1"))
	float LengthScale = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Volume)
	UTexture2D* LengthScaleMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Volume, meta = (ClampMin = "0", ClampMax = "1"))
	float LengthNoise = 0;
#pragma endregion

#pragma region Strand Width
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StrandWidth, meta = (DisplayName = "Scale", ClampMin = "0", ClampMax = "10"))
	float WidthScale = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StrandWidth, meta = (DisplayName = "Scale Map"))
	UTexture2D* WidthScaleMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StrandWidth, meta = (DisplayName = "Root", ClampMin = "0", ClampMax = "1"))
	float WidthRootScale = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StrandWidth, meta = (DisplayName = "Tip", ClampMin = "0", ClampMax = "1"))
	float WidthTipScale = 0.1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StrandWidth, meta = (DisplayName = "Noise", ClampMin = "0", ClampMax = "1"))
	float WidthNoise = 0;
#pragma endregion

#pragma region Clumping
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Clumping, meta = (DisplayName = "Scale", ClampMin = "0", ClampMax = "1"))
	float ClumpingScale = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Clumping, meta = (DisplayName = "Scale Map"))
	UTexture2D* ClumpingScaleMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Clumping, meta = (DisplayName = "Roundness", ClampMin = "0.01", ClampMax = "2"))
	float ClumpingRoundness = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Clumping, meta = (DisplayName = "Roundness Map"))
	UTexture2D* ClumpingRoundnessMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Clumping, meta = (DisplayName = "Noise", ClampMin = "0", ClampMax = "1"))
	float ClumpingNoise;
#pragma endregion

#pragma region Waviness
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Waviness, meta = (DisplayName = "Scale", ClampMin = "0", ClampMax = "5"))
	float WavinessScale = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Waviness, meta = (DisplayName = "Scale Map"))
	UTexture2D* WavinessScaleMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Waviness, meta = (DisplayName = "Scale Noise", ClampMin = "0", ClampMax = "1"))
	float WavinessScaleNoise = 0.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Waviness, meta = (DisplayName = "Scale Strand", ClampMin = "0", ClampMax = "1"))
	float WavinessScaleStrand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Waviness, meta = (DisplayName = "Scale Clump", ClampMin = "0", ClampMax = "1"))
	float WavinessScaleClump;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Waviness, meta = (DisplayName = "Frequency", ClampMin = "1", ClampMax = "10"))
	float WavinessFreq = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Waviness, meta = (DisplayName = "Frequency Map"))
	UTexture2D* WavinessFreqMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Waviness, meta = (DisplayName = "Frequency Noise", ClampMin = "0", ClampMax = "1"))
	float WavinessFreqNoise = 0.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Waviness, meta = (DisplayName = "Root Straighten", ClampMin = "0", ClampMax = "1"))
	float WavinessRootStraigthen = 0;
#pragma endregion

#pragma region Color
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Color)
	FLinearColor RootColor = FLinearColor(1, 1, 1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Color)
	UTexture2D* RootColorMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Color)
	FLinearColor TipColor = FLinearColor(1, 1, 1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Color)
	UTexture2D* TipColorMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Color, meta = (DisplayName = "Root/Tip Color Weight", ClampMin = "0", ClampMax = "1"))
	float RootTipColorWeight = 0.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Color, meta = (DisplayName = "Root/Tip Color Falloff", ClampMin = "0", ClampMax = "1"))
	float RootTipColorFalloff = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Color, meta = (DisplayName = "Root Alpha Falloff", ClampMin = "0", ClampMax = "1"))
	float RootAlphaFalloff = 0;
#pragma endregion

#pragma region Diffuse
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Diffuse, meta = (ClampMin = "0", ClampMax = "1"))
	float DiffuseBlend = 0.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Diffuse, meta = (ClampMin = "0", ClampMax = "1"))
	float HairNormalWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Diffuse)
	FName HairNormalCenter;
#pragma endregion

#pragma region Specular
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Specular, meta = (DisplayName = "Color"))
	FLinearColor SpecularColor = FLinearColor(1, 1, 1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Specular, meta = (DisplayName = "Color Map"))
	UTexture2D* SpecularColorMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Specular, meta = (ClampMin = "0", ClampMax = "1"))
	float PrimaryScale = 0.1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Specular, meta = (ClampMin = "1", ClampMax = "1000"))
	float PrimaryShininess = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Specular, meta = (ClampMin = "0", ClampMax = "1"))
	float PrimaryBreakup = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Specular, meta = (ClampMin = "0", ClampMax = "1"))
	float SecondaryScale = 0.05;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Specular, meta = (ClampMin = "1", ClampMax = "1000"))
	float SecondaryShininess = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Specular, meta = (ClampMin = "-1", ClampMax = "1"))
	float SecondaryOffset = 0.1;
#pragma endregion

#pragma region Glint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Glint, meta = (DisplayName = "Strength", ClampMin = "0", ClampMax = "1"))
	float GlintStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Glint, meta = (DisplayName = "Size", ClampMin = "1", ClampMax = "1024"))
	float GlintSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Glint, meta = (DisplayName = "Power Exponent", ClampMin = "1", ClampMax = "16"))
	float GlintPowerExponent;
#pragma endregion

#pragma region Shadow
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Shadow, meta = (ClampMin = "0", ClampMax = "1"))
	float ShadowDensityScale = 0.5;
#pragma endregion

#pragma region Culling
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Culling)
	bool bBackfaceCulling = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Culling, meta = (ClampMin = "-1", ClampMax = "1"))
	float BackfaceCullingThreshold = -0.2;
#pragma endregion

#pragma region Distance LOD
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DistanceLOD, meta = (DisplayName = "Enable"))
	bool bDistanceLodEnable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DistanceLOD, meta = (DisplayName = "Start Distance", ClampMin = "0"))
	float DistanceLodStart = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DistanceLOD, meta = (DisplayName = "End Distance", ClampMin = "0"))
	float DistanceLodEnd = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DistanceLOD, meta = (ClampMin = "0", ClampMax = "1000"))
	float FadeStartDistance = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DistanceLOD, meta = (DisplayName = "Base Width Scale", ClampMin = "0", ClampMax = "10"))
	float DistanceLodBaseWidthScale = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DistanceLOD, meta = (DisplayName = "Base Density Scale", ClampMin = "0", ClampMax = "10"))
	float DistanceLodBaseDensityScale = 0;
#pragma endregion

#pragma region Detail LOD
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DetailLOD, meta = (DisplayName = "Enable"))
	bool bDetailLodEnable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DetailLOD, meta = (DisplayName = "Start Distance", ClampMin = "0"))
	float DetailLodStart = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DetailLOD, meta = (DisplayName = "End Distance", ClampMin = "0"))
	float DetailLodEnd = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DetailLOD, meta = (DisplayName = "Base Width Scale", ClampMin = "0", ClampMax = "10"))
	float DetailLodBaseWidthScale = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DetailLOD, meta = (DisplayName = "Base Density Scale", ClampMin = "0", ClampMax = "10"))
	float DetailLodBaseDensityScale = 1;
#pragma endregion

	FPrimitiveSceneProxy* CreateSceneProxy() override;
	void OnAttachmentChanged() override;
	void SendRenderDynamicData_Concurrent() override;
	void CreateRenderState_Concurrent() override;
	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
#if WITH_EDITOR
	void PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	UPROPERTY()
	USkinnedMeshComponent* ParentSkeleton;

	// Send data for rendering
	void SendHairDynamicData()const;

	// Bone mapping
	void SetupBoneMapping()const;

	// Sync parameters with hair
	void SyncHairParameters(GFSDK_HairInstanceDescriptor& HairDescriptor, TArray<FTexture2DRHIRef>& HairTextures, bool bDescriptorToComponent);

	// Default hair parameters from asset
	GFSDK_HairInstanceDescriptor DefaultHairDesc;

	// Bone names and indices.
	TMap<FName, int32> HairBoneToIdxMap;
};
