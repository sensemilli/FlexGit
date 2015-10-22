
#include "EnginePrivate.h"

// FWaveWorksVertexFactory

void FWaveWorksVertexFactory::Init(const FWaveWorksVertexBuffer* VertexBuffer)
{
	if (IsInRenderingThread())
	{
		DataType NewData;
		NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FWaveWorksVertex, Position, VET_Float2);
		SetData(NewData);
	}
	else
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			InitVertexFactory,
			FWaveWorksVertexFactory*, VertexFactory, this,
			const FWaveWorksVertexBuffer*, VertexBuffer, VertexBuffer,
			{
				DataType NewData;
				NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FWaveWorksVertex, Position, VET_Float2);
				VertexFactory->SetData(NewData);
			}
		);
	}
}

bool FWaveWorksVertexFactory::ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
{
	return true;
}

void FWaveWorksVertexFactory::ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
{
	OutEnvironment.SetDefine(TEXT("WITH_GFSDK_WAVEWORKS"), TEXT("1"));
	OutEnvironment.SetDefine(TEXT("GFSDK_WAVEWORKS_USE_TESSELLATION"), TEXT("1"));
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FWaveWorksVertexFactory, "LocalVertexFactory", true, false, true, true, true);

// FWaveWorksSceneProxy

FWaveWorksSceneProxy::FWaveWorksSceneProxy(const UWaveWorksComponent* InComponent, UWaveWorks* InWaveWorks)
	: FPrimitiveSceneProxy(InComponent)
	, WaveWorksMaterial(InComponent->WaveWorksMaterial)
	, WaveWorksResource(InWaveWorks->GetWaveWorksResource())
	, QuadTreeHandle(0)
{
	bWaveWorks = true;

	FWaveWorksRHIRef WaveWorksRHI = WaveWorksResource->GetWaveWorksRHI();
	WaveWorksRHI->CreateQuadTree(
		&QuadTreeHandle,
		InComponent->MeshDim,
		InComponent->MinPatchLength,
		(uint32) InComponent->AutoRootLOD,
		InComponent->UpperGridCoverage,
		InComponent->SeaLevel,
		InComponent->UseTessellation && (GMaxRHIFeatureLevel >= ERHIFeatureLevel::SM5),
		InComponent->TessellationLOD,
		InComponent->GeoMorphingDegree
		);

	/* Add temp vert */
	VertexBuffer.Vertices.Add(FWaveWorksVertex());
	VertexBuffer.Vertices.Add(FWaveWorksVertex());
	VertexBuffer.Vertices.Add(FWaveWorksVertex());

	VertexFactory.Init(&VertexBuffer);
	BeginInitResource(&VertexFactory);
	BeginInitResource(&VertexBuffer);
}

FWaveWorksSceneProxy::~FWaveWorksSceneProxy()
{
	FWaveWorksRHIRef WaveWorksRHI = WaveWorksResource->GetWaveWorksRHI();
	WaveWorksRHI->DestroyQuadTree(QuadTreeHandle);

	VertexBuffer.ReleaseResource();
	VertexFactory.ReleaseResource();
}

uint32 FWaveWorksSceneProxy::GetMemoryFootprint(void) const
{
	return 0;
}

FPrimitiveViewRelevance FWaveWorksSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance ViewRelevance;
	ViewRelevance.bDrawRelevance = true;
	ViewRelevance.bDynamicRelevance = true;
	ViewRelevance.bRenderInMainPass = true;
	ViewRelevance.bOpaqueRelevance = true;
	return ViewRelevance;
}

void FWaveWorksSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, class FMeshElementCollector& Collector) const
{
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			const FSceneView* View = Views[ViewIndex];

			/* Create a dummy mesh (This will not actually be rendered) */
			FMeshBatch& Mesh = Collector.AllocateMesh();

			/* Create batch */
			Mesh.LODIndex = 0;
			Mesh.UseDynamicData = false;
			Mesh.DynamicVertexStride = 0;
			Mesh.DynamicVertexData = NULL;
			Mesh.VertexFactory = &VertexFactory;
			Mesh.ReverseCulling = false;
			Mesh.bDisableBackfaceCulling = true;
			Mesh.bWireframe = ViewFamily.EngineShowFlags.Wireframe;
			Mesh.Type = PT_3_ControlPointPatchList;
			Mesh.DepthPriorityGroup = SDPG_World;
			Mesh.MaterialRenderProxy = WaveWorksMaterial->GetRenderProxy(IsSelected(), IsHovered());

			/* Create element */
			FMeshBatchElement& BatchElement = Mesh.Elements[0];

			BatchElement.PrimitiveUniformBufferResource = &GetUniformBuffer();
			BatchElement.IndexBuffer = nullptr;
			BatchElement.FirstIndex = 0;
			BatchElement.NumPrimitives = 1;
			BatchElement.MinVertexIndex = 0;
			BatchElement.MaxVertexIndex = 1;

			Collector.AddMesh(ViewIndex, Mesh);
		}
	}
}

/** */
void FWaveWorksSceneProxy::SampleDisplacements_GameThread(TArray<FVector2D> InSamplePoints, TArray<FVector4>& OutDisplacements)
{
	if (!WaveWorksResource)
		return;

	FWaveWorksRHIRef WaveWorksRHI = WaveWorksResource->GetWaveWorksRHI();
	WaveWorksRHI->GetDisplacements(InSamplePoints, OutDisplacements);
}