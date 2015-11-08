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

void FFlexParticleSpriteVertexFactory::ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
{
	FParticleSpriteVertexFactory::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
}

FVertexFactoryShaderParameters* FFlexParticleSpriteVertexFactory::ConstructShaderParameters(EShaderFrequency ShaderFrequency)
{
	return FParticleSpriteVertexFactory::ConstructShaderParameters(ShaderFrequency);
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FFlexParticleSpriteVertexFactory, "ParticleSpriteVertexFactory", true, false, true, false, false);

class FFlexDynamicSpriteCollectorResources : public FOneFrameResource
{
public:

	FFlexParticleSpriteVertexFactory VertexFactory;
	FParticleSpriteUniformBufferRef UniformBuffer;

	virtual ~FFlexDynamicSpriteCollectorResources()
	{
		VertexFactory.ReleaseResource();
	}
};

FFlexParticleSceneProxy::FFlexParticleSceneProxy(UFlexComponent* Component)
	: FPrimitiveSceneProxy(Component)
	, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
	, Material(Component->ParticleMaterial)
	, DiffuseMaterial(Component->DiffuseParticleMaterial)
	, bFlexParticleFluid(Component->FluidSurfaceTemplate != nullptr)
{
	if (Material == nullptr)
	{
		Material = UMaterial::GetDefaultMaterial(MD_Surface);
	}

	Particles.AddZeroed(Component->ParticleIndices.Num());
	for (int32 i = 0; i < Particles.Num(); i++)
	{
		Particles[i] = Component->ContainerInstance->Particles[Component->ParticleIndices[i]];
	}
	
	int32 NumBatches = FMath::RoundToInt(Particles.Num());
	ParticleUserData.SetNum(NumBatches);

	ParticleRadius = Component->ContainerTemplate->Radius;
	DiffuseParticleScale = Component->DiffuseParticleScale;

}

/* Editor constructor for preview mode */
FFlexParticleSceneProxy::FFlexParticleSceneProxy(UFlexComponent* Component, TArray<FVector> InPositions)
	: FFlexParticleSceneProxy(Component)
{
	Particles.AddZeroed(InPositions.Num());
	for (int32 i = 0; i < Particles.Num(); i++)
	{
		Particles[i] = FVector4(InPositions[i], 1.f);
	}

	int32 NumBatches = FMath::RoundToInt(Particles.Num());
	ParticleUserData.SetNum(NumBatches);
}

FFlexParticleSceneProxy::~FFlexParticleSceneProxy()
{
}

FPrimitiveViewRelevance FFlexParticleSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance Relevance;
	Relevance.bDrawRelevance = IsShown(View);
	Relevance.bShadowRelevance = IsShadowCast(View);
	Relevance.bDynamicRelevance = true;
	Relevance.bStaticRelevance = false;
	Relevance.bRenderInMainPass = ShouldRenderInMainPass();
	Relevance.bRenderCustomDepth = ShouldRenderCustomDepth();
	Relevance.bNormalTranslucencyRelevance = (DiffuseMaterial && DiffuseMaterial->GetBlendMode() == EBlendMode::BLEND_Translucent);
	//MaterialRelevance.SetPrimitiveViewRelevance(Relevance);

	return Relevance;
}

void FFlexParticleSceneProxy::RenderParticles(TArray<FVector4> InParticles, UMaterialInterface* InMaterial, float Scale, const FSceneView* View, int32 ViewIndex, FMeshElementCollector& Collector, bool bIsDiffuse) const
{
	int32 TotalParticleCount = InParticles.Num();
	int32 NumLoops = (TotalParticleCount / 16384) + 1;

	//struct FCompareParticleOrderZ
	//{
	//	FORCEINLINE bool operator()(const FParticleOrder& A, const FParticleOrder& B) const { return B.Z < A.Z; }
	//};

	///* Sort particles by depth */
	//FParticleOrder* ParticleOrder = GParticleOrderPool.GetParticleOrderData(TotalParticleCount);

	//for (int32 ParticleIdx = 0; ParticleIdx < InParticles.Num(); ParticleIdx++)
	//{
	//	FVector4& Particle = InParticles[ParticleIdx];
	//	float InZ = View->ViewProjectionMatrix.TransformPosition(Particle).W;

	//	ParticleOrder[ParticleIdx].ParticleIndex = ParticleIdx;
	//	ParticleOrder[ParticleIdx].Z = InZ;
	//}
	//Sort(ParticleOrder, TotalParticleCount, FCompareParticleOrderZ());

	for (int32 LoopIndex = 0; LoopIndex < NumLoops; LoopIndex++)
	{
		int32 NumParticles = FMath::Min(TotalParticleCount - (LoopIndex * 16384), 16384);
		if (NumParticles == 0)
			continue;

		int32 Offset = LoopIndex * 16384;
		
		FFlexParticleUserData* UserData = nullptr;
		if (!bIsDiffuse)
		{
			UserData = (FFlexParticleUserData*)&ParticleUserData[LoopIndex];
			UserData->ParticleOffset = Offset;
			UserData->ParticleCount = NumParticles;
		}

		FFlexDynamicSpriteCollectorResources& CollectorResources = Collector.AllocateOneFrameResource<FFlexDynamicSpriteCollectorResources>();
		CollectorResources.VertexFactory.SetParticleFactoryType(PVFT_Sprite);
		CollectorResources.VertexFactory.SetFeatureLevel(ERHIFeatureLevel::SM5);
		CollectorResources.VertexFactory.InitResource();

		FGlobalDynamicVertexBuffer::FAllocation Allocation;
		FGlobalDynamicVertexBuffer::FAllocation DynamicParameterAllocation;

		Allocation = FGlobalDynamicVertexBuffer::Get().Allocate(NumParticles * (sizeof(FParticleSpriteVertex)));
		if (Allocation.IsValid())
		{
			int32 VertexStride = sizeof(FParticleSpriteVertex);
			uint8* TempVert = (uint8*)Allocation.Buffer;

			/* Setup particle buffer */
			for (int32 j = 0; j < NumParticles; j++)
			{
				FParticleSpriteVertex* FillVertex = (FParticleSpriteVertex*)TempVert;

				//FillVertex->Position = InParticles[ParticleOrder[Offset + j].ParticleIndex];
				FillVertex->Position = InParticles[Offset + j];
				FillVertex->RelativeTime = 0.0f;
				FillVertex->OldPosition = FVector::ZeroVector;
				FillVertex->ParticleId = (float)((Offset + j) / TotalParticleCount);
				FillVertex->Size = FVector2D(ParticleRadius, ParticleRadius) * Scale;
				FillVertex->Rotation = 0.0f;
				FillVertex->SubImageIndex = 0.0f;
				FillVertex->Color = FLinearColor::White;

				TempVert += VertexStride;
			}

			/* Setup per view params */
			FParticleSpriteUniformParameters PerViewUniformParameters;
			PerViewUniformParameters.AxisLockRight = FVector::ZeroVector;
			PerViewUniformParameters.AxisLockUp = FVector::ZeroVector;
			PerViewUniformParameters.RotationScale = 1.0f;
			PerViewUniformParameters.RotationBias = 0.0f;
			PerViewUniformParameters.TangentSelector = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
			PerViewUniformParameters.InvDeltaSeconds = 30.0f;
			PerViewUniformParameters.SubImageSize = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
			PerViewUniformParameters.NormalsType = 0.0f;
			PerViewUniformParameters.NormalsSphereCenter = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
			PerViewUniformParameters.NormalsCylinderUnitDirection = FVector4(0.0f, 0.0f, 1.0f, 0.0f);
			PerViewUniformParameters.PivotOffset = FVector2D(-0.5f, -0.5f);
			PerViewUniformParameters.MacroUVParameters = FVector4(0.0f, 0.0f, 1.0f, 1.0f);

			CollectorResources.UniformBuffer = FParticleSpriteUniformBufferRef::CreateUniformBufferImmediate(PerViewUniformParameters, UniformBuffer_SingleFrame);
			CollectorResources.VertexFactory.SetSpriteUniformBuffer(CollectorResources.UniformBuffer);
			CollectorResources.VertexFactory.SetInstanceBuffer(Allocation.VertexBuffer, Allocation.VertexOffset, sizeof(FParticleSpriteVertex), true);
			CollectorResources.VertexFactory.SetDynamicParameterBuffer(DynamicParameterAllocation.VertexBuffer, DynamicParameterAllocation.VertexOffset, sizeof(FParticleVertexDynamicParameter), true);

			FMeshBatch& ParticleMesh = Collector.AllocateMesh();
			FMeshBatchElement& ParticleBatchElement = ParticleMesh.Elements[0];

			ParticleMesh.VertexFactory = &CollectorResources.VertexFactory;
			ParticleMesh.Type = PT_TriangleList;
			ParticleMesh.bDisableBackfaceCulling = InMaterial->IsTwoSided(false);
			ParticleMesh.MaterialRenderProxy = InMaterial->GetRenderProxy(false, false);
			ParticleMesh.bFlexFluidParticles = bFlexParticleFluid && !bIsDiffuse;

			ParticleBatchElement.UserData = (void*) UserData;
			ParticleBatchElement.IndexBuffer = &GFlexParticleIndexBuffer;
			ParticleBatchElement.NumPrimitives = 2;
			ParticleBatchElement.NumInstances = NumParticles;
			ParticleBatchElement.FirstIndex = 0;
			ParticleBatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(FMatrix::Identity, GetBounds(), GetLocalBounds(), false, false);
			ParticleBatchElement.MinVertexIndex = 0;
			ParticleBatchElement.MaxVertexIndex = (NumParticles * 4) - 1;

			Collector.AddMesh(ViewIndex, ParticleMesh);
		}
	}
}

void FFlexParticleSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			const FSceneView* View = Views[ViewIndex];

			/* Render standard particles */
			RenderParticles(Particles, Material, 1.f, View, ViewIndex, Collector, false);

			/* Diffuse only render if they exist */
			if (DiffuseParticles.Num() > 0 && DiffuseMaterial != nullptr)
			{
				/* Render diffuse particles if there are any */
				RenderParticles(DiffuseParticles, DiffuseMaterial, DiffuseParticleScale, View, ViewIndex, Collector, true);
			}
		}
	}
}

uint32 FFlexParticleSceneProxy::GetMemoryFootprint(void) const
{
	return sizeof(*this);
}

/* Update particle positions */
void FFlexParticleSceneProxy::UpdateParticlePositions(TArray<FVector4>& InPositions)
{
	if (InPositions.Num() != Particles.Num())
	{
		Particles.SetNum(InPositions.Num());

		int32 NumBatches = FMath::RoundToInt(Particles.Num());
		ParticleUserData.SetNum(NumBatches);
	}

	for (int32 i = 0; i < InPositions.Num(); i++)
	{
		Particles[i] = InPositions[i];
	}
}

void FFlexParticleSceneProxy::UpdateDiffuseParticlePositions(TArray<FVector4>& InDiffusePositions)
{
	if (InDiffusePositions.Num() != DiffuseParticles.Num())
	{
		DiffuseParticles.SetNum(InDiffusePositions.Num());
	}

	for (int32 i = 0; i < InDiffusePositions.Num(); i++)
	{
		DiffuseParticles[i] = InDiffusePositions[i];
	}
}

void FFlexParticleSceneProxy::UpdateAnisotropy(TArray<FVector4>& InAnisotropy1, TArray<FVector4>& InAnisotropy2, TArray<FVector4>& InAnisotropy3)
{
	if (InAnisotropy1.Num() != Anisotropy1.Num())
	{
		Anisotropy1.SetNum(InAnisotropy1.Num());
		Anisotropy2.SetNum(InAnisotropy2.Num());
		Anisotropy3.SetNum(InAnisotropy3.Num());
	}

	for (int32 i = 0; i < InAnisotropy1.Num(); i++)
	{
		Anisotropy1[i] = InAnisotropy1[i];
		Anisotropy2[i] = InAnisotropy2[i];
		Anisotropy3[i] = InAnisotropy3[i];
	}
}

void FFlexParticleIndexBuffer::InitRHI()
{
	// Instanced path needs only MAX_PARTICLES_PER_INSTANCE,
	// but using the maximum needed for the non-instanced path
	// in prep for future flipping of both instanced and non-instanced at runtime.
	const uint32 MaxParticles = 65536 / 4;
	const uint32 Size = sizeof(uint16) * 6 * MaxParticles;
	const uint32 Stride = sizeof(uint16);
	FRHIResourceCreateInfo CreateInfo;
	IndexBufferRHI = RHICreateIndexBuffer(Stride, Size, BUF_Static, CreateInfo);
	uint16* Indices = (uint16*)RHILockIndexBuffer(IndexBufferRHI, 0, Size, RLM_WriteOnly);
	for (uint32 SpriteIndex = 0; SpriteIndex < MaxParticles; ++SpriteIndex)
	{
		Indices[SpriteIndex * 6 + 0] = SpriteIndex * 4 + 0;
		Indices[SpriteIndex * 6 + 1] = SpriteIndex * 4 + 2;
		Indices[SpriteIndex * 6 + 2] = SpriteIndex * 4 + 3;
		Indices[SpriteIndex * 6 + 3] = SpriteIndex * 4 + 0;
		Indices[SpriteIndex * 6 + 4] = SpriteIndex * 4 + 1;
		Indices[SpriteIndex * 6 + 5] = SpriteIndex * 4 + 2;
	}
	RHIUnlockIndexBuffer(IndexBufferRHI);
}

/** Global particle index buffer. */
TGlobalResource<FFlexParticleIndexBuffer> GFlexParticleIndexBuffer;

#endif // WITH_FLEX
