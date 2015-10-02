#pragma once

#include "PhysXSupport.h"
#include "StaticMeshResources.h"

#if WITH_FLEX

// if true GPU skinning will be used for soft bodies on SM4+ devices
#define USE_FLEX_GPU_SKINNING 1	

struct FFlexShapeTransform
{
	FVector Translation;
	FQuat Rotation;
};

class FFlexVertexBuffer : public FVertexBuffer
{
public:

	int NumVerts;

	void Init(int Count);

	virtual ~FFlexVertexBuffer() {}
	virtual void InitRHI() override;
};

// Flex vertex factories override the local vertex factory and modify the position stream
class FFlexVertexFactory : public FLocalVertexFactory
{
public:

	virtual void SkinCloth(const FVector4* SimulatedPositions, const FVector* SimulatedNormals, const int* VertexToParticleMap) = 0;
	virtual void SkinSoft(const FPositionVertexBuffer& Positions, const FStaticMeshVertexBuffer& Vertices, const FFlexShapeTransform* Transforms, const FVector* RestPoses, const int16* ClusterIndices, const float* ClusterWeights, int NumClusters) = 0;
};

// Overrides local vertex factory with CPU skinned deformation
class FFlexCPUVertexFactory : public FFlexVertexFactory
{
public:
	
	FFlexCPUVertexFactory(const FLocalVertexFactory& Base, int NumVerts);
	virtual ~FFlexCPUVertexFactory();

	virtual void SkinCloth(const FVector4* SimulatedPositions, const FVector* SimulatedNormals, const int* VertexToParticleMap) override;
	virtual void SkinSoft(const FPositionVertexBuffer& Positions, const FStaticMeshVertexBuffer& Vertices, const FFlexShapeTransform* Transforms, const FVector* RestPoses, const int16* ClusterIndices, const float* ClusterWeights, int NumClusters) override;

	// Stores CPU skinned positions and normals to override default static mesh stream
	FFlexVertexBuffer VertexBuffer;
};

class FFlexGPUVertexFactory : public FFlexVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FFlexVertexFactory);

public:
	
	struct FlexDataType
	{
		/** Skinning weights for clusters */
		FVertexStreamComponent ClusterWeights;

		/** Skinning indices for clusters */
		FVertexStreamComponent ClusterIndices;
	};

	/** Should we cache the material's shadertype on this platform with this vertex factory? */
	static bool ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
	{
		return  IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4) &&
				(Material->IsUsedWithFlexMeshes() || Material->IsSpecialEngineMaterial()) && 
				FLocalVertexFactory::ShouldCache(Platform, Material, ShaderType);
	}

	/** Modify compile environment to enable flex cluster deformation */
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FLocalVertexFactory::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("USE_FLEX_DEFORM"), TEXT("1"));
	}

	static FVertexFactoryShaderParameters* ConstructShaderParameters(EShaderFrequency ShaderFrequency);
	
public:

	FFlexGPUVertexFactory(const FLocalVertexFactory& Base, const FVertexBuffer* ClusterWeightsVertexBuffer, const FVertexBuffer* ClusterIndicesVertexBuffer);
	virtual ~FFlexGPUVertexFactory();

	virtual void AddVertexElements(DataType& InData, FVertexDeclarationElementList& Elements) override;
	virtual void AddVertexPositionElements(DataType& Data, FVertexDeclarationElementList& Elements) override;

	virtual void InitDynamicRHI();
	virtual void ReleaseDynamicRHI();

	void AllocateFor(int32 InMaxClusters);

	// FFlexVertexFactory methods
	virtual void SkinCloth(const FVector4* SimulatedPositions, const FVector* SimulatedNormals, const int* VertexToParticleMap) override;
	virtual void SkinSoft(const FPositionVertexBuffer& Positions, const FStaticMeshVertexBuffer& Vertices, const FFlexShapeTransform* Transforms, const FVector* RestPoses, const int16* ClusterIndices, const float* ClusterWeights, int NumClusters) override;

	int MaxClusters;

	FReadBuffer ClusterTranslations;
	FReadBuffer ClusterRotations;

protected:

	FlexDataType FlexData;
};

/** Scene proxy, overrides default static mesh behavior */
class FFlexMeshSceneProxy : public FStaticMeshSceneProxy
{
public:

	FFlexMeshSceneProxy(UStaticMeshComponent* Component);
	virtual ~FFlexMeshSceneProxy();

	int GetLastVisibleFrame();

	void UpdateClothTransforms();
	void UpdateSoftTransforms(const FFlexShapeTransform* Transforms, int32 NumShapes);

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) override;
	//virtual void PreRenderView(const FSceneViewFamily* ViewFamily, const uint32 VisibilityMap, int32 FrameNumber) override;
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, class FMeshElementCollector& Collector) const override;
		 
	virtual bool GetMeshElement(int32 LODIndex, int32 BatchIndex, int32 ElementIndex, uint8 InDepthPriorityGroup, const bool bUseSelectedMaterial, const bool bUseHoveredMaterial, FMeshBatch& OutMeshBatch) const;
	virtual bool GetShadowMeshElement(int32 LODIndex, int32 BatchIndex, uint8 InDepthPriorityGroup, FMeshBatch& OutMeshBatch, bool bDitheredLODTransition) const;

	FFlexVertexFactory* VertexFactory;

	UFlexComponent* FlexComponent;
	uint32 LastFrame;

	// shape transforms sent from game thread
	const FFlexShapeTransform* ShapeTransforms;
};

#endif // WITH FLEX