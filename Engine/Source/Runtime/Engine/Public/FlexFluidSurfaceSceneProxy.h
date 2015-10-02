// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
FlexFluidSurfaceSceneProxy.h:
=============================================================================*/

/*=============================================================================
FFlexFluidSurfaceTextures, Encapsulates the textures used for scene rendering. TODO, move to some better place
=============================================================================*/
struct FFlexFluidSurfaceTextures
{
public:
	FFlexFluidSurfaceTextures() : BufferSize(0, 0)	{}

	~FFlexFluidSurfaceTextures() {}

	// Current buffer size
	FIntPoint BufferSize;

	// Intermediate results
	TRefCountPtr<IPooledRenderTarget> Depth;
	TRefCountPtr<IPooledRenderTarget> DepthStencil;
	TRefCountPtr<IPooledRenderTarget> Thickness;
	TRefCountPtr<IPooledRenderTarget> SmoothDepth;

	// Helper functions
	const FTexture2DRHIRef& GetDepthSurface() const { return (const FTexture2DRHIRef&)Depth->GetRenderTargetItem().TargetableTexture; }
	const FTexture2DRHIRef& GetDepthStencilSurface() const { return (const FTexture2DRHIRef&)DepthStencil->GetRenderTargetItem().TargetableTexture; }
	const FTexture2DRHIRef& GetThicknessSurface() const { return (const FTexture2DRHIRef&)Thickness->GetRenderTargetItem().TargetableTexture; }
	const FTexture2DRHIRef& GetSmoothDepthSurface() const { return (const FTexture2DRHIRef&)SmoothDepth->GetRenderTargetItem().TargetableTexture; }

	const FTexture2DRHIRef& GetDepthTexture() const { return (const FTexture2DRHIRef&)Depth->GetRenderTargetItem().ShaderResourceTexture; }
	const FTexture2DRHIRef& GetDepthStencilTexture() const { return (const FTexture2DRHIRef&)DepthStencil->GetRenderTargetItem().ShaderResourceTexture; }
	const FTexture2DRHIRef& GetThicknessTexture() const { return (const FTexture2DRHIRef&)Thickness->GetRenderTargetItem().ShaderResourceTexture; }
	const FTexture2DRHIRef& GetSmoothDepthTexture() const { return (const FTexture2DRHIRef&)SmoothDepth->GetRenderTargetItem().ShaderResourceTexture; }
};

/*=============================================================================
FFlexFluidSurfaceSceneProxy
=============================================================================*/

#define MAX_EMITTERS_PER_FLEX_FLUID_SURFACE 64

struct FSurfaceParticleMesh
{
	const FParticleSystemSceneProxy* PSysSceneProxy;
	const FDynamicEmitterDataBase* DynamicEmitterData;
	const FMeshBatch* Mesh;
};

struct FFlexFluidSurfaceProperties
{
	float SmoothingRadius;
	int32 MaxRadialSamples;
	float DepthEdgeFalloff;
	float ThicknessParticleScale;
	float DepthParticleScale;
	bool ReceiveShadows;
	UMaterialInterface* Material;
};

class FFlexFluidSurfaceSceneProxy : public FPrimitiveSceneProxy
{
public:

	FFlexFluidSurfaceSceneProxy(UFlexFluidSurfaceComponent* Component);

	virtual ~FFlexFluidSurfaceSceneProxy();

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View) {}

	virtual void CreateRenderThreadResources() override;

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View);

	virtual uint32 GetMemoryFootprint(void) const { return(sizeof(*this) + GetAllocatedSize()); }

	uint32 GetAllocatedSize(void) const { return(FPrimitiveSceneProxy::GetAllocatedSize()); }

	void SetEmitterDynamicData_RenderThread(struct FDynamicEmitterDataBase* DynamicEmitterData, 
		class FParticleSystemSceneProxy* PSysSceneProxy, int32 EmitterIndex);

	void SetDynamicData_RenderThread(int32 EmitterCount, FFlexFluidSurfaceProperties SurfaceProperties);

	ENGINE_API void ClearDynamicEmitterData_RenderThread();

public:

	// resources managed by game thread
	int32 EmitterCount;
	struct FDynamicEmitterDataBase* DynamicEmitterDataArray[MAX_EMITTERS_PER_FLEX_FLUID_SURFACE];
	class FParticleSystemSceneProxy* ParticleSystemSceneProxyArray[MAX_EMITTERS_PER_FLEX_FLUID_SURFACE];
	UMaterialInterface* SurfaceMaterial;
	float SmoothingRadius;
	int32 MaxRadialSamples;
	float DepthEdgeFalloff;
	float ThicknessParticleScale;
	float DepthParticleScale;

	// resources managed in render thread
	TArray<FSurfaceParticleMesh> VisibleParticleMeshes;
	FMeshBatch* MeshBatch;
	FFlexFluidSurfaceTextures* Textures;

private:
	class FFlexFluidSurfaceVertexFactory* VertexFactory;
};