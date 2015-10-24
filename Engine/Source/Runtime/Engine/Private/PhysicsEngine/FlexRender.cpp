// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#include "FlexRender.h"
#include "PhysicsEngine/FlexComponent.h"

#if WITH_FLEX

#if STATS
DECLARE_CYCLE_STAT(TEXT("Skin Mesh Time (CPU)"), STAT_Flex_RenderMeshTime, STATGROUP_Flex);
#endif


/* CPU Skinning */

struct FFlexVertex
{
	FVector Position;
	FPackedNormal TangentZ;
};


void FFlexVertexBuffer::Init(int Count)
{
	NumVerts = Count;

	BeginInitResource(this);
}

void FFlexVertexBuffer::InitRHI()
{
	// Create the vertex buffer.
	FRHIResourceCreateInfo CreateInfo;
	VertexBufferRHI = RHICreateVertexBuffer( NumVerts*sizeof(FFlexVertex), BUF_AnyDynamic, CreateInfo);
}


// Overrides local vertex factory with CPU skinned deformation
FFlexCPUVertexFactory::FFlexCPUVertexFactory(const FLocalVertexFactory& Base, int NumVerts)
{
	VertexBuffer.Init(NumVerts);

	// have to first initialize our RHI and then recreate it from the static mesh
	BeginInitResource(this);

	// copy vertex factory from LOD0 of staticmesh
	Copy(Base);

	// update position and normal components to point to our vertex buffer
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		InitFlexVertexFactory,
		FFlexCPUVertexFactory*, Factory, this,
		const FFlexVertexBuffer*, VertexBuffer, &VertexBuffer,
	{
		Factory->Data.PositionComponent = FVertexStreamComponent(
			VertexBuffer,	
			STRUCT_OFFSET(FFlexVertex,Position),
			sizeof(FFlexVertex),
			VET_Float3
			);
			
		/*
		Data->TangentBasisComponents[0] = FVertexStreamComponent(
			VertexBuffer,
			STRUCT_OFFSET(FFlexVertex,TangentX),
			sizeof(FFlexVertex),
			VET_Float3
			);
		*/

		Factory->Data.TangentBasisComponents[1] = FVertexStreamComponent(
			VertexBuffer,
			STRUCT_OFFSET(FFlexVertex,TangentZ),
			sizeof(FFlexVertex),
			VET_PackedNormal
			);

		Factory->UpdateRHI();
	});
}

FFlexCPUVertexFactory::~FFlexCPUVertexFactory()
{
	VertexBuffer.ReleaseResource();

	ReleaseResource();
}

void FFlexCPUVertexFactory::SkinCloth(const FVector4* SimulatedPositions, const FVector* SimulatedNormals, const int* VertexToParticleMap)
{
	SCOPE_CYCLE_COUNTER(STAT_Flex_RenderMeshTime);

	FFlexVertex* RESTRICT Vertex = (FFlexVertex*)RHILockVertexBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.NumVerts*sizeof(FFlexVertex), RLM_WriteOnly);

	if (SimulatedPositions && SimulatedNormals)
	{
		// update both positions and normals
		for (int i=0; i < VertexBuffer.NumVerts; ++i)
		{
			const int particleIndex = VertexToParticleMap[i];

			Vertex[i].Position = FVector(SimulatedPositions[particleIndex]);
			
			// convert normal to packed format
			FPackedNormal Normal = -FVector(SimulatedNormals[particleIndex]);
			Normal.Vector.W = 255;
			Vertex[i].TangentZ = Normal;
		}
	}

	RHIUnlockVertexBuffer(VertexBuffer.VertexBufferRHI);		
}

void FFlexCPUVertexFactory::SkinSoft(const FPositionVertexBuffer& Positions, const FStaticMeshVertexBuffer& Vertices, const FFlexShapeTransform* Transforms, const FVector* RestPoses, const int16* ClusterIndices, const float* ClusterWeights, int NumClusters)
{
	SCOPE_CYCLE_COUNTER(STAT_Flex_RenderMeshTime);

	const int NumVertices = Vertices.GetNumVertices();

	TArray<FFlexVertex> SkinnedVertices;
	SkinnedVertices.SetNum(NumVertices);

	for (int VertexIndex=0; VertexIndex < NumVertices; ++VertexIndex)
	{
		FVector SoftPos(0.0f);
		FVector SoftNormal(0.0f);
		FVector SoftTangent(0.0f);

		for (int w=0; w < 4; ++ w)
		{
			const int Cluster = ClusterIndices[VertexIndex*4 + w];
			const float Weight = ClusterWeights[VertexIndex*4 + w];

			if (Cluster > -1)
			{
				const FFlexShapeTransform Transform = Transforms[Cluster];

				FVector LocalPos = Positions.VertexPosition(VertexIndex) - RestPoses[Cluster];
				FVector LocalNormal = Vertices.VertexTangentZ(VertexIndex);
				//FVector LocalTangent = Vertices.VertexTangentX(VertexIndex);
			
				SoftPos += (Transform.Rotation.RotateVector(LocalPos) + Transform.Translation)*Weight;
				SoftNormal += (Transform.Rotation.RotateVector(LocalNormal))*Weight;
				//SoftTangent += Rotation.RotateVector(LocalTangent)*Weight;
			}
		}
			
		// position
		SkinnedVertices[VertexIndex].Position = SoftPos;

		// normal
		FPackedNormal Normal = SoftNormal;
		Normal.Vector.W = 255;
		SkinnedVertices[VertexIndex].TangentZ = Normal;

		// tangent
		//FPackedNormal Tangent = -SoftTangent;
		//Tangent.Vector.W = 255;
		//SkinnedTangents[VertexIndex] = Tangent;
	}

	FFlexVertex* RESTRICT Vertex = (FFlexVertex*)RHILockVertexBuffer(VertexBuffer.VertexBufferRHI, 0, NumVertices*sizeof(FFlexVertex), RLM_WriteOnly);
	FMemory::Memcpy(Vertex, &SkinnedVertices[0], sizeof(FFlexVertex)*NumVertices);
	RHIUnlockVertexBuffer(VertexBuffer.VertexBufferRHI);		
}

/* GPU Skinning */

class FFlexMeshVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
	virtual void Bind(const FShaderParameterMap& ParameterMap) override;
	virtual void SetMesh(FRHICommandList& RHICmdList, FShader* Shader, const FVertexFactory* VertexFactory, const FSceneView& View, const FMeshBatchElement& BatchElement, uint32 DataFlags) const override;
	void Serialize(FArchive& Ar) override;
	virtual uint32 GetSize() const override;

private:

	FShaderResourceParameter ClusterTranslationsParameter;
	FShaderResourceParameter ClusterRotationsParameter;
};

FVertexFactoryShaderParameters* FFlexGPUVertexFactory::ConstructShaderParameters(EShaderFrequency ShaderFrequency)
{
	return ShaderFrequency == SF_Vertex ? new FFlexMeshVertexFactoryShaderParameters() : NULL;
}

FFlexGPUVertexFactory::FFlexGPUVertexFactory(const FLocalVertexFactory& Base, const FVertexBuffer* ClusterWeightsVertexBuffer, const FVertexBuffer* ClusterIndicesVertexBuffer)
{
	// set our streams
	FlexData.ClusterWeights = FVertexStreamComponent(ClusterWeightsVertexBuffer, 0, sizeof(float)*4, VET_Float4);
	FlexData.ClusterIndices = FVertexStreamComponent(ClusterIndicesVertexBuffer, 0,	sizeof(int16)*4, VET_Short4);

	MaxClusters = 0;

	// have to first initialize our RHI and then recreate it from the static mesh
	BeginInitResource(this);

	// copy vertex factory from LOD0 of staticmesh
	Copy(Base);
}

FFlexGPUVertexFactory::~FFlexGPUVertexFactory()
{
	ReleaseResource();
}

void FFlexGPUVertexFactory::AddVertexElements(DataType& InData, FVertexDeclarationElementList& Elements)
{
	FLocalVertexFactory::AddVertexElements(Data, Elements);

	// Add Flex elements
	Elements.Add(AccessStreamComponent(FlexData.ClusterIndices, 8));
	Elements.Add(AccessStreamComponent(FlexData.ClusterWeights, 9));
	// And Flex elements
}

void FFlexGPUVertexFactory::AddVertexPositionElements(DataType& Data, FVertexDeclarationElementList& Elements)
{
	FLocalVertexFactory::AddVertexElements(Data, Elements);

	// Add Flex elements
	Elements.Add(AccessStreamComponent(FlexData.ClusterIndices, 8));
	Elements.Add(AccessStreamComponent(FlexData.ClusterWeights, 9));
	// And Flex elements
}

void FFlexGPUVertexFactory::InitDynamicRHI()
{
	if (MaxClusters > 0)
	{
		ClusterTranslations.Initialize(sizeof(FVector4), MaxClusters, PF_A32B32G32R32F, BUF_AnyDynamic);
		ClusterRotations.Initialize(sizeof(FVector4), MaxClusters, PF_A32B32G32R32F, BUF_AnyDynamic);			
	}
}

void FFlexGPUVertexFactory::ReleaseDynamicRHI()
{
	if (ClusterTranslations.NumBytes > 0)
	{
		ClusterTranslations.Release();
		ClusterRotations.Release();
	}
}

void FFlexGPUVertexFactory::AllocateFor(int32 InMaxClusters)
{
	if (InMaxClusters > MaxClusters)
	{
		MaxClusters = InMaxClusters;

		if (!IsInitialized())
		{
			InitResource();
		}
		else
		{
			UpdateRHI();
		}
	}
}

void FFlexGPUVertexFactory::SkinCloth(const FVector4* SimulatedPositions, const FVector* SimulatedNormals, const int* VertexToParticleMap)
{
	// todo: implement
	check(0);
}

// for GPU skinning this method just uploads the necessary data to the skinning buffers
void FFlexGPUVertexFactory::SkinSoft(const FPositionVertexBuffer& Positions, const FStaticMeshVertexBuffer& Vertices, const FFlexShapeTransform* Transforms, const FVector* RestPoses, const int16* ClusterIndices, const float* ClusterWeights, int NumClusters)
{
	SCOPE_CYCLE_COUNTER(STAT_Flex_RenderMeshTime);
	
	AllocateFor(NumClusters);

	if(NumClusters)
	{
		// remove rest pose translation now, rest pose rotation is always the identity so we can send those directly (below)
		FVector4* TranslationData = (FVector4*)RHILockVertexBuffer(ClusterTranslations.Buffer, 0, NumClusters*sizeof(FVector4), RLM_WriteOnly);			
			
		for (int i=0; i < NumClusters; ++i)
			TranslationData[i] = FVector4(Transforms[i].Translation - Transforms[i].Rotation*RestPoses[i], 0.0f);
			
		RHIUnlockVertexBuffer(ClusterTranslations.Buffer);

		// rotations
		FQuat* RotationData = (FQuat*)RHILockVertexBuffer(ClusterRotations.Buffer, 0, NumClusters*sizeof(FQuat), RLM_WriteOnly);
			
		for (int i=0; i < NumClusters; ++i)
			RotationData[i] = Transforms[i].Rotation;

		RHIUnlockVertexBuffer(ClusterRotations.Buffer);
	}
}

// factory shader parameter implementation methods

void FFlexMeshVertexFactoryShaderParameters::Bind(const FShaderParameterMap& ParameterMap) 
{
	ClusterTranslationsParameter.Bind(ParameterMap, TEXT("ClusterTranslations"), SPF_Optional);
	ClusterRotationsParameter.Bind(ParameterMap, TEXT("ClusterRotations"), SPF_Optional);
}

void FFlexMeshVertexFactoryShaderParameters::SetMesh(FRHICommandList& RHICmdList, FShader* Shader, const FVertexFactory* VertexFactory, const FSceneView& View, const FMeshBatchElement& BatchElement, uint32 DataFlags) const 
{
	if (Shader->GetVertexShader())
	{
		FFlexGPUVertexFactory* Factory = (FFlexGPUVertexFactory*)(VertexFactory);

		if(ClusterTranslationsParameter.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(Shader->GetVertexShader(), ClusterTranslationsParameter.GetBaseIndex(), Factory->ClusterTranslations.SRV);
		}

		if(ClusterRotationsParameter.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(Shader->GetVertexShader(), ClusterRotationsParameter.GetBaseIndex(), Factory->ClusterRotations.SRV);
		}
	}
}

void FFlexMeshVertexFactoryShaderParameters::Serialize(FArchive& Ar) 
{
	Ar << ClusterTranslationsParameter;
	Ar << ClusterRotationsParameter;
}

uint32 FFlexMeshVertexFactoryShaderParameters::GetSize() const 
{
	return sizeof(*this);
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FFlexGPUVertexFactory, "LocalVertexFactory", true, true, true, true, true);


/** Scene proxy */
FFlexMeshSceneProxy::FFlexMeshSceneProxy(UStaticMeshComponent* Component)
	: FStaticMeshSceneProxy(Component)
{
	FlexComponent = Cast<UFlexComponent>(Component);
	
	const FStaticMeshLODResources& LOD = Component->StaticMesh->RenderData->LODResources[0];
	const UFlexAssetSoft* SoftAsset = Cast<UFlexAssetSoft>(FlexComponent->StaticMesh->FlexAsset);

	ERHIFeatureLevel::Type FeatureLevel = Component->GetWorld()->FeatureLevel;

	if (FeatureLevel >= ERHIFeatureLevel::SM4 && SoftAsset)
	{
		// Ensure top LOD has a compatible material 
		FLODInfo& ProxyLODInfo = LODs[0];
		for (int i=0; i < ProxyLODInfo.Sections.Num(); ++i)
		{
			if (!ProxyLODInfo.Sections[i].Material->CheckMaterialUsage(MATUSAGE_FlexMeshes))
				ProxyLODInfo.Sections[i].Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}

		// use GPU skinning for SM4 and soft assets only
		VertexFactory = new FFlexGPUVertexFactory(LOD.VertexFactory, &SoftAsset->WeightsVertexBuffer, &SoftAsset->IndicesVertexBuffer);
	}
	else
	{
		// use CPU skinning for everything eles
		VertexFactory = new FFlexCPUVertexFactory(LOD.VertexFactory, LOD.VertexBuffer.GetNumVertices());
	}

	ShapeTransforms = NULL;
	LastFrame = 0;
}

FFlexMeshSceneProxy::~FFlexMeshSceneProxy()
{
	check(IsInRenderingThread());

	delete VertexFactory;

	delete[] ShapeTransforms;
}

int FFlexMeshSceneProxy::GetLastVisibleFrame()
{
	// called by the game thread to determine whether to disable simulation
	return LastFrame;
}

FPrimitiveViewRelevance FFlexMeshSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance Relevance = FStaticMeshSceneProxy::GetViewRelevance(View);
	Relevance.bDynamicRelevance = true;
	Relevance.bStaticRelevance = false;

	return Relevance;
}
	
void FFlexMeshSceneProxy::UpdateClothTransforms()
{	
	// unsafe: update vertex buffers here by grabbing data directly from simulation container 
	// this won't be necessary when skinning is done on the GPU
	VertexFactory->SkinCloth(
		&FlexComponent->SimPositions[0],
		&FlexComponent->SimNormals[0],
		&FlexComponent->StaticMesh->FlexAsset->VertexToParticleMap[0]);
}

void FFlexMeshSceneProxy::UpdateSoftTransforms(const FFlexShapeTransform* NewTransforms, int32 NumShapes)
{
	// delete old transforms
	delete[] ShapeTransforms;
	ShapeTransforms = NewTransforms;

	const UFlexAssetSoft* SoftAsset = Cast<UFlexAssetSoft>(FlexComponent->StaticMesh->FlexAsset);

	const FPositionVertexBuffer& Positions = FlexComponent->StaticMesh->RenderData->LODResources[0].PositionVertexBuffer;
	const FStaticMeshVertexBuffer& Vertices = FlexComponent->StaticMesh->RenderData->LODResources[0].VertexBuffer;			

	// only used for CPU skinning
	const int16* ClusterIndices = &SoftAsset->IndicesVertexBuffer.Vertices[0];
	const float* ClusterWeights = &SoftAsset->WeightsVertexBuffer.Vertices[0];
	const FVector* RestPoses = &SoftAsset->ShapeCenters[0];

	VertexFactory->SkinSoft(Positions, Vertices, ShapeTransforms, RestPoses, ClusterIndices, ClusterWeights, NumShapes);
}

void FFlexMeshSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, class FMeshElementCollector& Collector) const 
{
	// store last renderer frame (used for LOD), is the const_cast valid here?
	const_cast<FFlexMeshSceneProxy*>(this)->LastFrame = GFrameNumber;

	FStaticMeshSceneProxy::GetDynamicMeshElements(Views, ViewFamily, VisibilityMap, Collector);
}

bool FFlexMeshSceneProxy::GetMeshElement(int32 LODIndex, int32 BatchIndex, int32 ElementIndex, uint8 InDepthPriorityGroup, const bool bUseSelectedMaterial, const bool bUseHoveredMaterial, FMeshBatch& OutMeshBatch) const
{
	bool Ret = FStaticMeshSceneProxy::GetMeshElement(LODIndex, BatchIndex, ElementIndex, InDepthPriorityGroup, bUseSelectedMaterial, bUseHoveredMaterial, OutMeshBatch);

	// override top LOD with our simulated vertex factory
	if (LODIndex == 0)
		OutMeshBatch.VertexFactory = VertexFactory;
	
	return Ret;
}

bool FFlexMeshSceneProxy::GetShadowMeshElement(int32 LODIndex, int32 BatchIndex, uint8 InDepthPriorityGroup, FMeshBatch& OutMeshBatch, bool bDitheredLODTransition) const
{
	return false;
}

#endif // WITH_FLEX
