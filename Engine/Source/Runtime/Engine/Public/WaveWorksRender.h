
#pragma once

DECLARE_LOG_CATEGORY_EXTERN(LogWaveWorks, Log, All);

struct FWaveWorksVertex
{
	FWaveWorksVertex()
		: Position(FVector2D::ZeroVector)
	{
	}

	FVector2D Position;
};

class FWaveWorksVertexBuffer : public FVertexBuffer
{
public:

	/** Vertices */
	TArray<FWaveWorksVertex> Vertices;

	/** Initialise vertex buffer */
	virtual void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.Num() * sizeof(FWaveWorksVertex), BUF_Static, CreateInfo);

		void* VertexBufferData = RHILockVertexBuffer(VertexBufferRHI, 0, Vertices.Num() * sizeof(FWaveWorksVertex), RLM_WriteOnly);
		FMemory::Memcpy(VertexBufferData, Vertices.GetData(), Vertices.Num() * sizeof(FWaveWorksVertex));
		RHIUnlockVertexBuffer(VertexBufferRHI);

		Vertices.Empty();
	}
};

class FWaveWorksVertexFactory : public FLocalVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FWaveWorksVertexFactory)

public:

	/** Default constructor */
	FWaveWorksVertexFactory() {}

	/** Initialisation */
	void Init(const FWaveWorksVertexBuffer* VertexBuffer);

	/** Should cache */
	static bool ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType);

	/** Adds custom defines */
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment);
};

/**
* WaveWorks Scene Proxy
*/
class ENGINE_API FWaveWorksSceneProxy : public FPrimitiveSceneProxy
{
public:
	
	/* Default constructor */
	FWaveWorksSceneProxy(const class UWaveWorksComponent* InComponent, class UWaveWorks* InWaveWorks);
	~FWaveWorksSceneProxy();

	/* Begin FPrimitiveSceneProxy Interface */
	uint32 GetMemoryFootprint(void) const override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) override;
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, class FMeshElementCollector& Collector) const override;
	/* End FPrimitiveSceneProxy Interface */

public:

	/** The WaveWorks object */
	class FWaveWorksResource* WaveWorksResource;

	/** */
	class UMaterialInterface* WaveWorksMaterial;

	/** */
	struct GFSDK_WaveWorks_Quadtree* QuadTreeHandle;

	FWaveWorksVertexFactory VertexFactory;
	FWaveWorksVertexBuffer VertexBuffer;
};
