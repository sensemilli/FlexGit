// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	ParticleGpuSimulation.cpp: Implementation of GPU particle simulation.
==============================================================================*/

#include "EnginePrivate.h"
#include "FXSystemPrivate.h"
#include "ParticleSimulationGPU.h"
#include "ParticleSortingGPU.h"
#include "ParticleCurveTexture.h"
#include "RenderResource.h"
#include "ParticleResources.h"
#include "UniformBuffer.h"
#include "ShaderParameters.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"
#include "ParticleDefinitions.h"
#include "GlobalShader.h"
#include "../VectorField.h"
#include "../VectorFieldVisualization.h"
#include "Particles/Orientation/ParticleModuleOrientationAxisLock.h"
#include "Particles/Spawn/ParticleModuleSpawn.h"
#include "Particles/Spawn/ParticleModuleSpawnPerUnit.h"
#include "Particles/TypeData/ParticleModuleTypeDataGpu.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleModuleRequired.h"
#include "Particles/ParticleSpriteEmitter.h"
#include "Particles/ParticleSystemComponent.h"
#include "VectorField/VectorField.h"
#include "SceneUtils.h"
#include "MeshBatch.h"
#include "GlobalDistanceFieldParameters.h"
// NVCHANGE_BEGIN: JCAO - Replace vector fields with APEX turbulence velocity fields
#include "../PhysicsEngine/PhysXRender.h"
// NVCHANGE_END: JCAO - Replace vector fields with APEX turbulence velocity fields
#include "../FieldSampler/ApexFieldSamplerActor.h"

/*------------------------------------------------------------------------------
	Constants to tune memory and performance for GPU particle simulation.
------------------------------------------------------------------------------*/

/** Enable this define to permit tracking of tile allocations by GPU emitters. */
#define TRACK_TILE_ALLOCATIONS 0

/** The texture size allocated for GPU simulation. */
const int32 GParticleSimulationTextureSizeX = 1024;
const int32 GParticleSimulationTextureSizeY = 1024;

/** Texture size must be power-of-two. */
static_assert((GParticleSimulationTextureSizeX & (GParticleSimulationTextureSizeX - 1)) == 0, "Particle simulation texture size X is not a power of two.");
static_assert((GParticleSimulationTextureSizeY & (GParticleSimulationTextureSizeY - 1)) == 0, "Particle simulation texture size Y is not a power of two.");

/** The tile size. Texture space is allocated in TileSize x TileSize units. */
const int32 GParticleSimulationTileSize = 4;
const int32 GParticlesPerTile = GParticleSimulationTileSize * GParticleSimulationTileSize;

/** Tile size must be power-of-two and <= each dimension of the simulation texture. */
static_assert((GParticleSimulationTileSize & (GParticleSimulationTileSize - 1)) == 0, "Particle simulation tile size is not a power of two.");
static_assert(GParticleSimulationTileSize <= GParticleSimulationTextureSizeX, "Particle simulation tile size is larger than texture.");
static_assert(GParticleSimulationTileSize <= GParticleSimulationTextureSizeY, "Particle simulation tile size is larger than texture.");

/** How many tiles are in the simulation textures. */
const int32 GParticleSimulationTileCountX = GParticleSimulationTextureSizeX / GParticleSimulationTileSize;
const int32 GParticleSimulationTileCountY = GParticleSimulationTextureSizeY / GParticleSimulationTileSize;
const int32 GParticleSimulationTileCount = GParticleSimulationTileCountX * GParticleSimulationTileCountY;

/** GPU particle rendering code assumes that the number of particles per instanced draw is <= 16. */
static_assert(MAX_PARTICLES_PER_INSTANCE <= 16, "Max particles per instance is greater than 16.");
/** Also, it must be a power of 2. */
static_assert((MAX_PARTICLES_PER_INSTANCE & (MAX_PARTICLES_PER_INSTANCE - 1)) == 0, "Max particles per instance is not a power of two.");

/** Particle tiles are aligned to the same number as when rendering. */
enum { TILES_PER_INSTANCE = 8 };
/** The number of tiles per instance must be <= MAX_PARTICLES_PER_INSTANCE. */
static_assert(TILES_PER_INSTANCE <= MAX_PARTICLES_PER_INSTANCE, "Tiles per instance is greater than max particles per instance.");
/** Also, it must be a power of 2. */
static_assert((TILES_PER_INSTANCE & (TILES_PER_INSTANCE - 1)) == 0, "Tiles per instance is not a power of two.");

/** Maximum number of vector fields that can be evaluated at once. */
enum { MAX_VECTOR_FIELDS = 4 };

/*-----------------------------------------------------------------------------
	Allocators used to manage GPU particle resources.
-----------------------------------------------------------------------------*/

/**
 * Stack allocator for managing tile lifetime.
 */
class FParticleTileAllocator
{
public:

	/** Default constructor. */
	FParticleTileAllocator()
		: FreeTileCount(GParticleSimulationTileCount)
	{
		for ( int32 TileIndex = 0; TileIndex < GParticleSimulationTileCount; ++TileIndex )
		{
			FreeTiles[TileIndex] = GParticleSimulationTileCount - TileIndex - 1;
		}
	}

	/**
	 * Allocate a tile.
	 * @returns the index of the allocated tile, INDEX_NONE if no tiles are available.
	 */
	uint32 Allocate()
	{
		FScopeLock Lock(&CriticalSection);
		if ( FreeTileCount > 0 )
		{
			FreeTileCount--;
			return FreeTiles[FreeTileCount];
		}
		return INDEX_NONE;
	}

	/**
	 * Frees a tile so it may be allocated by another emitter.
	 * @param TileIndex - The index of the tile to free.
	 */
	void Free( uint32 TileIndex )
	{
		FScopeLock Lock(&CriticalSection);
		check( TileIndex < GParticleSimulationTileCount );
		check( FreeTileCount < GParticleSimulationTileCount );
		FreeTiles[FreeTileCount] = TileIndex;
		FreeTileCount++;
	}

	/**
	 * Returns the number of free tiles.
	 */
	int32 GetFreeTileCount() const
	{
		FScopeLock Lock(&CriticalSection);
		return FreeTileCount;
	}

private:

	/** List of free tiles. */
	uint32 FreeTiles[GParticleSimulationTileCount];
	/** How many tiles are in the free list. */
	int32 FreeTileCount;

	mutable FCriticalSection CriticalSection;
};

/*-----------------------------------------------------------------------------
	GPU resources required for simulation.
-----------------------------------------------------------------------------*/

/**
 * Per-particle information stored in a vertex buffer for drawing GPU sprites.
 */
struct FParticleIndex
{
	/** The X coordinate of the particle within the texture. */
	FFloat16 X;
	/** The Y coordinate of the particle within the texture. */
	FFloat16 Y;
};

/**
 * Texture resources holding per-particle state required for GPU simulation.
 */
class FParticleStateTextures : public FRenderResource
{
public:

	/** Contains the positions of all simulating particles. */
	FTexture2DRHIRef PositionTextureTargetRHI;
	FTexture2DRHIRef PositionTextureRHI;
	/** Contains the velocity of all simulating particles. */
	FTexture2DRHIRef VelocityTextureTargetRHI;
	FTexture2DRHIRef VelocityTextureRHI;
	// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
	/** Contains the density of all simulating particles. */
	FTexture2DRHIRef DensityTextureRHI;
	FUnorderedAccessViewRHIRef DensityTextureUAV;
	// NVCHANGE_END: JCAO - Grid Density with GPU particles

	bool bTexturesCleared;

	/**
	 * Initialize RHI resources used for particle simulation.
	 */
	virtual void InitRHI() override
	{
		const int32 SizeX = GParticleSimulationTextureSizeX;
		const int32 SizeY = GParticleSimulationTextureSizeY;

		// 32-bit per channel RGBA texture for position.
		check( !IsValidRef( PositionTextureTargetRHI ) );
		check( !IsValidRef( PositionTextureRHI ) );

		FRHIResourceCreateInfo CreateInfo(FClearValueBinding::Transparent);
		RHICreateTargetableShaderResource2D(
			SizeX,
			SizeY,
			PF_A32B32G32R32F,
			/*NumMips=*/ 1,
			TexCreate_None,
			TexCreate_RenderTargetable,
			/*bForceSeparateTargetAndShaderResource=*/ false,
			CreateInfo,
			PositionTextureTargetRHI,
			PositionTextureRHI
			);

		// 16-bit per channel RGBA texture for velocity.
		check( !IsValidRef( VelocityTextureTargetRHI ) );
		check( !IsValidRef( VelocityTextureRHI ) );

		RHICreateTargetableShaderResource2D(
			SizeX,
			SizeY,
			PF_FloatRGBA,
			/*NumMips=*/ 1,
			TexCreate_None,
			TexCreate_RenderTargetable,
			/*bForceSeparateTargetAndShaderResource=*/ false,
			CreateInfo,
			VelocityTextureTargetRHI,
			VelocityTextureRHI
			);

		bTexturesCleared = false;

		// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
		check(!IsValidRef(DensityTextureRHI));
		check(!IsValidRef(DensityTextureUAV));

		DensityTextureRHI = RHICreateTexture2D(
			SizeX, SizeY, PF_R32_FLOAT,
			/*NumMips=*/ 1,
			/*NumSamples=*/ 1,
			/*Flags=*/ TexCreate_ShaderResource | TexCreate_UAV,
			CreateInfo);

		// create UAV for compute shader
		DensityTextureUAV = RHICreateUnorderedAccessView(DensityTextureRHI);
		// NVCHANGE_END: JCAO - Grid Density with GPU particles
	}

	/**
	 * Releases RHI resources used for particle simulation.
	 */
	virtual void ReleaseRHI() override
	{
		// Release textures.
		PositionTextureTargetRHI.SafeRelease();
		PositionTextureRHI.SafeRelease();
		VelocityTextureTargetRHI.SafeRelease();
		VelocityTextureRHI.SafeRelease();

		// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
		DensityTextureUAV.SafeRelease();
		DensityTextureRHI.SafeRelease();
		// NVCHANGE_END: JCAO - Grid Density with GPU particles
	}
};

/**
 * A texture holding per-particle attributes.
 */
class FParticleAttributesTexture : public FRenderResource
{
public:

	/** Contains the attributes of all simulating particles. */
	FTexture2DRHIRef TextureTargetRHI;
	FTexture2DRHIRef TextureRHI;

	/**
	 * Initialize RHI resources used for particle simulation.
	 */
	virtual void InitRHI() override
	{
		const int32 SizeX = GParticleSimulationTextureSizeX;
		const int32 SizeY = GParticleSimulationTextureSizeY;

		FRHIResourceCreateInfo CreateInfo(FClearValueBinding::None);
		RHICreateTargetableShaderResource2D(
			SizeX,
			SizeY,
			PF_B8G8R8A8,
			/*NumMips=*/ 1,
			TexCreate_None,
			TexCreate_RenderTargetable | TexCreate_NoFastClear,
			/*bForceSeparateTargetAndShaderResource=*/ false,
			CreateInfo,
			TextureTargetRHI,
			TextureRHI
			);
	}

	/**
	 * Releases RHI resources used for particle simulation.
	 */
	virtual void ReleaseRHI() override
	{
		TextureTargetRHI.SafeRelease();
		TextureRHI.SafeRelease();
	}
};

/**
 * Vertex buffer used to hold particle indices.
 */
class FParticleIndicesVertexBuffer : public FVertexBuffer
{
public:

	/** Shader resource view of the vertex buffer. */
	FShaderResourceViewRHIRef VertexBufferSRV;

	/** Release RHI resources. */
	virtual void ReleaseRHI() override
	{
		VertexBufferSRV.SafeRelease();
		FVertexBuffer::ReleaseRHI();
	}
};

/**
 * Resources required for GPU particle simulation.
 */
class FParticleSimulationResources
{
public:

	/** Textures needed for simulation, double buffered. */
	FParticleStateTextures StateTextures[2];
	/** Texture holding render attributes. */
	FParticleAttributesTexture RenderAttributesTexture;
	/** Texture holding simulation attributes. */
	FParticleAttributesTexture SimulationAttributesTexture;
	/** Vertex buffer that points to the current sorted vertex buffer. */
	FParticleIndicesVertexBuffer SortedVertexBuffer;

	/** Frame index used to track double buffered resources on the GPU. */
	int32 FrameIndex;

	/** List of simulations to be sorted. */
	TArray<FParticleSimulationSortInfo> SimulationsToSort;
	/** The total number of sorted particles. */
	int32 SortedParticleCount;

	/** Default constructor. */
	FParticleSimulationResources()
		: FrameIndex(0)
		, SortedParticleCount(0)
	{
	}

	/**
	 * Initialize resources.
	 */
	void Init()
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FInitParticleSimulationResourcesCommand,
			FParticleSimulationResources*, ParticleResources, this,
		{
			ParticleResources->StateTextures[0].InitResource();
			ParticleResources->StateTextures[1].InitResource();
			ParticleResources->RenderAttributesTexture.InitResource();
			ParticleResources->SimulationAttributesTexture.InitResource();
			ParticleResources->SortedVertexBuffer.InitResource();
		});
	}

	/**
	 * Release resources.
	 */
	void Release()
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FReleaseParticleSimulationResourcesCommand,
			FParticleSimulationResources*, ParticleResources, this,
		{
			ParticleResources->StateTextures[0].ReleaseResource();
			ParticleResources->StateTextures[1].ReleaseResource();
			ParticleResources->RenderAttributesTexture.ReleaseResource();
			ParticleResources->SimulationAttributesTexture.ReleaseResource();
			ParticleResources->SortedVertexBuffer.ReleaseResource();
		});
	}

	/**
	 * Destroy resources.
	 */
	void Destroy()
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FDestroyParticleSimulationResourcesCommand,
			FParticleSimulationResources*, ParticleResources, this,
		{
			delete ParticleResources;
		});
	}

	/**
	 * Retrieve texture resources with up-to-date particle state.
	 */
	FParticleStateTextures& GetCurrentStateTextures()
	{
		return StateTextures[FrameIndex];
	}

	/**
	 * Retrieve texture resources with previous particle state.
	 */
	FParticleStateTextures& GetPreviousStateTextures()
	{
		return StateTextures[FrameIndex ^ 0x1];
	}

	/**
	 * Allocate a particle tile.
	 */
	uint32 AllocateTile()
	{
		return TileAllocator.Allocate();
	}

	/**
	 * Free a particle tile.
	 */
	void FreeTile( uint32 Tile )
	{
		TileAllocator.Free( Tile );
	}

	/**
	 * Returns the number of free tiles.
	 */
	int32 GetFreeTileCount() const
	{
		return TileAllocator.GetFreeTileCount();
	}

private:

	/** Allocator for managing particle tiles. */
	FParticleTileAllocator TileAllocator;
};

/** The global vertex buffers used for sorting particles on the GPU. */
TGlobalResource<FParticleSortBuffers> GParticleSortBuffers(GParticleSimulationTextureSizeX * GParticleSimulationTextureSizeY);

/*-----------------------------------------------------------------------------
	Vertex factory.
-----------------------------------------------------------------------------*/

/**
 * Uniform buffer for GPU particle sprite emitters.
 */
BEGIN_UNIFORM_BUFFER_STRUCT(FGPUSpriteEmitterUniformParameters,)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, ColorCurve)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, ColorScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, ColorBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, MiscCurve)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, MiscScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, MiscBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, SizeBySpeed)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, SubImageSize)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, TangentSelector)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, RotationRateScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, RotationBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, CameraMotionBlurAmount)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector2D, PivotOffset)
	// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, DensityColorCurve)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, DensityColorScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, DensityColorBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, DensitySizeCurve)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector2D, DensitySizeScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector2D, DensitySizeBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32, GridDensityEnabled)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32, ColorOverDensityEnabled)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32, SizeOverDensityEnabled)
	// NVCHANGE_END: JCAO - Grid Density with GPU particles
END_UNIFORM_BUFFER_STRUCT(FGPUSpriteEmitterUniformParameters)

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FGPUSpriteEmitterUniformParameters,TEXT("EmitterUniforms"));

typedef TUniformBufferRef<FGPUSpriteEmitterUniformParameters> FGPUSpriteEmitterUniformBufferRef;

/**
 * Uniform buffer to hold dynamic parameters for GPU particle sprite emitters.
 */
BEGIN_UNIFORM_BUFFER_STRUCT( FGPUSpriteEmitterDynamicUniformParameters, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( FVector2D, LocalToWorldScale )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( FVector4, AxisLockRight )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( FVector4, AxisLockUp )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( FVector4, DynamicColor)
END_UNIFORM_BUFFER_STRUCT( FGPUSpriteEmitterDynamicUniformParameters )

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FGPUSpriteEmitterDynamicUniformParameters,TEXT("EmitterDynamicUniforms"));

typedef TUniformBufferRef<FGPUSpriteEmitterDynamicUniformParameters> FGPUSpriteEmitterDynamicUniformBufferRef;

/**
 * Shader parameters for the particle vertex factory.
 */
class FGPUSpriteVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:
	virtual void Bind( const FShaderParameterMap& ParameterMap ) override
	{
		ParticleIndices.Bind(ParameterMap, TEXT("ParticleIndices"));
		ParticleIndicesOffset.Bind(ParameterMap, TEXT("ParticleIndicesOffset"));
		PositionTexture.Bind(ParameterMap, TEXT("PositionTexture"));
		PositionTextureSampler.Bind(ParameterMap, TEXT("PositionTextureSampler"));
		VelocityTexture.Bind(ParameterMap, TEXT("VelocityTexture"));
		VelocityTextureSampler.Bind(ParameterMap, TEXT("VelocityTextureSampler"));
		AttributesTexture.Bind(ParameterMap, TEXT("AttributesTexture"));
		AttributesTextureSampler.Bind(ParameterMap, TEXT("AttributesTextureSampler"));
		CurveTexture.Bind(ParameterMap, TEXT("CurveTexture"));
		CurveTextureSampler.Bind(ParameterMap, TEXT("CurveTextureSampler"));
		// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
		DensityTexture.Bind(ParameterMap, TEXT("DensityTexture"));
		DensityTextureSampler.Bind(ParameterMap, TEXT("DensityTextureSampler"));
		// NVCHANGE_END: JCAO - Grid Density with GPU particles
	}

	virtual void Serialize(FArchive& Ar) override
	{
		Ar << ParticleIndices;
		Ar << ParticleIndicesOffset;
		Ar << PositionTexture;
		Ar << PositionTextureSampler;
		Ar << VelocityTexture;
		Ar << VelocityTextureSampler;
		Ar << AttributesTexture;
		Ar << AttributesTextureSampler;
		Ar << CurveTexture;
		Ar << CurveTextureSampler;
		// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
		Ar << DensityTexture;
		Ar << DensityTextureSampler;
		// NVCHANGE_END: JCAO - Grid Density with GPU particles
	}

	virtual void SetMesh(FRHICommandList& RHICmdList, FShader* Shader,const FVertexFactory* VertexFactory,const FSceneView& View,const FMeshBatchElement& BatchElement,uint32 DataFlags) const override;

	virtual uint32 GetSize() const override { return sizeof(*this); }

private:

	/** Buffer containing particle indices. */
	FShaderResourceParameter ParticleIndices;
	/** Offset in to the particle indices buffer. */
	FShaderParameter ParticleIndicesOffset;
	/** Texture containing positions for all particles. */
	FShaderResourceParameter PositionTexture;
	FShaderResourceParameter PositionTextureSampler;
	/** Texture containing velocities for all particles. */
	FShaderResourceParameter VelocityTexture;
	FShaderResourceParameter VelocityTextureSampler;
	/** Texture containint attributes for all particles. */
	FShaderResourceParameter AttributesTexture;
	FShaderResourceParameter AttributesTextureSampler;
	/** Texture containing curves from which attributes are sampled. */
	FShaderResourceParameter CurveTexture;
	FShaderResourceParameter CurveTextureSampler;
	// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
	/** Texture containing density for all particles. */
	FShaderResourceParameter DensityTexture;
	FShaderResourceParameter DensityTextureSampler;
	// NVCHANGE_END: JCAO - Grid Density with GPU particles
};

/**
 * GPU Sprite vertex factory vertex declaration.
 */
class FGPUSpriteVertexDeclaration : public FRenderResource
{
public:

	/** The vertex declaration for GPU sprites. */
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	/**
	 * Initialize RHI resources.
	 */
	virtual void InitRHI() override
	{
		FVertexDeclarationElementList Elements;

		/** The stream to read the texture coordinates from. */
		Elements.Add(FVertexElement(0, 0, VET_Float2, 0, sizeof(FVector2D), false));

		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	/**
	 * Release RHI resources.
	 */
	virtual void ReleaseRHI() override
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

/** Global GPU sprite vertex declaration. */
TGlobalResource<FGPUSpriteVertexDeclaration> GGPUSpriteVertexDeclaration;

/**
 * Vertex factory for render sprites from GPU simulated particles.
 */
class FGPUSpriteVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FGPUSpriteVertexFactory);

public:

	/** Emitter uniform buffer. */
	FUniformBufferRHIParamRef EmitterUniformBuffer;
	/** Emitter uniform buffer for dynamic parameters. */
	FUniformBufferRHIRef EmitterDynamicUniformBuffer;
	/** Buffer containing particle indices. */
	FParticleIndicesVertexBuffer* ParticleIndicesBuffer;
	/** Offset in to the particle indices buffer. */
	uint32 ParticleIndicesOffset;
	/** Texture containing positions for all particles. */
	FTexture2DRHIParamRef PositionTextureRHI;
	/** Texture containing velocities for all particles. */
	FTexture2DRHIParamRef VelocityTextureRHI;
	/** Texture containint attributes for all particles. */
	FTexture2DRHIParamRef AttributesTextureRHI;
	// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
	/** Texture containing densities for all particles. */
	FTexture2DRHIParamRef DensityTextureRHI;
	// NVCHANGE_END JCAO - Grid Density with GPU particles

	/**
	 * Constructs render resources for this vertex factory.
	 */
	virtual void InitRHI() override
	{
		FVertexStream Stream;

		// No streams should currently exist.
		check( Streams.Num() == 0 );

		// Stream 0: Global particle texture coordinate buffer.
		Stream.VertexBuffer = &GParticleTexCoordVertexBuffer;
		Stream.Stride = sizeof(FVector2D);
		Stream.Offset = 0;
		Streams.Add( Stream );

		// Set the declaration.
		SetDeclaration( GGPUSpriteVertexDeclaration.VertexDeclarationRHI );
	}

	/**
	 * Set the source vertex buffer that contains particle indices.
	 */
	void SetVertexBuffer( FParticleIndicesVertexBuffer* VertexBuffer, uint32 Offset )
	{
		ParticleIndicesBuffer = VertexBuffer;
		ParticleIndicesOffset = Offset;
	}

	/**
	 * Should we cache the material's shadertype on this platform with this vertex factory? 
	 */
	static bool ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
	{
		return (Material->IsUsedWithParticleSprites() || Material->IsSpecialEngineMaterial()) && SupportsGPUParticles(Platform);
	}

	/**
	 * Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
	 */
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FParticleVertexFactoryBase::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("PARTICLES_PER_INSTANCE"), MAX_PARTICLES_PER_INSTANCE);

		// Set a define so we can tell in MaterialTemplate.usf when we are compiling a sprite vertex factory
		OutEnvironment.SetDefine(TEXT("PARTICLE_SPRITE_FACTORY"),TEXT("1"));
	}

	/**
	 * Construct shader parameters for this type of vertex factory.
	 */
	static FVertexFactoryShaderParameters* ConstructShaderParameters(EShaderFrequency ShaderFrequency)
	{
		return ShaderFrequency == SF_Vertex ? new FGPUSpriteVertexFactoryShaderParameters() : NULL;
	}
};

/**
 * Set vertex factory shader parameters.
 */
void FGPUSpriteVertexFactoryShaderParameters::SetMesh(FRHICommandList& RHICmdList, FShader* Shader, const FVertexFactory* VertexFactory, const FSceneView& View, const FMeshBatchElement& BatchElement, uint32 DataFlags) const
{
	FGPUSpriteVertexFactory* GPUVF = (FGPUSpriteVertexFactory*)VertexFactory;
	FVertexShaderRHIParamRef VertexShader = Shader->GetVertexShader();
	FSamplerStateRHIParamRef SamplerStatePoint = TStaticSamplerState<SF_Point>::GetRHI();
	FSamplerStateRHIParamRef SamplerStateLinear = TStaticSamplerState<SF_Bilinear>::GetRHI();
	SetUniformBufferParameter(RHICmdList, VertexShader, Shader->GetUniformBufferParameter<FGPUSpriteEmitterUniformParameters>(), GPUVF->EmitterUniformBuffer );
	SetUniformBufferParameter(RHICmdList, VertexShader, Shader->GetUniformBufferParameter<FGPUSpriteEmitterDynamicUniformParameters>(), GPUVF->EmitterDynamicUniformBuffer );
	if (ParticleIndices.IsBound())
	{
		RHICmdList.SetShaderResourceViewParameter(VertexShader, ParticleIndices.GetBaseIndex(), GPUVF->ParticleIndicesBuffer->VertexBufferSRV);
	}
	SetShaderValue(RHICmdList, VertexShader, ParticleIndicesOffset, GPUVF->ParticleIndicesOffset);
	SetTextureParameter(RHICmdList, VertexShader, PositionTexture, PositionTextureSampler, SamplerStatePoint, GPUVF->PositionTextureRHI );
	SetTextureParameter(RHICmdList, VertexShader, VelocityTexture, VelocityTextureSampler, SamplerStatePoint, GPUVF->VelocityTextureRHI );
	SetTextureParameter(RHICmdList, VertexShader, AttributesTexture, AttributesTextureSampler, SamplerStatePoint, GPUVF->AttributesTextureRHI );
	SetTextureParameter(RHICmdList, VertexShader, CurveTexture, CurveTextureSampler, SamplerStateLinear, GParticleCurveTexture.GetCurveTexture() );
	// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
	SetTextureParameter(RHICmdList, VertexShader, DensityTexture, DensityTextureSampler, SamplerStatePoint, GPUVF->DensityTextureRHI);
	// NVCHANGE_END: JCAO - Grid Density with GPU particles
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FGPUSpriteVertexFactory,"ParticleGPUSpriteVertexFactory",true,false,true,false,false);

/*-----------------------------------------------------------------------------
	Shaders used for simulation.
-----------------------------------------------------------------------------*/

/**
 * Uniform buffer to hold parameters for particle simulation.
 */
BEGIN_UNIFORM_BUFFER_STRUCT(FParticleSimulationParameters,)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, AttributeCurve)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, AttributeCurveScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, AttributeCurveBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, AttributeScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, AttributeBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, MiscCurve)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, MiscScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, MiscBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, Acceleration)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, OrbitOffsetBase)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, OrbitOffsetRange)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, OrbitFrequencyBase)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, OrbitFrequencyRange)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, OrbitPhaseBase)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, OrbitPhaseRange)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, CollisionRadiusScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, CollisionRadiusBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, CollisionTimeBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, OneMinusFriction)
END_UNIFORM_BUFFER_STRUCT(FParticleSimulationParameters)

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FParticleSimulationParameters,TEXT("Simulation"));

typedef TUniformBufferRef<FParticleSimulationParameters> FParticleSimulationBufferRef;

/**
 * Per-frame parameters for particle simulation.
 */
struct FParticlePerFrameSimulationParameters
{
	/** Position (XYZ) and squared radius (W) of the point attractor. */
	FVector4 PointAttractor;
	/** Position offset (XYZ) to add to particles and strength of the attractor (W). */
	FVector4 PositionOffsetAndAttractorStrength;
	/** Amount by which to scale bounds for collision purposes. */
	FVector2D LocalToWorldScale;
	/** Amount of time by which to simulate particles. */
	float DeltaSeconds;
	// NVCHANGE_BEGIN: JCAO - Support Force Type Noise
	float TotalSeconds;
	// NVCHANGE_END: JCAO - Support Force Type Noise

	FParticlePerFrameSimulationParameters()
		: PointAttractor(FVector::ZeroVector,0.0f)
		, PositionOffsetAndAttractorStrength(FVector::ZeroVector,0.0f)
		, LocalToWorldScale(1.0f, 1.0f)
		, DeltaSeconds(0.0f)
		// NVCHANGE_BEGIN: JCAO - Support Force Type Noise
		, TotalSeconds(0.0f)
		// NVCHANGE_END: JCAO - Support Force Type Noise
	{
	}
};

/**
 * Per-frame shader parameters for particle simulation.
 */
struct FParticlePerFrameSimulationShaderParameters
{
	FShaderParameter PointAttractor;
	FShaderParameter PositionOffsetAndAttractorStrength;
	FShaderParameter LocalToWorldScale;
	FShaderParameter DeltaSeconds;
	// NVCHANGE_BEGIN: JCAO - Support Force Type Noise
	FShaderParameter TotalSeconds;
	// NVCHANGE_END: JCAO - Support Force Type Noise

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		PointAttractor.Bind(ParameterMap,TEXT("PointAttractor"));
		PositionOffsetAndAttractorStrength.Bind(ParameterMap,TEXT("PositionOffsetAndAttractorStrength"));
		LocalToWorldScale.Bind(ParameterMap,TEXT("LocalToWorldScale"));
		DeltaSeconds.Bind(ParameterMap,TEXT("DeltaSeconds"));
		// NVCHANGE_BEGIN: JCAO - Support Force Type Noise
		TotalSeconds.Bind(ParameterMap, TEXT("TotalSeconds"));
		// NVCHANGE_END: JCAO - Support Force Type Noise
	}

	template <typename ShaderRHIParamRef>
	void Set(FRHICommandList& RHICmdList, const ShaderRHIParamRef& ShaderRHI, const FParticlePerFrameSimulationParameters& Parameters) const
	{
		SetShaderValue(RHICmdList,ShaderRHI,PointAttractor,Parameters.PointAttractor);
		SetShaderValue(RHICmdList,ShaderRHI,PositionOffsetAndAttractorStrength,Parameters.PositionOffsetAndAttractorStrength);
		SetShaderValue(RHICmdList,ShaderRHI,LocalToWorldScale,Parameters.LocalToWorldScale);
		SetShaderValue(RHICmdList,ShaderRHI,DeltaSeconds,Parameters.DeltaSeconds);
		// NVCHANGE_BEGIN: JCAO - Support Force Type Noise
		SetShaderValue(RHICmdList, ShaderRHI, TotalSeconds, Parameters.TotalSeconds);
		// NVCHANGE_END: JCAO - Support Force Type Noise
	}
};

FArchive& operator<<(FArchive& Ar, FParticlePerFrameSimulationShaderParameters& PerFrameParameters)
{
	Ar << PerFrameParameters.PointAttractor;
	Ar << PerFrameParameters.PositionOffsetAndAttractorStrength;
	Ar << PerFrameParameters.LocalToWorldScale;
	Ar << PerFrameParameters.DeltaSeconds;
	// NVCHANGE_BEGIN: JCAO - Support Force Type Noise
	Ar << PerFrameParameters.TotalSeconds;
	// NVCHANGE_END: JCAO - Support Force Type Noise
	return Ar;
}

/**
 * Uniform buffer to hold parameters for vector fields sampled during particle
 * simulation.
 */
BEGIN_UNIFORM_BUFFER_STRUCT( FVectorFieldUniformParameters,)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY( FMatrix, WorldToVolume, [MAX_VECTOR_FIELDS] )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY( FMatrix, VolumeToWorld, [MAX_VECTOR_FIELDS] )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY( FVector, VolumeSize, [MAX_VECTOR_FIELDS] )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY( FVector2D, IntensityAndTightness, [MAX_VECTOR_FIELDS] )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( int32, Count )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY( FVector, TilingAxes, [MAX_VECTOR_FIELDS] )
	// NVCHANGE_BEGIN: JCAO - Replace vector fields with APEX turbulence velocity fields
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int32, VectorFieldCount)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int32, VelocityFieldCount)
	// NVCHANGE_END: JCAO - Replace vector fields with APEX turbulence velocity fields
END_UNIFORM_BUFFER_STRUCT( FVectorFieldUniformParameters )

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FVectorFieldUniformParameters,TEXT("VectorFields"));

typedef TUniformBufferRef<FVectorFieldUniformParameters> FVectorFieldUniformBufferRef;

// NVCHANGE_BEGIN: JCAO - Add Attractor working with GPU particles
#if WITH_APEX_TURBULENCE
/**
 * Uniform buffer to hold parameters for attractor field sampler. 
 */
BEGIN_UNIFORM_BUFFER_STRUCT(FAttractorFSUniformParameters, )
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, Origin)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, RadiusAndStrength)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int32, Count)
END_UNIFORM_BUFFER_STRUCT(FAttractorFSUniformParameters)

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FAttractorFSUniformParameters, TEXT("AttractorFieldSamplers"));

typedef TUniformBufferRef<FAttractorFSUniformParameters> FAttractorFSUniformBufferRef;

// NVCHANGE_BEGIN: JCAO - Support Force Type Noise
BEGIN_UNIFORM_BUFFER_STRUCT(FNoiseFSUniformParameters, )
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, NoiseSpaceFreq)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, NoiseSpaceFreqOctaveMultiplier)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, NoiseStrengthAndTimeFreq) // NoiseStrength NoiseTimeFreq*TotalTime NoiseStrengthOctaveMultiplier NoiseTimeFreqOctaveMultiplier
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int32, NoiseOctaves)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int32, NoiseType)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int32, NoiseSeed)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int32, Count)
END_UNIFORM_BUFFER_STRUCT(FNoiseFSUniformParameters)

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FNoiseFSUniformParameters, TEXT("NoiseFieldSamplers"));

typedef TUniformBufferRef<FNoiseFSUniformParameters> FNoiseFSUniformBufferRef;
// NVCHANGE_END: JCAO - Support Force Type Noise
#endif // WITH_APEX_TURBULENCE
// NVCHANGE_END: JCAO - Add Attractor working with GPU particles
/**
 * Vertex shader for drawing particle tiles on the GPU.
 */
class FParticleTileVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleTileVS,Global);

public:

	static bool ShouldCache( EShaderPlatform Platform )
	{
		return SupportsGPUParticles(Platform);
	}

	static void ModifyCompilationEnvironment( EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment )
	{
		FGlobalShader::ModifyCompilationEnvironment( Platform, OutEnvironment );
		OutEnvironment.SetDefine(TEXT("TILES_PER_INSTANCE"), TILES_PER_INSTANCE);
		OutEnvironment.SetFloatDefine(
			TEXT("TILE_SIZE_X"),
			(float)GParticleSimulationTileSize / (float)GParticleSimulationTextureSizeX
			);
		OutEnvironment.SetFloatDefine(
			TEXT("TILE_SIZE_Y"),
			(float)GParticleSimulationTileSize / (float)GParticleSimulationTextureSizeY
									  );
	}

	/** Default constructor. */
	FParticleTileVS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleTileVS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
		TileOffsets.Bind(Initializer.ParameterMap, TEXT("TileOffsets"));
	}

	/** Serialization. */
	virtual bool Serialize( FArchive& Ar ) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize( Ar );
		Ar << TileOffsets;
		return bShaderHasOutdatedParameters;
	}

	/** Set parameters. */
	void SetParameters(FRHICommandList& RHICmdList, FParticleShaderParamRef TileOffsetsRef)
	{
		FVertexShaderRHIParamRef VertexShaderRHI = GetVertexShader();
		if (TileOffsets.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(VertexShaderRHI, TileOffsets.GetBaseIndex(), TileOffsetsRef);
		}
	}

private:

	/** Buffer from which to read tile offsets. */
	FShaderResourceParameter TileOffsets;
};

enum EParticleCollisionShaderMode
{
	PCM_None,
	PCM_DepthBuffer,
	PCM_DistanceField
};

/** Helper function to determine whether the given particle collision shader mode is supported on the given shader platform */
inline bool IsParticleCollisionModeSupported(EShaderPlatform InPlatform, EParticleCollisionShaderMode InCollisionShaderMode)
{
	return IsFeatureLevelSupported(InPlatform, ERHIFeatureLevel::SM5) || InCollisionShaderMode != PCM_DistanceField;
}

/**
 * Pixel shader for simulating particles on the GPU.
 */
template <EParticleCollisionShaderMode CollisionMode>
class TParticleSimulationPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TParticleSimulationPS,Global);

public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return SupportsGPUParticles(Platform) && IsParticleCollisionModeSupported(Platform, CollisionMode);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("PARTICLE_SIMULATION_PIXELSHADER"), 1);
		OutEnvironment.SetDefine(TEXT("MAX_VECTOR_FIELDS"), MAX_VECTOR_FIELDS);
		OutEnvironment.SetDefine(TEXT("DEPTH_BUFFER_COLLISION"), (uint32)(CollisionMode == PCM_DepthBuffer ? 1 : 0));
		OutEnvironment.SetDefine(TEXT("DISTANCE_FIELD_COLLISION"), (uint32)(CollisionMode == PCM_DistanceField ? 1 : 0));
		OutEnvironment.SetRenderTargetOutputFormat(0, PF_A32B32G32R32F);
		// NVCHANGE_BEGIN: JCAO - Replace vector fields with APEX turbulence velocity fields
#if WITH_APEX_TURBULENCE
		OutEnvironment.SetDefine(TEXT("WITH_APEX_TURBULENCE"), 1);
#endif
		// NVCHANGE_END: JCAO - Replace vector fields with APEX turbulence velocity fields
	}

	/** Default constructor. */
	TParticleSimulationPS()
	{
	}

	/** Initialization constructor. */
	explicit TParticleSimulationPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PositionTexture.Bind(Initializer.ParameterMap, TEXT("PositionTexture"));
		PositionTextureSampler.Bind(Initializer.ParameterMap, TEXT("PositionTextureSampler"));
		VelocityTexture.Bind(Initializer.ParameterMap, TEXT("VelocityTexture"));
		VelocityTextureSampler.Bind(Initializer.ParameterMap, TEXT("VelocityTextureSampler"));
		AttributesTexture.Bind(Initializer.ParameterMap, TEXT("AttributesTexture"));
		AttributesTextureSampler.Bind(Initializer.ParameterMap, TEXT("AttributesTextureSampler"));
		RenderAttributesTexture.Bind(Initializer.ParameterMap, TEXT("RenderAttributesTexture"));
		RenderAttributesTextureSampler.Bind(Initializer.ParameterMap, TEXT("RenderAttributesTextureSampler"));
		CurveTexture.Bind(Initializer.ParameterMap, TEXT("CurveTexture"));
		CurveTextureSampler.Bind(Initializer.ParameterMap, TEXT("CurveTextureSampler"));
		VectorFieldTextures0.Bind(Initializer.ParameterMap, TEXT("VectorFieldTextures0"));
		VectorFieldTextures1.Bind(Initializer.ParameterMap, TEXT("VectorFieldTextures1"));
		VectorFieldTextures2.Bind(Initializer.ParameterMap, TEXT("VectorFieldTextures2"));
		VectorFieldTextures3.Bind(Initializer.ParameterMap, TEXT("VectorFieldTextures3"));
		VectorFieldTexturesSampler0.Bind(Initializer.ParameterMap, TEXT("VectorFieldTexturesSampler0"));
		VectorFieldTexturesSampler1.Bind(Initializer.ParameterMap, TEXT("VectorFieldTexturesSampler1"));
		VectorFieldTexturesSampler2.Bind(Initializer.ParameterMap, TEXT("VectorFieldTexturesSampler2"));
		VectorFieldTexturesSampler3.Bind(Initializer.ParameterMap, TEXT("VectorFieldTexturesSampler3"));
		SceneDepthTextureParameter.Bind(Initializer.ParameterMap,TEXT("SceneDepthTexture"));
		SceneDepthTextureParameterSampler.Bind(Initializer.ParameterMap,TEXT("SceneDepthTextureSampler"));
		GBufferATextureParameter.Bind(Initializer.ParameterMap,TEXT("GBufferATexture"));
		GBufferATextureParameterSampler.Bind(Initializer.ParameterMap,TEXT("GBufferATextureSampler"));
		CollisionDepthBounds.Bind(Initializer.ParameterMap,TEXT("CollisionDepthBounds"));
		PerFrameParameters.Bind(Initializer.ParameterMap);
		GlobalDistanceFieldParameters.Bind(Initializer.ParameterMap);
	}

	/** Serialization. */
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize( Ar );
		Ar << PositionTexture;
		Ar << PositionTextureSampler;
		Ar << VelocityTexture;
		Ar << VelocityTextureSampler;
		Ar << AttributesTexture;
		Ar << AttributesTextureSampler;
		Ar << RenderAttributesTexture;
		Ar << RenderAttributesTextureSampler;
		Ar << CurveTexture;
		Ar << CurveTextureSampler;
		Ar << VectorFieldTextures0;
		Ar << VectorFieldTextures1;
		Ar << VectorFieldTextures2;
		Ar << VectorFieldTextures3;
		Ar << VectorFieldTexturesSampler0;
		Ar << VectorFieldTexturesSampler1;
		Ar << VectorFieldTexturesSampler2;
		Ar << VectorFieldTexturesSampler3;
		Ar << SceneDepthTextureParameter;
		Ar << SceneDepthTextureParameterSampler;
		Ar << GBufferATextureParameter;
		Ar << GBufferATextureParameterSampler;
		Ar << CollisionDepthBounds;
		Ar << PerFrameParameters;
		Ar << GlobalDistanceFieldParameters;
		return bShaderHasOutdatedParameters;
	}

	/**
	 * Set parameters for this shader.
	 */
	void SetParameters(
		FRHICommandList& RHICmdList, 
		const FParticleStateTextures& TextureResources,
		const FParticleAttributesTexture& InAttributesTexture,
		const FParticleAttributesTexture& InRenderAttributesTexture,
		const FSceneView* CollisionView,
		const FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData,
		FTexture2DRHIParamRef SceneDepthTexture,
		FTexture2DRHIParamRef GBufferATexture
		)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		FSamplerStateRHIParamRef SamplerStatePoint = TStaticSamplerState<SF_Point>::GetRHI();
		FSamplerStateRHIParamRef SamplerStateLinear = TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
		SetTextureParameter(RHICmdList, PixelShaderRHI, PositionTexture, PositionTextureSampler, SamplerStatePoint, TextureResources.PositionTextureRHI);
		SetTextureParameter(RHICmdList, PixelShaderRHI, VelocityTexture, VelocityTextureSampler, SamplerStatePoint, TextureResources.VelocityTextureRHI);
		SetTextureParameter(RHICmdList, PixelShaderRHI, AttributesTexture, AttributesTextureSampler, SamplerStatePoint, InAttributesTexture.TextureRHI);
		SetTextureParameter(RHICmdList, PixelShaderRHI, CurveTexture, CurveTextureSampler, SamplerStateLinear, GParticleCurveTexture.GetCurveTexture());

		if (CollisionMode == PCM_DepthBuffer)
		{
			check(CollisionView != NULL);
			FGlobalShader::SetParameters(RHICmdList, PixelShaderRHI,*CollisionView);
			SetTextureParameter(
				RHICmdList, 
				PixelShaderRHI,
				SceneDepthTextureParameter,
				SceneDepthTextureParameterSampler,
				TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
				SceneDepthTexture
				);
			SetTextureParameter(
				RHICmdList, 
				PixelShaderRHI,
				GBufferATextureParameter,
				GBufferATextureParameterSampler,
				TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
				GBufferATexture
				);
			SetTextureParameter(
				RHICmdList, 
				PixelShaderRHI,
				RenderAttributesTexture,
				RenderAttributesTextureSampler,
				SamplerStatePoint,
				InRenderAttributesTexture.TextureRHI
				);
			SetShaderValue(RHICmdList, PixelShaderRHI, CollisionDepthBounds, FXConsoleVariables::GPUCollisionDepthBounds);
		}
		else if (CollisionMode == PCM_DistanceField)
		{
			GlobalDistanceFieldParameters.Set(RHICmdList, PixelShaderRHI, *GlobalDistanceFieldParameterData);

			SetTextureParameter(
				RHICmdList, 
				PixelShaderRHI,
				RenderAttributesTexture,
				RenderAttributesTextureSampler,
				SamplerStatePoint,
				InRenderAttributesTexture.TextureRHI
				);
		}
	}

	/**
	 * Set parameters for the vector fields sampled by this shader.
	 * @param VectorFieldParameters -Parameters needed to sample local vector fields.
	 */
	void SetVectorFieldParameters(FRHICommandList& RHICmdList, const FVectorFieldUniformBufferRef& UniformBuffer, const FTexture3DRHIParamRef VolumeTexturesRHI[])
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		FSamplerStateRHIParamRef SamplerStateLinear = TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
		SetUniformBufferParameter(RHICmdList, PixelShaderRHI, GetUniformBufferParameter<FVectorFieldUniformParameters>(), UniformBuffer);
		SetTextureParameter(RHICmdList, PixelShaderRHI, VectorFieldTextures0, VectorFieldTexturesSampler0, SamplerStateLinear, VolumeTexturesRHI[0], 0);
		SetTextureParameter(RHICmdList, PixelShaderRHI, VectorFieldTextures1, VectorFieldTexturesSampler1, SamplerStateLinear, VolumeTexturesRHI[1], 0);
		SetTextureParameter(RHICmdList, PixelShaderRHI, VectorFieldTextures2, VectorFieldTexturesSampler2, SamplerStateLinear, VolumeTexturesRHI[2], 0);
		SetTextureParameter(RHICmdList, PixelShaderRHI, VectorFieldTextures3, VectorFieldTexturesSampler3, SamplerStateLinear, VolumeTexturesRHI[3], 0);
	}

	// NVCHANGE_BEGIN: JCAO - Add Attractor working with GPU particles
#if WITH_APEX_TURBULENCE
	void SetAttractorFSParameters(FRHICommandList& RHICmdList, const FAttractorFSUniformBufferRef& UniformBuffer)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		SetUniformBufferParameter(RHICmdList, PixelShaderRHI, GetUniformBufferParameter<FAttractorFSUniformParameters>(), UniformBuffer);
	}
	// NVCHANGE_BEGIN: JCAO - Support Force Type Noise
	void SetNoiseFSParameters(FRHICommandList& RHICmdList, const FNoiseFSUniformBufferRef& UniformBuffer)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		SetUniformBufferParameter(RHICmdList, PixelShaderRHI, GetUniformBufferParameter<FNoiseFSUniformParameters>(), UniformBuffer);
	}
	// NVCHANGE_END: JCAO - Support Force Type Noise
#endif // WITH_APEX_TURBULENCE
	// NVCHANGE_END: JCAO - Add Attractor working with GPU particles

	/**
	 * Set per-instance parameters for this shader.
	 */
	void SetInstanceParameters(FRHICommandList& RHICmdList, FUniformBufferRHIParamRef UniformBuffer, const FParticlePerFrameSimulationParameters& InPerFrameParameters)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		SetUniformBufferParameter(RHICmdList, PixelShaderRHI, GetUniformBufferParameter<FParticleSimulationParameters>(), UniformBuffer);
		PerFrameParameters.Set(RHICmdList, PixelShaderRHI, InPerFrameParameters);
	}

	/**
	 * Unbinds buffers that may need to be bound as UAVs.
	 */
	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		FShaderResourceViewRHIParamRef NullSRV = FShaderResourceViewRHIParamRef();
		if (VectorFieldTextures0.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI, VectorFieldTextures0.GetBaseIndex(), NullSRV);
		}
		if (VectorFieldTextures1.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI, VectorFieldTextures1.GetBaseIndex(), NullSRV);
		}
		if (VectorFieldTextures2.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI, VectorFieldTextures2.GetBaseIndex(), NullSRV);
		}
		if (VectorFieldTextures3.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI, VectorFieldTextures3.GetBaseIndex(), NullSRV);
		}
	}

private:

	/** The position texture parameter. */
	FShaderResourceParameter PositionTexture;
	FShaderResourceParameter PositionTextureSampler;
	/** The velocity texture parameter. */
	FShaderResourceParameter VelocityTexture;
	FShaderResourceParameter VelocityTextureSampler;
	/** The simulation attributes texture parameter. */
	FShaderResourceParameter AttributesTexture;
	FShaderResourceParameter AttributesTextureSampler;
	/** The render attributes texture parameter. */
	FShaderResourceParameter RenderAttributesTexture;
	FShaderResourceParameter RenderAttributesTextureSampler;
	/** The curve texture parameter. */
	FShaderResourceParameter CurveTexture;
	FShaderResourceParameter CurveTextureSampler;
	/** Vector fields. */
	FShaderResourceParameter VectorFieldTextures0;
	FShaderResourceParameter VectorFieldTextures1;
	FShaderResourceParameter VectorFieldTextures2;
	FShaderResourceParameter VectorFieldTextures3;
	FShaderResourceParameter VectorFieldTexturesSampler0;
	FShaderResourceParameter VectorFieldTexturesSampler1;
	FShaderResourceParameter VectorFieldTexturesSampler2;
	FShaderResourceParameter VectorFieldTexturesSampler3;
	/** The SceneDepthTexture parameter for depth buffer collision. */
	FShaderResourceParameter SceneDepthTextureParameter;
	FShaderResourceParameter SceneDepthTextureParameterSampler;
	/** The GBufferATexture parameter for depth buffer collision. */
	FShaderResourceParameter GBufferATextureParameter;
	FShaderResourceParameter GBufferATextureParameterSampler;
	/** Per frame simulation parameters. */
	FParticlePerFrameSimulationShaderParameters PerFrameParameters;
	/** Collision depth bounds. */
	FShaderParameter CollisionDepthBounds;
	FGlobalDistanceFieldParameters GlobalDistanceFieldParameters;
};

/**
 * Pixel shader for clearing particle simulation data on the GPU.
 */
class FParticleSimulationClearPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleSimulationClearPS,Global);

public:

	static bool ShouldCache( EShaderPlatform Platform )
	{
		return SupportsGPUParticles(Platform);
	}

	static void ModifyCompilationEnvironment( EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment )
	{
		FGlobalShader::ModifyCompilationEnvironment( Platform, OutEnvironment );
		OutEnvironment.SetDefine( TEXT("PARTICLE_CLEAR_PIXELSHADER"), 1 );
		OutEnvironment.SetRenderTargetOutputFormat(0, PF_A32B32G32R32F);
	}

	/** Default constructor. */
	FParticleSimulationClearPS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleSimulationClearPS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
	}

	/** Serialization. */
	virtual bool Serialize( FArchive& Ar ) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize( Ar );
		return bShaderHasOutdatedParameters;
	}
};

/** Implementation for all shaders used for simulation. */
IMPLEMENT_SHADER_TYPE(,FParticleTileVS,TEXT("ParticleSimulationShader"),TEXT("VertexMain"),SF_Vertex);
IMPLEMENT_SHADER_TYPE(template<>,TParticleSimulationPS<PCM_None>,TEXT("ParticleSimulationShader"),TEXT("PixelMain"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>,TParticleSimulationPS<PCM_DepthBuffer>,TEXT("ParticleSimulationShader"),TEXT("PixelMain"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>,TParticleSimulationPS<PCM_DistanceField>,TEXT("ParticleSimulationShader"),TEXT("PixelMain"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(,FParticleSimulationClearPS,TEXT("ParticleSimulationShader"),TEXT("PixelMain"),SF_Pixel);

/**
 * Vertex declaration for drawing particle tiles.
 */
class FParticleTileVertexDeclaration : public FRenderResource
{
public:

	/** The vertex declaration. */
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	virtual void InitRHI() override
	{
		FVertexDeclarationElementList Elements;
		// TexCoord.
		Elements.Add(FVertexElement(0, 0, VET_Float2, 0, sizeof(FVector2D), /*bUseInstanceIndex=*/ false));
		VertexDeclarationRHI = RHICreateVertexDeclaration( Elements );
	}

	virtual void ReleaseRHI() override
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

/** Global vertex declaration resource for particle sim visualization. */
TGlobalResource<FParticleTileVertexDeclaration> GParticleTileVertexDeclaration;

/**
 * Computes the aligned tile count.
 */
FORCEINLINE int32 ComputeAlignedTileCount(int32 TileCount)
{
	return (TileCount + (TILES_PER_INSTANCE-1)) & (~(TILES_PER_INSTANCE-1));
}

/**
 * Builds a vertex buffer containing the offsets for a set of tiles.
 * @param TileOffsetsRef - The vertex buffer to fill. Must be at least TileCount * sizeof(FVector4) in size.
 * @param Tiles - The tiles which will be drawn.
 * @param TileCount - The number of tiles in the array.
 */
static void BuildTileVertexBuffer( FParticleBufferParamRef TileOffsetsRef, const uint32* Tiles, int32 TileCount )
{
	const int32 AlignedTileCount = ComputeAlignedTileCount(TileCount);
	FVector2D* TileOffset = (FVector2D*)RHILockVertexBuffer( TileOffsetsRef, 0, AlignedTileCount * sizeof(FVector2D), RLM_WriteOnly );
	for ( int32 Index = 0; Index < TileCount; ++Index )
	{
		const uint32 TileIndex = Tiles[Index];
		TileOffset[Index].X = FMath::Fractional( (float)TileIndex / (float)GParticleSimulationTileCountX );
		TileOffset[Index].Y = FMath::Fractional( FMath::TruncToFloat( (float)TileIndex / (float)GParticleSimulationTileCountX ) / (float)GParticleSimulationTileCountY );
	}
	for ( int32 Index = TileCount; Index < AlignedTileCount; ++Index )
	{
		TileOffset[Index].X = 100.0f;
		TileOffset[Index].Y = 100.0f;
	}
		RHIUnlockVertexBuffer( TileOffsetsRef );
	}

/**
 * Builds a vertex buffer containing the offsets for a set of tiles.
 * @param TileOffsetsRef - The vertex buffer to fill. Must be at least TileCount * sizeof(FVector4) in size.
 * @param Tiles - The tiles which will be drawn.
 */
static void BuildTileVertexBuffer( FParticleBufferParamRef TileOffsetsRef, const TArray<uint32>& Tiles )
{
	BuildTileVertexBuffer( TileOffsetsRef, Tiles.GetData(), Tiles.Num() );
}

/**
 * Issues a draw call for an aligned set of tiles.
 * @param TileCount - The number of tiles to be drawn.
 */
static void DrawAlignedParticleTiles(FRHICommandList& RHICmdList, int32 TileCount)
{
	check((TileCount & (TILES_PER_INSTANCE-1)) == 0);

	// Stream 0: TexCoord.
	RHICmdList.SetStreamSource(
		0,
		GParticleTexCoordVertexBuffer.VertexBufferRHI,
		/*Stride=*/ sizeof(FVector2D),
		/*Offset=*/ 0
		);

	// Draw tiles.
	RHICmdList.DrawIndexedPrimitive(
		GParticleIndexBuffer.IndexBufferRHI,
		PT_TriangleList,
		/*BaseVertexIndex=*/ 0,
		/*MinIndex=*/ 0,
		/*NumVertices=*/ 4,
		/*StartIndex=*/ 0,
		/*NumPrimitives=*/ 2 * TILES_PER_INSTANCE,
		/*NumInstances=*/ TileCount / TILES_PER_INSTANCE
		);
}

/**
 * The data needed to simulate a set of particle tiles on the GPU.
 */
struct FSimulationCommandGPU
{
	/** Buffer containing the offsets of each tile. */
	FParticleShaderParamRef TileOffsetsRef;
	/** Uniform buffer containing simulation parameters. */
	FUniformBufferRHIParamRef UniformBuffer;
	/** Uniform buffer containing per-frame simulation parameters. */
	FParticlePerFrameSimulationParameters PerFrameParameters;
	/** Parameters to sample the local vector field for this simulation. */
	FVectorFieldUniformBufferRef VectorFieldsUniformBuffer;
	// NVCHANGE_BEGIN: JCAO - Add Attractor working with GPU particles
#if WITH_APEX_TURBULENCE
	FAttractorFSUniformBufferRef AttractorFSUniformBuffer;
	FNoiseFSUniformBufferRef NoiseFSUniformBuffer;
#endif // WITH_APEX_TURBULENCE
	// NVCHANGE_END: JCAO - Add Attractor working with GPU particles
	/** Vector field volume textures for this simulation. */
	FTexture3DRHIParamRef VectorFieldTexturesRHI[MAX_VECTOR_FIELDS];
	/** The number of tiles to simulate. */
	int32 TileCount;

	/** Initialization constructor. */
#if WITH_APEX_TURBULENCE
	FSimulationCommandGPU(FParticleShaderParamRef InTileOffsetsRef, FUniformBufferRHIParamRef InUniformBuffer, const FParticlePerFrameSimulationParameters& InPerFrameParameters, FVectorFieldUniformBufferRef& InVectorFieldsUniformBuffer, FAttractorFSUniformBufferRef& InAttractorFSUniformBuffer, FNoiseFSUniformBufferRef& InNoiseFSUniformBuffer, int32 InTileCount)
#else
	FSimulationCommandGPU(FParticleShaderParamRef InTileOffsetsRef, FUniformBufferRHIParamRef InUniformBuffer, const FParticlePerFrameSimulationParameters& InPerFrameParameters, FVectorFieldUniformBufferRef& InVectorFieldsUniformBuffer, int32 InTileCount)
#endif
		: TileOffsetsRef(InTileOffsetsRef)
		, UniformBuffer(InUniformBuffer)
		, PerFrameParameters(InPerFrameParameters)
		, VectorFieldsUniformBuffer(InVectorFieldsUniformBuffer)
		, TileCount(InTileCount)
#if WITH_APEX_TURBULENCE
		, AttractorFSUniformBuffer(InAttractorFSUniformBuffer)
		, NoiseFSUniformBuffer(InNoiseFSUniformBuffer)
#endif
	{
		FTexture3DRHIParamRef BlackVolumeTextureRHI = (FTexture3DRHIParamRef)(FTextureRHIParamRef)GBlackVolumeTexture->TextureRHI;
		for (int32 i = 0; i < MAX_VECTOR_FIELDS; ++i)
		{
			VectorFieldTexturesRHI[i] = BlackVolumeTextureRHI;
		}
	}
};

/**
 * Executes each command invoking the simulation pixel shader for each particle.
 * calling with empty SimulationCommands is a waste of performance
 * @param SimulationCommands The list of simulation commands to execute.
 * @param TextureResources	The resources from which the current state can be read.
 * @param AttributeTexture	The texture from which particle simulation attributes can be read.
 * @param CollisionView		The view to use for collision, if any.
 * @param SceneDepthTexture The depth texture to use for collision, if any.
 */
template <EParticleCollisionShaderMode CollisionMode>
void ExecuteSimulationCommands(
	FRHICommandList& RHICmdList,
	ERHIFeatureLevel::Type FeatureLevel,
	const TArray<FSimulationCommandGPU>& SimulationCommands,
	const FParticleStateTextures& TextureResources,
	const FParticleAttributesTexture& AttributeTexture,
	const FParticleAttributesTexture& RenderAttributeTexture,
	const FSceneView* CollisionView,
	const FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData,
	FTexture2DRHIParamRef SceneDepthTexture,
	FTexture2DRHIParamRef GBufferATexture
	)
{
	// Grab shaders.
	TShaderMapRef<FParticleTileVS> VertexShader(GetGlobalShaderMap(FeatureLevel));
	TShaderMapRef<TParticleSimulationPS<CollisionMode> > PixelShader(GetGlobalShaderMap(FeatureLevel));

	// Bound shader state.
	
	static FGlobalBoundShaderState BoundShaderState;
	SetGlobalBoundShaderState(
		RHICmdList,
		FeatureLevel,
		BoundShaderState,
		GParticleTileVertexDeclaration.VertexDeclarationRHI,
		*VertexShader,
		*PixelShader,
		0
		);

	PixelShader->SetParameters(RHICmdList, TextureResources, AttributeTexture, RenderAttributeTexture, CollisionView, GlobalDistanceFieldParameterData, SceneDepthTexture, GBufferATexture);

	// Draw tiles to perform the simulation step.
	const int32 CommandCount = SimulationCommands.Num();
	for (int32 CommandIndex = 0; CommandIndex < CommandCount; ++CommandIndex)
	{
		const FSimulationCommandGPU& Command = SimulationCommands[CommandIndex];
		VertexShader->SetParameters(RHICmdList, Command.TileOffsetsRef);
		PixelShader->SetInstanceParameters(RHICmdList, Command.UniformBuffer, Command.PerFrameParameters);
		PixelShader->SetVectorFieldParameters(
			RHICmdList, 
			Command.VectorFieldsUniformBuffer,
			Command.VectorFieldTexturesRHI
			);
		// NVCHANGE_BEGIN: JCAO - Add Attractor working with GPU particles
#if WITH_APEX_TURBULENCE
		PixelShader->SetAttractorFSParameters(RHICmdList, Command.AttractorFSUniformBuffer);
		// NVCHANGE_BEGIN: JCAO - Support Force Type Noise
		PixelShader->SetNoiseFSParameters(RHICmdList, Command.NoiseFSUniformBuffer);
		// NVCHANGE_END: JCAO - Support Force Type Noise
#endif // WITH_APEX_TURBULENCE
		// NVCHANGE_END: JCAO - Add Attractor working with GPU particles
		DrawAlignedParticleTiles(RHICmdList, Command.TileCount);
	}

	// Unbind input buffers.
	PixelShader->UnbindBuffers(RHICmdList);
}

/**
 * Invokes the clear simulation shader for each particle in each tile.
 * @param Tiles - The list of tiles to clear.
 */
void ClearTiles(FRHICommandList& RHICmdList, ERHIFeatureLevel::Type FeatureLevel, const TArray<uint32>& Tiles)
{
	SCOPED_DRAW_EVENT(RHICmdList, ClearTiles);

	const int32 MaxTilesPerDrawCallUnaligned = GParticleScratchVertexBufferSize / sizeof(FVector2D);
	const int32 MaxTilesPerDrawCall = MaxTilesPerDrawCallUnaligned & (~(TILES_PER_INSTANCE-1));

	FParticleShaderParamRef ShaderParam = GParticleScratchVertexBuffer.GetShaderParam();
	check(ShaderParam);
	FParticleBufferParamRef BufferParam = GParticleScratchVertexBuffer.GetBufferParam();
	check(BufferParam);
	
	int32 TileCount = Tiles.Num();
	int32 FirstTile = 0;

	// Grab shaders.
	TShaderMapRef<FParticleTileVS> VertexShader(GetGlobalShaderMap(FeatureLevel));
	TShaderMapRef<FParticleSimulationClearPS> PixelShader(GetGlobalShaderMap(FeatureLevel));
	
	// Bound shader state.
	
	static FGlobalBoundShaderState BoundShaderState;
	SetGlobalBoundShaderState(
		RHICmdList,
		FeatureLevel,
		BoundShaderState,
		GParticleTileVertexDeclaration.VertexDeclarationRHI,
		*VertexShader,
		*PixelShader,
		0
		);

	while (TileCount > 0)
	{
		// Copy new particles in to the vertex buffer.
		const int32 TilesThisDrawCall = FMath::Min<int32>( TileCount, MaxTilesPerDrawCall );
		const uint32* TilesPtr = Tiles.GetData() + FirstTile;
		BuildTileVertexBuffer( BufferParam, TilesPtr, TilesThisDrawCall );
		
		VertexShader->SetParameters(RHICmdList, ShaderParam);
		DrawAlignedParticleTiles(RHICmdList, ComputeAlignedTileCount(TilesThisDrawCall));
		TileCount -= TilesThisDrawCall;
		FirstTile += TilesThisDrawCall;
	}
}

/*-----------------------------------------------------------------------------
	Injecting particles in to the GPU for simulation.
-----------------------------------------------------------------------------*/

/**
 * Data passed to the GPU to inject a new particle in to the simulation.
 */
struct FNewParticle
{
	/** The initial position of the particle. */
	FVector Position;
	/** The relative time of the particle. */
	float RelativeTime;
	/** The initial velocity of the particle. */
	FVector Velocity;
	/** The time scale for the particle. */
	float TimeScale;
	/** Initial size of the particle. */
	FVector2D Size;
	/** Initial rotation of the particle. */
	float Rotation;
	/** Relative rotation rate of the particle. */
	float RelativeRotationRate;
	/** Coefficient of drag. */
	float DragCoefficient;
	/** Per-particle vector field scale. */
	float VectorFieldScale;
	/** Resilience for collision. */
	union
	{
		float Resilience;
		int32 AllocatedTileIndex;
	} ResilienceAndTileIndex;
	/** Random selection of orbit attributes. */
	float RandomOrbit;
	/** The offset at which to inject the new particle. */
	FVector2D Offset;
};

/**
 * Uniform buffer to hold parameters for particle simulation.
 */
BEGIN_UNIFORM_BUFFER_STRUCT( FParticleInjectionParameters, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( FVector2D, PixelScale )
END_UNIFORM_BUFFER_STRUCT( FParticleInjectionParameters )

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FParticleInjectionParameters,TEXT("ParticleInjection"));

typedef TUniformBufferRef<FParticleInjectionParameters> FParticleInjectionBufferRef;

/**
 * Vertex shader for simulating particles on the GPU.
 */
class FParticleInjectionVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleInjectionVS,Global);

public:

	static bool ShouldCache( EShaderPlatform Platform )
	{
		return SupportsGPUParticles(Platform);
	}

	static void ModifyCompilationEnvironment( EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment )
	{
		FGlobalShader::ModifyCompilationEnvironment( Platform, OutEnvironment );
	}

	/** Default constructor. */
	FParticleInjectionVS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleInjectionVS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
	}

	/**
	 * Sets parameters for particle injection.
	 */
	void SetParameters(FRHICommandList& RHICmdList)
	{
		FParticleInjectionParameters Parameters;
		Parameters.PixelScale.X = 1.0f / GParticleSimulationTextureSizeX;
		Parameters.PixelScale.Y = 1.0f / GParticleSimulationTextureSizeY;
		FParticleInjectionBufferRef UniformBuffer = FParticleInjectionBufferRef::CreateUniformBufferImmediate( Parameters, UniformBuffer_SingleDraw );
		FVertexShaderRHIParamRef VertexShader = GetVertexShader();
		SetUniformBufferParameter(RHICmdList, VertexShader, GetUniformBufferParameter<FParticleInjectionParameters>(), UniformBuffer );
	}
};

/**
 * Pixel shader for simulating particles on the GPU.
 */
class FParticleInjectionPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleInjectionPS,Global);

public:

	static bool ShouldCache( EShaderPlatform Platform )
	{
		return SupportsGPUParticles(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetRenderTargetOutputFormat(0, PF_A32B32G32R32F);
	}

	/** Default constructor. */
	FParticleInjectionPS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleInjectionPS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
	}

	/** Serialization. */
	virtual bool Serialize( FArchive& Ar ) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize( Ar );
		return bShaderHasOutdatedParameters;
	}
};

/** Implementation for all shaders used for particle injection. */
IMPLEMENT_SHADER_TYPE(,FParticleInjectionVS,TEXT("ParticleInjectionShader"),TEXT("VertexMain"),SF_Vertex);
IMPLEMENT_SHADER_TYPE(,FParticleInjectionPS,TEXT("ParticleInjectionShader"),TEXT("PixelMain"),SF_Pixel);

/**
 * Vertex declaration for injecting particles.
 */
class FParticleInjectionVertexDeclaration : public FRenderResource
{
public:

	/** The vertex declaration. */
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	virtual void InitRHI() override
	{
		FVertexDeclarationElementList Elements;

		// Stream 0.
		{
			int32 Offset = 0;
			uint16 Stride = sizeof(FNewParticle);
			// InitialPosition.
			Elements.Add(FVertexElement(0, Offset, VET_Float4, 0, Stride, /*bUseInstanceIndex=*/ true));
			Offset += sizeof(FVector4);
			// InitialVelocity.
			Elements.Add(FVertexElement(0, Offset, VET_Float4, 1, Stride, /*bUseInstanceIndex=*/ true));
			Offset += sizeof(FVector4);
			// RenderAttributes.
			Elements.Add(FVertexElement(0, Offset, VET_Float4, 2, Stride, /*bUseInstanceIndex=*/ true));
			Offset += sizeof(FVector4);
			// SimulationAttributes.
			Elements.Add(FVertexElement(0, Offset, VET_Float4, 3, Stride, /*bUseInstanceIndex=*/ true));
			Offset += sizeof(FVector4);
			// ParticleIndex.
			Elements.Add(FVertexElement(0, Offset, VET_Float2, 4, Stride, /*bUseInstanceIndex=*/ true));
			Offset += sizeof(FVector2D);
		}

		// Stream 1.
		{
			int32 Offset = 0;
			// TexCoord.
			Elements.Add(FVertexElement(1, Offset, VET_Float2, 5, sizeof(FVector2D), /*bUseInstanceIndex=*/ false));
			Offset += sizeof(FVector2D);
		}

		VertexDeclarationRHI = RHICreateVertexDeclaration( Elements );
	}

	virtual void ReleaseRHI() override
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

/** The global particle injection vertex declaration. */
TGlobalResource<FParticleInjectionVertexDeclaration> GParticleInjectionVertexDeclaration;

/**
 * Injects new particles in to the GPU simulation.
 * @param NewParticles - A list of particles to inject in to the simulation.
 */
void InjectNewParticles(FRHICommandList& RHICmdList, ERHIFeatureLevel::Type FeatureLevel, const TArray<FNewParticle>& NewParticles)
{
	const int32 MaxParticlesPerDrawCall = GParticleScratchVertexBufferSize / sizeof(FNewParticle);
	FVertexBufferRHIParamRef ScratchVertexBufferRHI = GParticleScratchVertexBuffer.VertexBufferRHI;
	int32 ParticleCount = NewParticles.Num();
	int32 FirstParticle = 0;

	
	while ( ParticleCount > 0 )
	{
		// Copy new particles in to the vertex buffer.
		const int32 ParticlesThisDrawCall = FMath::Min<int32>( ParticleCount, MaxParticlesPerDrawCall );
		const void* Src = NewParticles.GetData() + FirstParticle;
		void* Dest = RHILockVertexBuffer( ScratchVertexBufferRHI, 0, ParticlesThisDrawCall * sizeof(FNewParticle), RLM_WriteOnly );
		FMemory::Memcpy( Dest, Src, ParticlesThisDrawCall * sizeof(FNewParticle) );
		RHIUnlockVertexBuffer( ScratchVertexBufferRHI );
		ParticleCount -= ParticlesThisDrawCall;
		FirstParticle += ParticlesThisDrawCall;

		// Grab shaders.
		TShaderMapRef<FParticleInjectionVS> VertexShader(GetGlobalShaderMap(FeatureLevel));
		TShaderMapRef<FParticleInjectionPS> PixelShader(GetGlobalShaderMap(FeatureLevel));

		// Bound shader state.
		
		static FGlobalBoundShaderState BoundShaderState;
		SetGlobalBoundShaderState(
			RHICmdList,
			FeatureLevel,
			BoundShaderState,
			GParticleInjectionVertexDeclaration.VertexDeclarationRHI,
			*VertexShader,
			*PixelShader,
			0
			);

		
		VertexShader->SetParameters(RHICmdList);

		// Stream 0: New particles.
		RHICmdList.SetStreamSource(
			0,
			ScratchVertexBufferRHI,
			/*Stride=*/ sizeof(FNewParticle),
			/*Offset=*/ 0
			);

		// Stream 1: TexCoord.
		RHICmdList.SetStreamSource(
			1,
			GParticleTexCoordVertexBuffer.VertexBufferRHI,
			/*Stride=*/ sizeof(FVector2D),
			/*Offset=*/ 0
			);

		// Inject particles.
		RHICmdList.DrawIndexedPrimitive(
			GParticleIndexBuffer.IndexBufferRHI,
			PT_TriangleList,
			/*BaseVertexIndex=*/ 0,
			/*MinIndex=*/ 0,
			/*NumVertices=*/ 4,
			/*StartIndex=*/ 0,
			/*NumPrimitives=*/ 2,
			/*NumInstances=*/ ParticlesThisDrawCall
			);
	}
}

/*-----------------------------------------------------------------------------
	Shaders used for visualizing the state of particle simulation on the GPU.
-----------------------------------------------------------------------------*/

/**
 * Uniform buffer to hold parameters for visualizing particle simulation.
 */
BEGIN_UNIFORM_BUFFER_STRUCT( FParticleSimVisualizeParameters, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( FVector4, ScaleBias )
END_UNIFORM_BUFFER_STRUCT( FParticleSimVisualizeParameters )

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FParticleSimVisualizeParameters,TEXT("PSV"));

typedef TUniformBufferRef<FParticleSimVisualizeParameters> FParticleSimVisualizeBufferRef;

/**
 * Vertex shader for visualizing particle simulation.
 */
class FParticleSimVisualizeVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleSimVisualizeVS,Global);

public:

	static bool ShouldCache( EShaderPlatform Platform )
	{
		return SupportsGPUParticles(Platform);
	}

	/** Default constructor. */
	FParticleSimVisualizeVS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleSimVisualizeVS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
	}

	/**
	 * Set parameters for this shader.
	 */
	void SetParameters(FRHICommandList& RHICmdList, const FParticleSimVisualizeBufferRef& UniformBuffer )
	{
		FVertexShaderRHIParamRef VertexShader = GetVertexShader();
		SetUniformBufferParameter(RHICmdList, VertexShader, GetUniformBufferParameter<FParticleSimVisualizeParameters>(), UniformBuffer );
	}
};

/**
 * Pixel shader for visualizing particle simulation.
 */
class FParticleSimVisualizePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleSimVisualizePS,Global);

public:

	static bool ShouldCache( EShaderPlatform Platform )
	{
		return SupportsGPUParticles(Platform);
	}

	/** Default constructor. */
	FParticleSimVisualizePS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleSimVisualizePS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
		VisualizationMode.Bind( Initializer.ParameterMap, TEXT("VisualizationMode") );
		PositionTexture.Bind( Initializer.ParameterMap, TEXT("PositionTexture") );
		PositionTextureSampler.Bind( Initializer.ParameterMap, TEXT("PositionTextureSampler") );
		CurveTexture.Bind( Initializer.ParameterMap, TEXT("CurveTexture") );
		CurveTextureSampler.Bind( Initializer.ParameterMap, TEXT("CurveTextureSampler") );
	}

	/** Serialization. */
	virtual bool Serialize( FArchive& Ar ) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize( Ar );
		Ar << VisualizationMode;
		Ar << PositionTexture;
		Ar << PositionTextureSampler;
		Ar << CurveTexture;
		Ar << CurveTextureSampler;
		return bShaderHasOutdatedParameters;
	}

	/**
	 * Set parameters for this shader.
	 */
	void SetParameters(FRHICommandList& RHICmdList, int32 InVisualizationMode, FTexture2DRHIParamRef PositionTextureRHI, FTexture2DRHIParamRef CurveTextureRHI )
	{
		FPixelShaderRHIParamRef PixelShader = GetPixelShader();
		SetShaderValue(RHICmdList, PixelShader, VisualizationMode, InVisualizationMode );
		FSamplerStateRHIParamRef SamplerStatePoint = TStaticSamplerState<SF_Point>::GetRHI();
		SetTextureParameter(RHICmdList, PixelShader, PositionTexture, PositionTextureSampler, SamplerStatePoint, PositionTextureRHI );
		SetTextureParameter(RHICmdList, PixelShader, CurveTexture, CurveTextureSampler, SamplerStatePoint, CurveTextureRHI );
	}

private:

	FShaderParameter VisualizationMode;
	FShaderResourceParameter PositionTexture;
	FShaderResourceParameter PositionTextureSampler;
	FShaderResourceParameter CurveTexture;
	FShaderResourceParameter CurveTextureSampler;
};

/** Implementation for all shaders used for visualization. */
IMPLEMENT_SHADER_TYPE(,FParticleSimVisualizeVS,TEXT("ParticleSimVisualizeShader"),TEXT("VertexMain"),SF_Vertex);
IMPLEMENT_SHADER_TYPE(,FParticleSimVisualizePS,TEXT("ParticleSimVisualizeShader"),TEXT("PixelMain"),SF_Pixel);

/**
 * Vertex declaration for particle simulation visualization.
 */
class FParticleSimVisualizeVertexDeclaration : public FRenderResource
{
public:

	/** The vertex declaration. */
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	virtual void InitRHI() override
	{
		FVertexDeclarationElementList Elements;
		Elements.Add(FVertexElement(0, 0, VET_Float2, 0, sizeof(FVector2D)));
		VertexDeclarationRHI = RHICreateVertexDeclaration( Elements );
	}

	virtual void ReleaseRHI() override
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

/** Global vertex declaration resource for particle sim visualization. */
TGlobalResource<FParticleSimVisualizeVertexDeclaration> GParticleSimVisualizeVertexDeclaration;

/**
 * Visualizes the current state of simulation on the GPU.
 * @param RenderTarget - The render target on which to draw the visualization.
 */
static void VisualizeGPUSimulation(
	FRHICommandList& RHICmdList,
	ERHIFeatureLevel::Type FeatureLevel,
	int32 VisualizationMode,
	FRenderTarget* RenderTarget,
	const FParticleStateTextures& StateTextures,
	FTexture2DRHIParamRef CurveTextureRHI
	)
{
	check(IsInRenderingThread());
	SCOPED_DRAW_EVENT(RHICmdList, ParticleSimDebugDraw);

	// Some constants for laying out the debug view.
	const float DisplaySizeX = 256.0f;
	const float DisplaySizeY = 256.0f;
	const float DisplayOffsetX = 60.0f;
	const float DisplayOffsetY = 60.0f;
	
	// Setup render states.
	FIntPoint TargetSize = RenderTarget->GetSizeXY();
	SetRenderTarget(RHICmdList, RenderTarget->GetRenderTargetTexture(), FTextureRHIParamRef());
	RHICmdList.SetViewport(0, 0, 0.0f, TargetSize.X, TargetSize.Y, 1.0f);
	RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
	RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
	RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());

	// Grab shaders.
	TShaderMapRef<FParticleSimVisualizeVS> VertexShader(GetGlobalShaderMap(FeatureLevel));
	TShaderMapRef<FParticleSimVisualizePS> PixelShader(GetGlobalShaderMap(FeatureLevel));

	// Bound shader state.
	
	static FGlobalBoundShaderState BoundShaderState;
	SetGlobalBoundShaderState(
		RHICmdList,
		FeatureLevel,
		BoundShaderState,
		GParticleSimVisualizeVertexDeclaration.VertexDeclarationRHI,
		*VertexShader,
		*PixelShader
		);

	// Parameters for the visualization.
	FParticleSimVisualizeParameters Parameters;
	Parameters.ScaleBias.X = 2.0f * DisplaySizeX / (float)TargetSize.X;
	Parameters.ScaleBias.Y = 2.0f * DisplaySizeY / (float)TargetSize.Y;
	Parameters.ScaleBias.Z = 2.0f * DisplayOffsetX / (float)TargetSize.X - 1.0f;
	Parameters.ScaleBias.W = 2.0f * DisplayOffsetY / (float)TargetSize.Y - 1.0f;
	FParticleSimVisualizeBufferRef UniformBuffer = FParticleSimVisualizeBufferRef::CreateUniformBufferImmediate( Parameters, UniformBuffer_SingleDraw );
	VertexShader->SetParameters(RHICmdList, UniformBuffer);
	PixelShader->SetParameters(RHICmdList, VisualizationMode, StateTextures.PositionTextureRHI, CurveTextureRHI);

	const int32 VertexStride = sizeof(FVector2D);
	
	// Bind vertex stream.
	RHICmdList.SetStreamSource(
		0,
		GParticleTexCoordVertexBuffer.VertexBufferRHI,
		VertexStride,
		/*VertexOffset=*/ 0
		);

	// Draw.
	RHICmdList.DrawIndexedPrimitive(
		GParticleIndexBuffer.IndexBufferRHI,
		PT_TriangleList,
		/*BaseVertexIndex=*/ 0,
		/*MinIndex=*/ 0,
		/*NumVertices=*/ 4,
		/*StartIndex=*/ 0,
		/*NumPrimitives=*/ 2,
		/*NumInstances=*/ 1
		);
}

/**
 * Constructs a particle vertex buffer on the CPU for a given set of tiles.
 * @param VertexBuffer - The buffer with which to fill with particle indices.
 * @param InTiles - The list of tiles for which to generate indices.
 */
static void BuildParticleVertexBuffer( FVertexBufferRHIParamRef VertexBufferRHI, const TArray<uint32>& InTiles )
{
	check( IsInRenderingThread() );

	const int32 TileCount = InTiles.Num();
	const int32 IndexCount = TileCount * GParticlesPerTile;
	const int32 BufferSize = IndexCount * sizeof(FParticleIndex);
	const int32 Stride = 1;
	FParticleIndex* RESTRICT ParticleIndices = (FParticleIndex*)RHILockVertexBuffer( VertexBufferRHI, 0, BufferSize, RLM_WriteOnly );

	for ( int32 Index = 0; Index < TileCount; ++Index )
	{
		const uint32 TileIndex = InTiles[Index];
		const FVector2D TileOffset(
			FMath::Fractional( (float)TileIndex / (float)GParticleSimulationTileCountX ),
			FMath::Fractional( FMath::TruncToFloat( (float)TileIndex / (float)GParticleSimulationTileCountX ) / (float)GParticleSimulationTileCountY )
			);
		for ( int32 ParticleY = 0; ParticleY < GParticleSimulationTileSize; ++ParticleY )
		{
			for ( int32 ParticleX = 0; ParticleX < GParticleSimulationTileSize; ++ParticleX )
			{
				const float IndexX = TileOffset.X + ((float)ParticleX / (float)GParticleSimulationTextureSizeX) + (0.5f / (float)GParticleSimulationTextureSizeX);
				const float IndexY = TileOffset.Y + ((float)ParticleY / (float)GParticleSimulationTextureSizeY) + (0.5f / (float)GParticleSimulationTextureSizeY);
				ParticleIndices->X.SetWithoutBoundsChecks(IndexX);
				ParticleIndices->Y.SetWithoutBoundsChecks(IndexY);					

				// move to next particle
				ParticleIndices += Stride;
			}
		}
	}
	RHIUnlockVertexBuffer( VertexBufferRHI );
}

/*-----------------------------------------------------------------------------
	Determine bounds for GPU particles.
-----------------------------------------------------------------------------*/

/** The number of threads per group used to generate particle keys. */
#define PARTICLE_BOUNDS_THREADS 64

/**
 * Uniform buffer parameters for generating particle bounds.
 */
BEGIN_UNIFORM_BUFFER_STRUCT( FParticleBoundsParameters, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( uint32, ChunksPerGroup )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( uint32, ExtraChunkCount )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( uint32, ParticleCount )
END_UNIFORM_BUFFER_STRUCT( FParticleBoundsParameters )

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FParticleBoundsParameters,TEXT("ParticleBounds"));

typedef TUniformBufferRef<FParticleBoundsParameters> FParticleBoundsUniformBufferRef;

/**
 * Compute shader used to generate particle bounds.
 */
class FParticleBoundsCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleBoundsCS,Global);

public:

	static bool ShouldCache( EShaderPlatform Platform )
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment( EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment )
	{
		FGlobalShader::ModifyCompilationEnvironment( Platform, OutEnvironment );
		OutEnvironment.SetDefine( TEXT("THREAD_COUNT"), PARTICLE_BOUNDS_THREADS );
		OutEnvironment.SetDefine( TEXT("TEXTURE_SIZE_X"), GParticleSimulationTextureSizeX );
		OutEnvironment.SetDefine( TEXT("TEXTURE_SIZE_Y"), GParticleSimulationTextureSizeY );
		OutEnvironment.CompilerFlags.Add( CFLAG_StandardOptimization );
	}

	/** Default constructor. */
	FParticleBoundsCS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleBoundsCS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
		InParticleIndices.Bind( Initializer.ParameterMap, TEXT("InParticleIndices") );
		PositionTexture.Bind( Initializer.ParameterMap, TEXT("PositionTexture") );
		PositionTextureSampler.Bind( Initializer.ParameterMap, TEXT("PositionTextureSampler") );
		OutBounds.Bind( Initializer.ParameterMap, TEXT("OutBounds") );
	}

	/** Serialization. */
	virtual bool Serialize( FArchive& Ar ) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize( Ar );
		Ar << InParticleIndices;
		Ar << PositionTexture;
		Ar << PositionTextureSampler;
		Ar << OutBounds;
		return bShaderHasOutdatedParameters;
	}

	/**
	 * Set output buffers for this shader.
	 */
	void SetOutput(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIParamRef OutBoundsUAV )
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if ( OutBounds.IsBound() )
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutBounds.GetBaseIndex(), OutBoundsUAV);
		}
	}

	/**
	 * Set input parameters.
	 */
	void SetParameters(
		FRHICommandList& RHICmdList,
		FParticleBoundsUniformBufferRef& UniformBuffer,
		FShaderResourceViewRHIParamRef InIndicesSRV,
		FTexture2DRHIParamRef PositionTextureRHI
		)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		SetUniformBufferParameter(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FParticleBoundsParameters>(), UniformBuffer );
		if ( InParticleIndices.IsBound() )
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, InParticleIndices.GetBaseIndex(), InIndicesSRV);
		}
		if ( PositionTexture.IsBound() )
		{
			RHICmdList.SetShaderTexture(ComputeShaderRHI, PositionTexture.GetBaseIndex(), PositionTextureRHI);
		}
	}

	/**
	 * Unbinds any buffers that have been bound.
	 */
	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if ( InParticleIndices.IsBound() )
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, InParticleIndices.GetBaseIndex(), FShaderResourceViewRHIParamRef());
		}
		if ( OutBounds.IsBound() )
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutBounds.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
		}
	}

private:

	/** Input buffer containing particle indices. */
	FShaderResourceParameter InParticleIndices;
	/** Texture containing particle positions. */
	FShaderResourceParameter PositionTexture;
	FShaderResourceParameter PositionTextureSampler;
	/** Output key buffer. */
	FShaderResourceParameter OutBounds;
};
IMPLEMENT_SHADER_TYPE(,FParticleBoundsCS,TEXT("ParticleBoundsShader"),TEXT("ComputeParticleBounds"),SF_Compute);

/**
 * Returns true if the Mins and Maxs consistutue valid bounds, i.e. Mins <= Maxs.
 */
static bool AreBoundsValid( const FVector& Mins, const FVector& Maxs )
{
	return Mins.X <= Maxs.X && Mins.Y <= Maxs.Y && Mins.Z <= Maxs.Z;
}

/**
 * Computes bounds for GPU particles. Note that this is slow as it requires
 * syncing with the GPU!
 * @param VertexBufferSRV - Vertex buffer containing particle indices.
 * @param PositionTextureRHI - Texture holding particle positions.
 * @param ParticleCount - The number of particles in the emitter.
 */
static FBox ComputeParticleBounds(
	FRHICommandListImmediate& RHICmdList,
	FShaderResourceViewRHIParamRef VertexBufferSRV,
	FTexture2DRHIParamRef PositionTextureRHI,
	int32 ParticleCount )
{
	FBox BoundingBox;
	FParticleBoundsParameters Parameters;
	FParticleBoundsUniformBufferRef UniformBuffer;

	if (ParticleCount > 0 && GMaxRHIFeatureLevel == ERHIFeatureLevel::SM5)
	{
		// Determine how to break the work up over individual work groups.
		const uint32 MaxGroupCount = 128;
		const uint32 AlignedParticleCount = ((ParticleCount + PARTICLE_BOUNDS_THREADS - 1) & (~(PARTICLE_BOUNDS_THREADS - 1)));
		const uint32 ChunkCount = AlignedParticleCount / PARTICLE_BOUNDS_THREADS;
		const uint32 GroupCount = FMath::Clamp<uint32>( ChunkCount, 1, MaxGroupCount );

		// Create the uniform buffer.
		Parameters.ChunksPerGroup = ChunkCount / GroupCount;
		Parameters.ExtraChunkCount = ChunkCount % GroupCount;
		Parameters.ParticleCount = ParticleCount;
		UniformBuffer = FParticleBoundsUniformBufferRef::CreateUniformBufferImmediate( Parameters, UniformBuffer_SingleFrame );

		// Create a buffer for storing bounds.
		const int32 BufferSize = GroupCount * 2 * sizeof(FVector4);
		FRHIResourceCreateInfo CreateInfo;
		FVertexBufferRHIRef BoundsVertexBufferRHI = RHICreateVertexBuffer(
			BufferSize,
			BUF_Static | BUF_UnorderedAccess,
			CreateInfo);
		FUnorderedAccessViewRHIRef BoundsVertexBufferUAV = RHICreateUnorderedAccessView(
			BoundsVertexBufferRHI,
			PF_A32B32G32R32F );

		// Grab the shader.
		TShaderMapRef<FParticleBoundsCS> ParticleBoundsCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		RHICmdList.SetComputeShader(ParticleBoundsCS->GetComputeShader());

		// Dispatch shader to compute bounds.
		ParticleBoundsCS->SetOutput(RHICmdList, BoundsVertexBufferUAV);
		ParticleBoundsCS->SetParameters(RHICmdList, UniformBuffer, VertexBufferSRV, PositionTextureRHI);
		DispatchComputeShader(
			RHICmdList, 
			*ParticleBoundsCS, 
			GroupCount,
			1,
			1 );
		ParticleBoundsCS->UnbindBuffers(RHICmdList);

		// Read back bounds.
		FVector4* GroupBounds = (FVector4*)RHILockVertexBuffer( BoundsVertexBufferRHI, 0, BufferSize, RLM_ReadOnly );

		// Find valid starting bounds.
		uint32 GroupIndex = 0;
		do
		{
			BoundingBox.Min = FVector(GroupBounds[GroupIndex * 2 + 0]);
			BoundingBox.Max = FVector(GroupBounds[GroupIndex * 2 + 1]);
			GroupIndex++;
		} while ( GroupIndex < GroupCount && !AreBoundsValid( BoundingBox.Min, BoundingBox.Max ) );

		if ( GroupIndex == GroupCount )
		{
			// No valid bounds!
			BoundingBox.Init();
		}
		else
		{
			// Bounds are valid. Add any other valid bounds.
			BoundingBox.IsValid = true;
			while ( GroupIndex < GroupCount )
			{
				const FVector Mins( GroupBounds[GroupIndex * 2 + 0] );
				const FVector Maxs( GroupBounds[GroupIndex * 2 + 1] );
				if ( AreBoundsValid( Mins, Maxs ) )
				{
					BoundingBox += Mins;
					BoundingBox += Maxs;
				}
				GroupIndex++;
			}
		}

		// Release buffer.
		RHICmdList.UnlockVertexBuffer(BoundsVertexBufferRHI);
		BoundsVertexBufferUAV.SafeRelease();
		BoundsVertexBufferRHI.SafeRelease();
	}
	else
	{
		BoundingBox.Init();
	}

	return BoundingBox;
}

// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
/*-----------------------------------------------------------------------------
Calculate the density in the grid for GPU particles.
-----------------------------------------------------------------------------*/
#define SCALE_FACTOR	(0.02f)

BEGIN_UNIFORM_BUFFER_STRUCT(FGridDensityFrustumUniformParameters, )
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix, DimMatrix)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector2D, NearDim)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector2D, FarDim)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, DimZ)
END_UNIFORM_BUFFER_STRUCT(FGridDensityFrustumUniformParameters)

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FGridDensityFrustumUniformParameters, TEXT("GridDensityFrustum"));

typedef TUniformBufferRef<FGridDensityFrustumUniformParameters> FGridDensityFrustumUniformBufferRef;


BEGIN_UNIFORM_BUFFER_STRUCT(FGridDensityUniformParameters, )
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int32, GridMaxCellCount)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int32, GridResolution)
END_UNIFORM_BUFFER_STRUCT(FGridDensityUniformParameters)

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FGridDensityUniformParameters, TEXT("GridDensity"));

typedef TUniformBufferRef<FGridDensityUniformParameters> FGridDensityUniformBufferRef;


class FParticleGridDensityResource : public FRenderResource
{
public:
	FStructuredBufferRHIRef		GridDensityBuffer;
	FShaderResourceViewRHIRef	GridDensityBufferSRV;
	FUnorderedAccessViewRHIRef	GridDensityBufferUAV;

	FStructuredBufferRHIRef		GridDensityLowPassBuffer;
	FShaderResourceViewRHIRef	GridDensityLowPassBufferSRV;
	FUnorderedAccessViewRHIRef	GridDensityLowPassBufferUAV;

	FGridDensityUniformBufferRef GridDensityUniformBuffer;

	int32						GridResolution;
	int32						GridMaxCellCount;
	float						GridDepth;

	FParticleGridDensityResource()
		: GridResolution(0)
		, GridMaxCellCount(0)
		, GridDepth(0)
	{
	}

	/**
	* Initialize RHI resources used for particle simulation.
	*/
	virtual void InitRHI() override
	{
		if (GMaxRHIFeatureLevel == ERHIFeatureLevel::SM5)
		{
			check(GridResolution);

			FRHIResourceCreateInfo CreateInfo;
			GridDensityBuffer = RHICreateStructuredBuffer(sizeof(uint32), GridResolution * GridResolution * GridResolution * sizeof(uint32), BUF_ShaderResource | BUF_UnorderedAccess, CreateInfo);
			GridDensityBufferSRV = RHICreateShaderResourceView(GridDensityBuffer);
			GridDensityBufferUAV = RHICreateUnorderedAccessView(GridDensityBuffer, false, false);

			GridDensityLowPassBuffer = RHICreateStructuredBuffer(sizeof(float), GridResolution * GridResolution * GridResolution * sizeof(float), BUF_ShaderResource | BUF_UnorderedAccess, CreateInfo);
			GridDensityLowPassBufferSRV = RHICreateShaderResourceView(GridDensityLowPassBuffer);
			GridDensityLowPassBufferUAV = RHICreateUnorderedAccessView(GridDensityLowPassBuffer, false, false);

			FGridDensityUniformParameters		GridDensityParameters;
			GridDensityParameters.GridResolution = GridResolution;
			GridDensityParameters.GridMaxCellCount = GridMaxCellCount;
			GridDensityUniformBuffer = FGridDensityUniformBufferRef::CreateUniformBufferImmediate(GridDensityParameters, UniformBuffer_MultiFrame);
		}
	}

	bool IsValid() const
	{
		return IsValidRef(GridDensityLowPassBufferUAV) && IsValidRef(GridDensityBufferUAV);
	}

	/**
	* Releases RHI resources used for particle simulation.
	*/
	virtual void ReleaseRHI() override
	{
		GridDensityLowPassBufferUAV.SafeRelease();
		GridDensityLowPassBufferSRV.SafeRelease();
		GridDensityLowPassBuffer.SafeRelease();

		GridDensityBufferUAV.SafeRelease();
		GridDensityBufferSRV.SafeRelease();
		GridDensityBuffer.SafeRelease();

		GridDensityUniformBuffer.SafeRelease();
	}
};

/** compute shader for clear density */
class FParticleGridDensityClearCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleGridDensityClearCS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADS_NUM"), PARTICLE_BOUNDS_THREADS);
	}

	/** Default constructor. */
	FParticleGridDensityClearCS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleGridDensityClearCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		OutGridDensity.Bind(Initializer.ParameterMap, TEXT("OutGridDensity"));
	}

	/** Serialization. */
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << OutGridDensity;
		return bShaderHasOutdatedParameters;
	}

	/**
	* Set output buffer for this shader.
	*/
	void SetOutput(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIParamRef GridDensityUAV)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (OutGridDensity.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutGridDensity.GetBaseIndex(), GridDensityUAV);
		}
	}

	/**
	* Unbinds any buffers that have been bound.
	*/
	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (OutGridDensity.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutGridDensity.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
		}
	}

private:

	FShaderResourceParameter OutGridDensity;
};
IMPLEMENT_SHADER_TYPE(, FParticleGridDensityClearCS, TEXT("ParticleGridDensityShaders"), TEXT("GridDensityClear"), SF_Compute);

/** compute shader for fill the grid density buffer */
class FParticleGridDensityFillFrustumCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleGridDensityFillFrustumCS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), PARTICLE_BOUNDS_THREADS);
		OutEnvironment.SetDefine(TEXT("TEXTURE_SIZE_X"), GParticleSimulationTextureSizeX);
		OutEnvironment.SetDefine(TEXT("TEXTURE_SIZE_Y"), GParticleSimulationTextureSizeY);
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	/** Default constructor. */
	FParticleGridDensityFillFrustumCS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleGridDensityFillFrustumCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		InParticleIndices.Bind(Initializer.ParameterMap, TEXT("InParticleIndices"));
		PositionTexture.Bind(Initializer.ParameterMap, TEXT("PositionTexture"));
		PositionTextureSampler.Bind(Initializer.ParameterMap, TEXT("PositionTextureSampler"));
		OutGridDensity.Bind(Initializer.ParameterMap, TEXT("OutGridDensity"));
	}

	/** Serialization. */
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << InParticleIndices;
		Ar << PositionTexture;
		Ar << PositionTextureSampler;
		Ar << OutGridDensity;
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(
		FRHICommandList& RHICmdList,
		const FGridDensityFrustumUniformBufferRef& GridDensityFrustumUniformBuffer,
		const FGridDensityUniformBufferRef& GridDensityUniformBuffer,
		FParticleBoundsUniformBufferRef& UniformBuffer,
		FShaderResourceViewRHIParamRef InIndicesSRV,
		FTexture2DRHIParamRef PositionTextureRHI
		)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		SetUniformBufferParameter(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FGridDensityFrustumUniformParameters>(), GridDensityFrustumUniformBuffer);
		SetUniformBufferParameter(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FGridDensityUniformParameters>(), GridDensityUniformBuffer);
		SetUniformBufferParameter(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FParticleBoundsParameters>(), UniformBuffer);

		if (InParticleIndices.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, InParticleIndices.GetBaseIndex(), InIndicesSRV);
		}
		if (PositionTexture.IsBound())
		{
			RHICmdList.SetShaderTexture(ComputeShaderRHI, PositionTexture.GetBaseIndex(), PositionTextureRHI);
		}
	}


	/**
	* Set output buffer for this shader.
	*/
	void SetOutput(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIParamRef GridDensityUAV)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (OutGridDensity.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutGridDensity.GetBaseIndex(), GridDensityUAV);
		}
	}

	/**
	* Unbinds any buffers that have been bound.
	*/
	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		if (InParticleIndices.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, InParticleIndices.GetBaseIndex(), FShaderResourceViewRHIParamRef());
		}
		if (OutGridDensity.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutGridDensity.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
		}
	}

private:

	/** Input buffer containing particle indices. */
	FShaderResourceParameter InParticleIndices;
	/** Texture containing particle positions. */
	FShaderResourceParameter PositionTexture;
	FShaderResourceParameter PositionTextureSampler;
	FShaderResourceParameter OutGridDensity;
};
IMPLEMENT_SHADER_TYPE(, FParticleGridDensityFillFrustumCS, TEXT("ParticleGridDensityFrustumShaders"), TEXT("GridDensityFillFrustum"), SF_Compute);

static void ComputeGridDensityFillFrustum(
	FRHICommandList& RHICmdList,
	ERHIFeatureLevel::Type FeatureLevel,
	const FGridDensityFrustumUniformBufferRef& GridDensityFrustumUniformBuffer,
	const FGridDensityUniformBufferRef& GridDensityUniformBuffer,
	FShaderResourceViewRHIParamRef VertexBufferSRV,
	FTexture2DRHIParamRef PositionTextureRHI,
	FUnorderedAccessViewRHIParamRef GridDensityUAV,
	int32 ParticleCount)
{
	FParticleBoundsParameters Parameters;
	FParticleBoundsUniformBufferRef UniformBuffer;

	if (ParticleCount > 0 && GMaxRHIFeatureLevel == ERHIFeatureLevel::SM5)
	{
		// Determine how to break the work up over individual work groups.
		const uint32 MaxGroupCount = 128;
		const uint32 AlignedParticleCount = ((ParticleCount + PARTICLE_BOUNDS_THREADS - 1) & (~(PARTICLE_BOUNDS_THREADS - 1)));
		const uint32 ChunkCount = AlignedParticleCount / PARTICLE_BOUNDS_THREADS;
		const uint32 GroupCount = FMath::Clamp<uint32>(ChunkCount, 1, MaxGroupCount);

		// Create the uniform buffer.
		Parameters.ChunksPerGroup = ChunkCount / GroupCount;
		Parameters.ExtraChunkCount = ChunkCount % GroupCount;
		Parameters.ParticleCount = ParticleCount;
		UniformBuffer = FParticleBoundsUniformBufferRef::CreateUniformBufferImmediate(Parameters, UniformBuffer_SingleFrame);

		// Grab the shader.
		TShaderMapRef<FParticleGridDensityFillFrustumCS> GridDensityFillFrustumCS(GetGlobalShaderMap(FeatureLevel));
		RHICmdList.SetComputeShader(GridDensityFillFrustumCS->GetComputeShader());
		// Dispatch shader to compute bounds.
		GridDensityFillFrustumCS->SetParameters(RHICmdList,
			GridDensityFrustumUniformBuffer,
			GridDensityUniformBuffer,
			UniformBuffer,
			VertexBufferSRV,
			PositionTextureRHI);
		GridDensityFillFrustumCS->SetOutput(RHICmdList, GridDensityUAV);
		DispatchComputeShader(
			RHICmdList,
			*GridDensityFillFrustumCS,
			GroupCount,
			1,
			1);
		GridDensityFillFrustumCS->UnbindBuffers(RHICmdList);
	}
}

class FParticleGridDensityLowPassCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleGridDensityLowPassCS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADS_NUM"), PARTICLE_BOUNDS_THREADS);
	}

	/** Default constructor. */
	FParticleGridDensityLowPassCS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleGridDensityLowPassCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		InGridDensity.Bind(Initializer.ParameterMap, TEXT("InGridDensity"));
		OutGridDensityLowPass.Bind(Initializer.ParameterMap, TEXT("OutGridDensityLowPass"));
	}

	/** Serialization. */
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << InGridDensity;
		Ar << OutGridDensityLowPass;
		return bShaderHasOutdatedParameters;
	}

	/**
	* Set parameters for this shader.
	*/
	void SetParameters(
		FRHICommandList& RHICmdList,
		const FGridDensityUniformBufferRef& UniformBuffer,
		FShaderResourceViewRHIParamRef InGridDensitySRV)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		SetUniformBufferParameter(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FGridDensityUniformParameters>(), UniformBuffer);

		if (InGridDensity.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, InGridDensity.GetBaseIndex(), InGridDensitySRV);
		}
	}

	/**
	* Set output buffer for this shader.
	*/
	void SetOutput(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIParamRef GridDensityLowPassUAV)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (OutGridDensityLowPass.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutGridDensityLowPass.GetBaseIndex(), GridDensityLowPassUAV);
		}
	}

	/**
	* Unbinds any buffers that have been bound.
	*/
	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		if (InGridDensity.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, InGridDensity.GetBaseIndex(), FShaderResourceViewRHIParamRef());
		}
		if (OutGridDensityLowPass.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutGridDensityLowPass.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
		}
	}

private:

	/** Texture containing particle positions. */
	FShaderResourceParameter InGridDensity;

	FShaderResourceParameter OutGridDensityLowPass;
};
IMPLEMENT_SHADER_TYPE(, FParticleGridDensityLowPassCS, TEXT("ParticleGridDensityShaders"), TEXT("GridDensityLowPass"), SF_Compute);

class FParticleGridDensityApplyFrustumCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleGridDensityApplyFrustumCS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), PARTICLE_BOUNDS_THREADS);
		OutEnvironment.SetDefine(TEXT("TEXTURE_SIZE_X"), GParticleSimulationTextureSizeX);
		OutEnvironment.SetDefine(TEXT("TEXTURE_SIZE_Y"), GParticleSimulationTextureSizeY);
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	/** Default constructor. */
	FParticleGridDensityApplyFrustumCS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleGridDensityApplyFrustumCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		InParticleIndices.Bind(Initializer.ParameterMap, TEXT("InParticleIndices"));
		PositionTexture.Bind(Initializer.ParameterMap, TEXT("PositionTexture"));
		PositionTextureSampler.Bind(Initializer.ParameterMap, TEXT("PositionTextureSampler"));
		InGridDensity.Bind(Initializer.ParameterMap, TEXT("InGridDensity"));
		OutDensityTexture.Bind(Initializer.ParameterMap, TEXT("OutDensityTexture"));
	}

	/** Serialization. */
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << InParticleIndices;
		Ar << PositionTexture;
		Ar << PositionTextureSampler;
		Ar << InGridDensity;
		Ar << OutDensityTexture;
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(
		FRHICommandList& RHICmdList,
		const FGridDensityFrustumUniformBufferRef& GridDensityFrustumUniformBuffer,
		const FGridDensityUniformBufferRef& GridDensityUniformBuffer,
		FParticleBoundsUniformBufferRef& UniformBuffer,
		FShaderResourceViewRHIParamRef InIndicesSRV,
		FTexture2DRHIParamRef PositionTextureRHI,
		FShaderResourceViewRHIParamRef InGridDensitySRV
		)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		SetUniformBufferParameter(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FGridDensityFrustumUniformParameters>(), GridDensityFrustumUniformBuffer);
		SetUniformBufferParameter(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FGridDensityUniformParameters>(), GridDensityUniformBuffer);
		SetUniformBufferParameter(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FParticleBoundsParameters>(), UniformBuffer);

		if (InParticleIndices.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, InParticleIndices.GetBaseIndex(), InIndicesSRV);
		}
		if (PositionTexture.IsBound())
		{
			RHICmdList.SetShaderTexture(ComputeShaderRHI, PositionTexture.GetBaseIndex(), PositionTextureRHI);
		}
		if (InGridDensity.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, InGridDensity.GetBaseIndex(), InGridDensitySRV);
		}
	}


	/**
	* Set output buffer for this shader.
	*/
	void SetOutput(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIParamRef DensityTextureUAV)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (OutDensityTexture.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutDensityTexture.GetBaseIndex(), DensityTextureUAV);
		}
	}

	/**
	* Unbinds any buffers that have been bound.
	*/
	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		if (InParticleIndices.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, InParticleIndices.GetBaseIndex(), FShaderResourceViewRHIParamRef());
		}

		if (InGridDensity.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, InGridDensity.GetBaseIndex(), FShaderResourceViewRHIParamRef());
		}

		if (OutDensityTexture.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutDensityTexture.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
		}
	}

private:

	/** Input buffer containing particle indices. */
	FShaderResourceParameter InParticleIndices;
	/** Texture containing particle positions. */
	FShaderResourceParameter PositionTexture;
	FShaderResourceParameter PositionTextureSampler;
	FShaderResourceParameter InGridDensity;

	FShaderResourceParameter OutDensityTexture;
};
IMPLEMENT_SHADER_TYPE(, FParticleGridDensityApplyFrustumCS, TEXT("ParticleGridDensityFrustumShaders"), TEXT("GridDensityApplyFrustum"), SF_Compute);

static void ComputeGridDensityApplyFrustum(
	FRHICommandList& RHICmdList,
	ERHIFeatureLevel::Type FeatureLevel,
	const FGridDensityFrustumUniformBufferRef& GridDensityFrustumUniformBuffer,
	const FGridDensityUniformBufferRef& GridDensityUniformBuffer,
	FShaderResourceViewRHIParamRef VertexBufferSRV,
	FTexture2DRHIParamRef PositionTextureRHI,
	FShaderResourceViewRHIParamRef GridDensitySRV,
	FUnorderedAccessViewRHIParamRef DensityTextureUAV,
	int32 ParticleCount)
{
	FParticleBoundsParameters Parameters;
	FParticleBoundsUniformBufferRef UniformBuffer;

	if (ParticleCount > 0 && GMaxRHIFeatureLevel == ERHIFeatureLevel::SM5)
	{
		// Determine how to break the work up over individual work groups.
		const uint32 MaxGroupCount = 128;
		const uint32 AlignedParticleCount = ((ParticleCount + PARTICLE_BOUNDS_THREADS - 1) & (~(PARTICLE_BOUNDS_THREADS - 1)));
		const uint32 ChunkCount = AlignedParticleCount / PARTICLE_BOUNDS_THREADS;
		const uint32 GroupCount = FMath::Clamp<uint32>(ChunkCount, 1, MaxGroupCount);

		// Create the uniform buffer.
		Parameters.ChunksPerGroup = ChunkCount / GroupCount;
		Parameters.ExtraChunkCount = ChunkCount % GroupCount;
		Parameters.ParticleCount = ParticleCount;
		UniformBuffer = FParticleBoundsUniformBufferRef::CreateUniformBufferImmediate(Parameters, UniformBuffer_SingleFrame);

		// Grab the shader.
		TShaderMapRef<FParticleGridDensityApplyFrustumCS> GridDensityApplyFrustumCS(GetGlobalShaderMap(FeatureLevel));
		RHICmdList.SetComputeShader(GridDensityApplyFrustumCS->GetComputeShader());
		// Dispatch shader to compute bounds.
		GridDensityApplyFrustumCS->SetParameters(RHICmdList,
			GridDensityFrustumUniformBuffer,
			GridDensityUniformBuffer,
			UniformBuffer,
			VertexBufferSRV,
			PositionTextureRHI,
			GridDensitySRV);
		GridDensityApplyFrustumCS->SetOutput(RHICmdList, DensityTextureUAV);
		DispatchComputeShader(
			RHICmdList,
			*GridDensityApplyFrustumCS,
			GroupCount,
			1,
			1);
		GridDensityApplyFrustumCS->UnbindBuffers(RHICmdList);
	}
}
// NVCHANGE_END: JCAO - Grid Density with GPU particles

/*-----------------------------------------------------------------------------
	Per-emitter GPU particle simulation.
-----------------------------------------------------------------------------*/

/**
 * Per-emitter resources for simulation.
 */
struct FParticleEmitterSimulationResources
{
	/** Emitter uniform buffer used for simulation. */
	FParticleSimulationBufferRef SimulationUniformBuffer;
	/** Scale to apply to global vector fields. */
	float GlobalVectorFieldScale;
	/** Tightness override value to apply to global vector fields. */
	float GlobalVectorFieldTightness;
};

/**
 * Vertex buffer used to hold tile offsets.
 */
class FParticleTileVertexBuffer : public FVertexBuffer
{
public:
	/** Shader resource of the vertex buffer. */
	FShaderResourceViewRHIRef VertexBufferSRV;
	/** The number of tiles held by this vertex buffer. */
	int32 TileCount;
	/** The number of tiles held by this vertex buffer, aligned for tile rendering. */
	int32 AlignedTileCount;

	/** Default constructor. */
	FParticleTileVertexBuffer()
		: TileCount(0)
		, AlignedTileCount(0)
	{
	}
	
	
	FParticleShaderParamRef GetShaderParam() { return VertexBufferSRV; }

	/**
	 * Initializes the vertex buffer from a list of tiles.
	 */
	void Init( const TArray<uint32>& Tiles )
	{
		check( IsInRenderingThread() );
		TileCount = Tiles.Num();
		AlignedTileCount = ComputeAlignedTileCount(TileCount);
		InitResource();
		if ( Tiles.Num() )
		{
			BuildTileVertexBuffer( VertexBufferRHI, Tiles );
		}
	}

	/**
	 * Initialize RHI resources.
	 */
	virtual void InitRHI() override
	{
		if ( AlignedTileCount > 0 )
		{
			const int32 TileBufferSize = AlignedTileCount * sizeof(FVector2D);
			check(TileBufferSize > 0);
			FRHIResourceCreateInfo CreateInfo;
			VertexBufferRHI = RHICreateVertexBuffer( TileBufferSize, BUF_Static | BUF_KeepCPUAccessible | BUF_ShaderResource, CreateInfo );
			VertexBufferSRV = RHICreateShaderResourceView( VertexBufferRHI, /*Stride=*/ sizeof(FVector2D), PF_G32R32F );
		}
	}

	/**
	 * Release RHI resources.
	 */
	virtual void ReleaseRHI() override
	{
		TileCount = 0;
		AlignedTileCount = 0;
		VertexBufferSRV.SafeRelease();
		FVertexBuffer::ReleaseRHI();
	}
};

/**
 * Vertex buffer used to hold particle indices.
 */
class FGPUParticleVertexBuffer : public FParticleIndicesVertexBuffer
{
public:

	/** The number of particles referenced by this vertex buffer. */
	int32 ParticleCount;

	/** Default constructor. */
	FGPUParticleVertexBuffer()
		: ParticleCount(0)
	{
	}

	/**
	 * Initializes the vertex buffer from a list of tiles.
	 */
	void Init( const TArray<uint32>& Tiles )
	{
		check( IsInRenderingThread() );
		ParticleCount = Tiles.Num() * GParticlesPerTile;
		InitResource();
		if ( Tiles.Num() )
		{
			BuildParticleVertexBuffer( VertexBufferRHI, Tiles );
		}
	}

	/** Initialize RHI resources. */
	virtual void InitRHI() override
	{
		if ( ParticleCount > 0 && RHISupportsGPUParticles(GetFeatureLevel()) )
		{
			const int32 BufferStride = sizeof(FParticleIndex);
			const int32 BufferSize = ParticleCount * BufferStride;
			uint32 Flags = BUF_Static | /*BUF_KeepCPUAccessible | */BUF_ShaderResource;
			FRHIResourceCreateInfo CreateInfo;
			VertexBufferRHI = RHICreateVertexBuffer(BufferSize, Flags, CreateInfo);
			VertexBufferSRV = RHICreateShaderResourceView(VertexBufferRHI, BufferStride, PF_G16R16F);
		}
	}
};

// NVCHANGE_BEGIN: JCAO - Field Sampler Module for GPU particle
#if WITH_APEX_TURBULENCE
struct FFieldSamplerResources
{
	FApexFieldSamplerActor* Actor;
	FFieldSamplerInstance*	Instance;

	FFieldSamplerResources()
		: Actor(NULL)
		, Instance(NULL)
	{
	}
};
#endif
// NVCHANGE_END: JCAO - Field Sampler Module for GPU particle

/**
 * Resources for simulating a set of particles on the GPU.
 */
class FParticleSimulationGPU
{
public:

	/** The vertex buffer used to access tiles in the simulation. */
	FParticleTileVertexBuffer TileVertexBuffer;
	/** The per-emitter simulation resources. */
	const FParticleEmitterSimulationResources* EmitterSimulationResources;

	// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
	/** the per-emitter grid density resources. */
	const FParticleGridDensityResource* GridDensityResource;
	/** if the view updated? */
	bool bViewMatricesUpdated;
	/** view matrices for the per-emitter. */
	FViewMatrices ViewMatrices;
	// NVCHANGE_END: JCAO - Grid Density with GPU particles

	/** The per-frame simulation uniform buffer. */
	FParticlePerFrameSimulationParameters PerFrameSimulationParameters;
	/** Bounds for particles in the simulation. */
	FBox Bounds;

	/** A list of new particles to inject in to the simulation for this emitter. */
	TArray<FNewParticle> NewParticles;
	/** A list of tiles to clear that were newly allocated for this emitter. */
	TArray<uint32> TilesToClear;

	/** Local vector field. */
	FVectorFieldInstance LocalVectorField;

	/** The vertex buffer used to access particles in the simulation. */
	FGPUParticleVertexBuffer VertexBuffer;
	/** The vertex factory for visualizing the local vector field. */
	FVectorFieldVisualizationVertexFactory* VectorFieldVisualizationVertexFactory;

	/** The simulation index within the associated FX system. */
	int32 SimulationIndex;

	/**
	 * The phase in which these particles should simulate.
	 */
	EParticleSimulatePhase::Type SimulationPhase;

	/** True if the simulation wants collision enabled. */
	bool bWantsCollision;

	EParticleCollisionMode::Type CollisionMode;

	/** Flag that specifies the simulation's resources are dirty and need to be updated. */
	bool bDirty_GameThread;
	bool bReleased_GameThread;
	bool bDestroyed_GameThread;

	// NVCHANGE_BEGIN: JCAO - Field Sampler Module for GPU particle
#if WITH_APEX_TURBULENCE
	TArray<FFieldSamplerResources>	LocalFieldSamplers;
#endif // WITH_APEX_TURBULENCE
	// NVCHANGE_END: JCAO - Field Sampler Module for GPU particle

	/** Default constructor. */
	FParticleSimulationGPU()
		: EmitterSimulationResources(NULL)
		, VectorFieldVisualizationVertexFactory(NULL)
		, SimulationIndex(INDEX_NONE)
		, SimulationPhase(EParticleSimulatePhase::Main)
		, bWantsCollision(false)
		, CollisionMode(EParticleCollisionMode::SceneDepth)
		, bDirty_GameThread(true)
		, bReleased_GameThread(true)
		, bDestroyed_GameThread(false)
		// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
		, GridDensityResource(NULL)
		, bViewMatricesUpdated(false)
		// NVCHANGE_END: JCAO - Grid Density with GPU particles
	{
	}

	/** Destructor. */
	~FParticleSimulationGPU()
	{
		delete VectorFieldVisualizationVertexFactory;
		VectorFieldVisualizationVertexFactory = NULL;
	}

	/**
	 * Initializes resources for simulating particles on the GPU.
	 * @param Tiles							The list of tiles to include in the simulation.
	 * @param InEmitterSimulationResources	The emitter resources used by this simulation.
	 */
	// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
	void InitResources(const TArray<uint32>& Tiles, const FParticleEmitterSimulationResources* InEmitterSimulationResources, const FParticleGridDensityResource* InGridDensityResource)
	{
		check(InEmitterSimulationResources);

		struct FInitResources
		{
			const FParticleEmitterSimulationResources* InEmitterSimulationResources;
			const FParticleGridDensityResource* InGridDensityResource;
		};

		FInitResources InitResources;
		InitResources.InEmitterSimulationResources = InEmitterSimulationResources;
		InitResources.InGridDensityResource = InGridDensityResource;

		ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
			FInitParticleSimulationGPUCommand,
			FParticleSimulationGPU*, Simulation, this,
			TArray<uint32>, Tiles, Tiles,
			FInitResources, InitResources, InitResources,
		{
			// Release vertex buffers.
			Simulation->VertexBuffer.ReleaseResource();
			Simulation->TileVertexBuffer.ReleaseResource();

			// Initialize new buffers with list of tiles.
			Simulation->VertexBuffer.Init(Tiles);
			Simulation->TileVertexBuffer.Init(Tiles);

			// Store simulation resources for this emitter.
			Simulation->EmitterSimulationResources = InitResources.InEmitterSimulationResources;

			// store grid density resource
			Simulation->GridDensityResource = InitResources.InGridDensityResource;

			// If a visualization vertex factory has been created, initialize it.
			if (Simulation->VectorFieldVisualizationVertexFactory)
			{
				Simulation->VectorFieldVisualizationVertexFactory->InitResource();
			}
		});
		bDirty_GameThread = false;
		bReleased_GameThread = false;
	}
	// NVCHANGE_END: JCAO - Grid Density with GPU particles

	/**
	 * Create and initializes a visualization vertex factory if needed.
	 */
	void CreateVectorFieldVisualizationVertexFactory()
	{
		if (VectorFieldVisualizationVertexFactory == NULL)
		{
			check(IsInRenderingThread());
			VectorFieldVisualizationVertexFactory = new FVectorFieldVisualizationVertexFactory();
			VectorFieldVisualizationVertexFactory->InitResource();
		}
	}

	/**
	 * Release and destroy simulation resources.
	 */
	void Destroy()
	{
		bDestroyed_GameThread = true;
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FReleaseParticleSimulationGPUCommand,
			FParticleSimulationGPU*, Simulation, this,
		{
			Simulation->Destroy_RenderThread();
		});
	}

	/**
	 * Destroy the simulation on the rendering thread.
	 */
	void Destroy_RenderThread()
	{
		// The check for GIsRequestingExit is done because at shut down UWorld can be destroyed before particle emitters(?)
		check(GIsRequestingExit || SimulationIndex == INDEX_NONE);
		ReleaseRenderResources();
		delete this;
	}

	/**
	 * Enqueues commands to release render resources.
	 */
	void BeginReleaseResources()
	{
		bReleased_GameThread = true;
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FReleaseParticleSimulationResourcesGPUCommand,
			FParticleSimulationGPU*, Simulation, this,
		{
			Simulation->ReleaseRenderResources();
		});
	}

	// NVCHANGE_BEGIN: JCAO - Field Sampler Module for GPU particle
#if WITH_APEX_TURBULENCE
	void InitLocalFieldSamplers(FPhysScene* InPhysScene, uint32 SceneType, UParticleSystemComponent* InComponent, FFXSystem* FXSystem, TArray<FGPUSpriteLocalFieldSamplerInfo>& LocalFieldSamplerInfo)
	{
		if (LocalFieldSamplers.Num() > 0)
		{
			// Only create once.
			return;
		}

		for (int32 FieldSamplerIndex = 0; FieldSamplerIndex < LocalFieldSamplerInfo.Num(); ++FieldSamplerIndex)
		{
			FGPUSpriteLocalFieldSamplerInfo& FieldSamplerInfo = LocalFieldSamplerInfo[FieldSamplerIndex];

			FFieldSamplerResources Resource;

			if (FieldSamplerInfo.FieldSamplerAsset)
			{
				FMatrix LocalToWorld = FieldSamplerInfo.Transform.ToMatrixWithScale() * InComponent->ComponentToWorld.ToMatrixWithScale();

				switch (FieldSamplerInfo.FieldSamplerAsset->GetType())
				{
				case EFieldSamplerAssetType::EFSAT_ATTRACTOR:
				{
					UAttractorAsset* AttractorAsset = Cast<UAttractorAsset>(FieldSamplerInfo.FieldSamplerAsset);
					FAttractorFSInstance* Instance = new FAttractorFSInstance();

					Instance->FSType = EFieldSamplerAssetType::EFSAT_ATTRACTOR;
					Instance->WorldBounds = FBox::BuildAABB(LocalToWorld.GetOrigin(), FVector(AttractorAsset->Radius));
					Instance->Origin = LocalToWorld.GetOrigin();
					Instance->Radius = AttractorAsset->Radius;
					Instance->ConstFieldStrength = AttractorAsset->ConstFieldStrength;
					Instance->VariableFieldStrength = AttractorAsset->VariableFieldStrength;
					Instance->bEnabled = true;

					Resource.Instance = Instance;
				}
				break;
				case EFieldSamplerAssetType::EFSAT_JET:
				{
					FApexJetActor* JetActor = new FApexJetActor(NULL, InPhysScene, SceneType, FieldSamplerInfo.FieldSamplerAsset);
					UJetAsset* JetAsset = Cast<UJetAsset>(FieldSamplerInfo.FieldSamplerAsset);
					JetActor->bEnabled = true;
					JetActor->FieldStrength = JetAsset->FieldStrength;
					JetActor->UpdateApexActor();
					JetActor->UpdatePosition(LocalToWorld);

					Resource.Actor = JetActor;
				}
				break;
				case EFieldSamplerAssetType::EFSAT_GRID:
				{
					FApexTurbulenceActor* TurbActor = new FApexTurbulenceActor(NULL, InPhysScene, SceneType, FieldSamplerInfo.FieldSamplerAsset);
					UGridAsset* GridAsset = Cast<UGridAsset>(FieldSamplerInfo.FieldSamplerAsset);

					TurbActor->bEnabled = true;
					TurbActor->UpdateApexActor();
					TurbActor->UpdatePosition(LocalToWorld);

					FTurbulenceFSInstance* Instance = new FTurbulenceFSInstance();
					Instance->TurbulenceFSActor = static_cast<FApexTurbulenceActor*>(TurbActor);
					Instance->FSType = EFieldSamplerAssetType::EFSAT_GRID;
					Instance->WorldBounds = FBox::BuildAABB(LocalToWorld.GetOrigin(), GridAsset->GridSize3D * (GridAsset->GridScale * 0.5f));
					Instance->LocalBounds = FBox::BuildAABB(FVector::ZeroVector, GridAsset->GridSize3D * (GridAsset->GridScale * 0.5f));
					Instance->VelocityMultiplier = GridAsset->FieldVelocityMultiplier;
					Instance->VelocityWeight = GridAsset->FieldVelocityWeight;
					Instance->bEnabled = true;

					Resource.Actor = TurbActor;
					Resource.Instance = Instance;
				}
				break;
				case EFieldSamplerAssetType::EFSAT_NOISE:
				{
					UNoiseAsset* NoiseAsset = Cast<UNoiseAsset>(FieldSamplerInfo.FieldSamplerAsset);
					if (NoiseAsset->FieldType == EFieldType::FORCE)
					{
						FNoiseFSInstance* Instance = new FNoiseFSInstance();
						Instance->FSType = EFieldSamplerAssetType::EFSAT_NOISE;
						Instance->WorldBounds = FBox::BuildAABB(LocalToWorld.GetOrigin(), NoiseAsset->BoundarySize * (NoiseAsset->BoundaryScale * 0.5f));

						Instance->NoiseSpaceFreq = FVector(1.0f / NoiseAsset->NoiseSpacePeriod.X, 1.0f / NoiseAsset->NoiseSpacePeriod.Y, 1.0f / NoiseAsset->NoiseSpacePeriod.Z);
						Instance->NoiseSpaceFreqOctaveMultiplier = FVector(1.0f / NoiseAsset->NoiseSpacePeriodOctaveMultiplier.X, 1.0f / NoiseAsset->NoiseSpacePeriodOctaveMultiplier.Y, 1.0f / NoiseAsset->NoiseSpacePeriodOctaveMultiplier.Z);
						Instance->NoiseStrength = NoiseAsset->NoiseStrength;
						Instance->NoiseTimeFreq = 1.0f / NoiseAsset->NoiseTimePeriod;
						Instance->NoiseStrengthOctaveMultiplier = NoiseAsset->NoiseStrengthOctaveMultiplier;
						Instance->NoiseTimeFreqOctaveMultiplier = 1.0f / NoiseAsset->NoiseTimePeriodOctaveMultiplier;
						Instance->NoiseOctaves = NoiseAsset->NoiseOctaves;
						Instance->NoiseType = NoiseAsset->NoiseType;
						Instance->NoiseSeed = NoiseAsset->NoiseSeed;
						Instance->bEnabled = true;

						Resource.Instance = Instance;
					}
					else
					{
						FApexNoiseActor* NoiseActor = new FApexNoiseActor(NULL, InPhysScene, SceneType, FieldSamplerInfo.FieldSamplerAsset);
						NoiseActor->bEnabled = true;
						NoiseActor->NoiseStrength = NoiseAsset->NoiseStrength;
						NoiseActor->UpdateApexActor();
						NoiseActor->UpdatePosition(LocalToWorld);

						Resource.Actor = NoiseActor;
					}
				}
				break;
				case EFieldSamplerAssetType::EFSAT_VORTEX:
				{
					FApexVortexActor* VortexActor = new FApexVortexActor(NULL, InPhysScene, SceneType, FieldSamplerInfo.FieldSamplerAsset);
					UVortexAsset* VortexAsset = Cast<UVortexAsset>(FieldSamplerInfo.FieldSamplerAsset);
					VortexActor->bEnabled = true;
					VortexActor->RotationalFieldStrength = VortexAsset->RotationalFieldStrength;
					VortexActor->RadialFieldStrength = VortexAsset->RadialFieldStrength;
					VortexActor->LiftFieldStrength = VortexAsset->LiftFieldStrength;
					VortexActor->UpdateApexActor();
					VortexActor->UpdatePosition(LocalToWorld);

					Resource.Actor = VortexActor;
				}
				break;
				}

				if (FXSystem && Resource.Instance)
				{
					FXSystem->AddFieldSampler(Resource.Instance, LocalToWorld);
				}
			}
			LocalFieldSamplers.Add(Resource);
		}
	}

	void DestroyLocalFieldSampler(FFXSystem* FXSystem)
	{
		for (int32 FieldSamplerIndex = 0; FieldSamplerIndex < LocalFieldSamplers.Num(); ++FieldSamplerIndex)
		{
			if (FXSystem && LocalFieldSamplers[FieldSamplerIndex].Instance)
			{
				FXSystem->RemoveFieldSampler(LocalFieldSamplers[FieldSamplerIndex].Instance);
				LocalFieldSamplers[FieldSamplerIndex].Instance = NULL;
			}

			if (LocalFieldSamplers[FieldSamplerIndex].Actor)
			{
				LocalFieldSamplers[FieldSamplerIndex].Actor->DeferredRelease();
				LocalFieldSamplers[FieldSamplerIndex].Actor = NULL;
			}
		}
		LocalFieldSamplers.Empty();
	}
#endif
	// NVCHANGE_END: JCAO - Field Sampler Module for GPU particle

private:

	/**
	 * Release resources on the rendering thread.
	 */
	void ReleaseRenderResources()
	{
		check( IsInRenderingThread() );
		VertexBuffer.ReleaseResource();
		TileVertexBuffer.ReleaseResource();
		if ( VectorFieldVisualizationVertexFactory )
		{
			VectorFieldVisualizationVertexFactory->ReleaseResource();
		}
	}
};

/*-----------------------------------------------------------------------------
	Dynamic emitter data for GPU sprite particles.
-----------------------------------------------------------------------------*/

/**
 * Per-emitter resources for GPU sprites.
 */
class FGPUSpriteResources : public FRenderResource
{
public:

	/** Emitter uniform buffer used for rendering. */
	FGPUSpriteEmitterUniformBufferRef UniformBuffer;
	/** Emitter simulation resources. */
	FParticleEmitterSimulationResources EmitterSimulationResources;
	/** Texel allocation for the color curve. */
	FTexelAllocation ColorTexelAllocation;
	/** Texel allocation for the misc attributes curve. */
	FTexelAllocation MiscTexelAllocation;
	/** Texel allocation for the simulation attributes curve. */
	FTexelAllocation SimulationAttrTexelAllocation;
	/** Emitter uniform parameters used for rendering. */
	FGPUSpriteEmitterUniformParameters UniformParameters;
	/** Emitter uniform parameters used for simulation. */
	FParticleSimulationParameters SimulationParameters;
	// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
	/** Grid Density Resource */
	FParticleGridDensityResource		GridDensityResource;
	/** Texel allocation for the color curve. */
	FTexelAllocation DensityColorTexelAllocation;
	/** Texel allocation for the size curve. */
	FTexelAllocation DensitySizeTexelAllocation;
	// NVCHANGE_END: JCAO - Grid Density with GPU particles

	/**
	 * Initialize RHI resources.
	 */
	virtual void InitRHI() override
	{
		UniformBuffer = FGPUSpriteEmitterUniformBufferRef::CreateUniformBufferImmediate( UniformParameters, UniformBuffer_MultiFrame );
		EmitterSimulationResources.SimulationUniformBuffer =
			FParticleSimulationBufferRef::CreateUniformBufferImmediate( SimulationParameters, UniformBuffer_MultiFrame );

		// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
		// only init the resource when grid resolution is not zero.
		if (GridDensityResource.GridResolution > 0)
		{
			GridDensityResource.InitRHI();
		}
		// NVCHANGE_END: JCAO - Grid Density with GPU particles
	}

	/**
	 * Release RHI resources.
	 */
	virtual void ReleaseRHI() override
	{
		// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
		GridDensityResource.ReleaseRHI();
		// NVCHANGE_END: JCAO - Grid Density with GPU particles

		UniformBuffer.SafeRelease();
		EmitterSimulationResources.SimulationUniformBuffer.SafeRelease();
	}
};

class FGPUSpriteCollectorResources : public FOneFrameResource
{
public:
	FGPUSpriteVertexFactory VertexFactory;

	~FGPUSpriteCollectorResources()
	{
		VertexFactory.ReleaseResource();
	}
};

/**
 * Dynamic emitter data for Cascade.
 */
struct FGPUSpriteDynamicEmitterData : FDynamicEmitterDataBase
{
	/** FX system. */
	FFXSystem* FXSystem;
	/** Per-emitter resources. */
	FGPUSpriteResources* Resources;
	/** Simulation resources. */
	FParticleSimulationGPU* Simulation;
	/** Bounds for particles in the simulation. */
	FBox SimulationBounds;
	/** The material with which to render sprites. */
	UMaterialInterface* Material;
	/** A list of new particles to inject in to the simulation for this emitter. */
	TArray<FNewParticle> NewParticles;
	/** A list of tiles to clear that were newly allocated for this emitter. */
	TArray<uint32> TilesToClear;
	/** Vector field-to-world transform. */
	FMatrix LocalVectorFieldToWorld;
	/** Vector field scale. */
	float LocalVectorFieldIntensity;
	/** Vector field tightness. */
	float LocalVectorFieldTightness;
	/** Per-frame simulation parameters. */
	FParticlePerFrameSimulationParameters PerFrameSimulationParameters;
	/** Per-emitter parameters that may change*/
	FGPUSpriteEmitterDynamicUniformParameters EmitterDynamicParameters;
	/** How the particles should be sorted, if at all. */
	EParticleSortMode SortMode;
	/** Whether to render particles in local space or world space. */
	bool bUseLocalSpace;	
	/** Tile vector field in x axis? */
	uint32 bLocalVectorFieldTileX : 1;
	/** Tile vector field in y axis? */
	uint32 bLocalVectorFieldTileY : 1;
	/** Tile vector field in z axis? */
	uint32 bLocalVectorFieldTileZ : 1;

	// NVCHANGE_BEGIN: JCAO - Field Sampler Module for GPU particle
#if WITH_APEX_TURBULENCE
	TArray<FMatrix> LocalFieldSamplersToWorld;
#endif
	// NVCHANGE_END: JCAO - Field Sampler Module for GPU particle


	/** Constructor. */
	explicit FGPUSpriteDynamicEmitterData( const UParticleModuleRequired* InRequiredModule )
		: FDynamicEmitterDataBase( InRequiredModule )
		, FXSystem(NULL)
		, Resources(NULL)
		, Simulation(NULL)
		, Material(NULL)
		, SortMode(PSORTMODE_None)
		, bLocalVectorFieldTileX(false)
		, bLocalVectorFieldTileY(false)
		, bLocalVectorFieldTileZ(false)
	{
	}

	bool RendersWithTranslucentMaterial() const
	{
		EBlendMode BlendMode = Material->GetRenderProxy(false)->GetMaterial(FXSystem->GetFeatureLevel())->GetBlendMode();
		return IsTranslucentBlendMode(BlendMode);
	}

	/**
	 * Called to create render thread resources.
	 */
	virtual void UpdateRenderThreadResourcesEmitter(const FParticleSystemSceneProxy* InOwnerProxy) override
	{
		check(Simulation);

		// Update the per-frame simulation parameters with those provided from the game thread.
		Simulation->PerFrameSimulationParameters = PerFrameSimulationParameters;

		// Local vector field parameters.
		Simulation->LocalVectorField.Intensity = LocalVectorFieldIntensity;
		Simulation->LocalVectorField.Tightness = LocalVectorFieldTightness;
		Simulation->LocalVectorField.bTileX = bLocalVectorFieldTileX;
		Simulation->LocalVectorField.bTileY = bLocalVectorFieldTileY;
		Simulation->LocalVectorField.bTileZ = bLocalVectorFieldTileZ;
		if (Simulation->LocalVectorField.Resource)
		{
			Simulation->LocalVectorField.UpdateTransforms(LocalVectorFieldToWorld);
		}

		// NVCHANGE_BEGIN: JCAO - Field Sampler Module for GPU particle
#if WITH_APEX_TURBULENCE
		// Update local field sampler world bound and transform.
		for (int32 FieldSamplerIndex = 0; FieldSamplerIndex < Simulation->LocalFieldSamplers.Num(); ++FieldSamplerIndex)
		{
			if (Simulation->LocalFieldSamplers[FieldSamplerIndex].Instance)
			{
				FVector Extent = Simulation->LocalFieldSamplers[FieldSamplerIndex].Instance->WorldBounds.GetExtent();
				Simulation->LocalFieldSamplers[FieldSamplerIndex].Instance->WorldBounds = FBox::BuildAABB(LocalFieldSamplersToWorld[FieldSamplerIndex].GetOrigin(), Extent);
				Simulation->LocalFieldSamplers[FieldSamplerIndex].Instance->UpdateTransforms(LocalFieldSamplersToWorld[FieldSamplerIndex]);
			}
		}
#endif
		// NVCHANGE_END: JCAO - Field Sampler Module for GPU particle	

		// Update world bounds.
		Simulation->Bounds = SimulationBounds;

		// Transfer ownership of new data.
		if (NewParticles.Num())
		{
			Exchange(Simulation->NewParticles, NewParticles);
		}
		if (TilesToClear.Num())
		{
			Exchange(Simulation->TilesToClear, TilesToClear);
		}

		const bool bTranslucent = RendersWithTranslucentMaterial();

		// If the simulation wants to collide against the depth buffer
		// and we're not rendering with an opaque material put the 
		// simulation in the collision phase.
		if (bTranslucent && Simulation->bWantsCollision && Simulation->CollisionMode == EParticleCollisionMode::SceneDepth)
		{
			Simulation->SimulationPhase = EParticleSimulatePhase::CollisionDepthBuffer;
		}
		else if (Simulation->bWantsCollision && Simulation->CollisionMode == EParticleCollisionMode::DistanceField)
		{
			if (IsParticleCollisionModeSupported(FXSystem->GetShaderPlatform(), PCM_DistanceField))
			{
				Simulation->SimulationPhase = EParticleSimulatePhase::CollisionDistanceField;
			}
			else if (bTranslucent)
			{
				// Fall back to scene depth collision if translucent
				Simulation->SimulationPhase = EParticleSimulatePhase::CollisionDepthBuffer;
			}
			else
			{
				Simulation->SimulationPhase = EParticleSimulatePhase::Main;
			}
		}
	}

	/**
	 * Called to release render thread resources.
	 */
	virtual void ReleaseRenderThreadResources(const FParticleSystemSceneProxy* InOwnerProxy) override
	{		
	}

	virtual void GetDynamicMeshElementsEmitter(const FParticleSystemSceneProxy* Proxy, const FSceneView* View, const FSceneViewFamily& ViewFamily, int32 ViewIndex, FMeshElementCollector& Collector) const override
	{
		auto FeatureLevel = ViewFamily.GetFeatureLevel();

		if (RHISupportsGPUParticles(FeatureLevel))
		{
			SCOPE_CYCLE_COUNTER(STAT_GPUSpritePreRenderTime);

			check(Simulation);

			// Do not render orphaned emitters. This can happen if the emitter
			// instance has been destroyed but we are rendering before the
			// scene proxy has received the update to clear dynamic data.
			if (Simulation->SimulationIndex != INDEX_NONE
				&& Simulation->VertexBuffer.ParticleCount > 0)
			{
				FGPUSpriteEmitterDynamicUniformBufferRef LocalDynamicUniformBuffer;
				// Create view agnostic render data.  Do here rather than in CreateRenderThreadResources because in some cases Render can be called before CreateRenderThreadResources
				{
					// Create per-emitter uniform buffer for dynamic parameters
					LocalDynamicUniformBuffer = FGPUSpriteEmitterDynamicUniformBufferRef::CreateUniformBufferImmediate(EmitterDynamicParameters, UniformBuffer_SingleFrame);
				}

				if (bUseLocalSpace == false)
				{
					Proxy->UpdateWorldSpacePrimitiveUniformBuffer();
				}

				const bool bTranslucent = RendersWithTranslucentMaterial();
				const bool bAllowSorting = FXConsoleVariables::bAllowGPUSorting
					&& FeatureLevel == ERHIFeatureLevel::SM5
					&& bTranslucent;

				// Iterate over views and assign parameters for each.
				FParticleSimulationResources* SimulationResources = FXSystem->GetParticleSimulationResources();
				FGPUSpriteCollectorResources& CollectorResources = Collector.AllocateOneFrameResource<FGPUSpriteCollectorResources>();
				CollectorResources.VertexFactory.SetFeatureLevel(FeatureLevel);
				CollectorResources.VertexFactory.InitResource();
				FGPUSpriteVertexFactory& VertexFactory = CollectorResources.VertexFactory;

				if (bAllowSorting && SortMode == PSORTMODE_DistanceToView)
				{
					// Extensibility TODO: This call to AddSortedGPUSimulation is very awkward. When rendering a frame we need to
					// accumulate all GPU particle emitters that need to be sorted. That is so they can be sorted in one big radix
					// sort for efficiency. Ideally that state is per-scene renderer but the renderer doesn't know anything about particles.
					const int32 SortedBufferOffset = FXSystem->AddSortedGPUSimulation(Simulation, View->ViewMatrices.ViewOrigin);
					check(SimulationResources->SortedVertexBuffer.IsInitialized());
					VertexFactory.SetVertexBuffer(&SimulationResources->SortedVertexBuffer, SortedBufferOffset);
				}
				else
				{
					check(Simulation->VertexBuffer.IsInitialized());
					VertexFactory.SetVertexBuffer(&Simulation->VertexBuffer, 0);
				}

				const int32 ParticleCount = Simulation->VertexBuffer.ParticleCount;
				const bool bIsWireframe = ViewFamily.EngineShowFlags.Wireframe;

				{
					SCOPE_CYCLE_COUNTER(STAT_GPUSpriteRenderingTime);

					FParticleSimulationResources* ParticleSimulationResources = FXSystem->GetParticleSimulationResources();
					FParticleStateTextures& StateTextures = ParticleSimulationResources->GetCurrentStateTextures();
							
					VertexFactory.EmitterUniformBuffer = Resources->UniformBuffer;
					VertexFactory.EmitterDynamicUniformBuffer = LocalDynamicUniformBuffer;
					VertexFactory.PositionTextureRHI = StateTextures.PositionTextureRHI;
					VertexFactory.VelocityTextureRHI = StateTextures.VelocityTextureRHI;
					VertexFactory.AttributesTextureRHI = ParticleSimulationResources->RenderAttributesTexture.TextureRHI;

					// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
					if (GMaxRHIFeatureLevel == ERHIFeatureLevel::SM5 && Simulation->GridDensityResource->IsValid())
					{
						Simulation->ViewMatrices = View->ViewMatrices;
						Simulation->bViewMatricesUpdated = true;
						VertexFactory.DensityTextureRHI = StateTextures.DensityTextureRHI;
					}
					else
					{
						// if grid density isn't enabled, do not pass the density texture to sprite shader.
						VertexFactory.DensityTextureRHI = FTexture2DRHIParamRef();
					}
					// NVCHANGE_END: JCAO - Grid Density with GPU particles

					FMeshBatch& Mesh = Collector.AllocateMesh();
					FMeshBatchElement& BatchElement = Mesh.Elements[0];
					BatchElement.IndexBuffer = &GParticleIndexBuffer;
					BatchElement.NumPrimitives = MAX_PARTICLES_PER_INSTANCE * 2;
					BatchElement.NumInstances = ParticleCount / MAX_PARTICLES_PER_INSTANCE;
					BatchElement.FirstIndex = 0;
					Mesh.VertexFactory = &VertexFactory;
					Mesh.LCI = NULL;
					if ( bUseLocalSpace )
					{
						BatchElement.PrimitiveUniformBufferResource = &Proxy->GetUniformBuffer();
					}
					else
					{
						BatchElement.PrimitiveUniformBufferResource = &Proxy->GetWorldSpacePrimitiveUniformBuffer();
					}
					BatchElement.MinVertexIndex = 0;
					BatchElement.MaxVertexIndex = 3;
					Mesh.ReverseCulling = Proxy->IsLocalToWorldDeterminantNegative();
					Mesh.CastShadow = Proxy->GetCastShadow();
					Mesh.DepthPriorityGroup = (ESceneDepthPriorityGroup)Proxy->GetDepthPriorityGroup(View);
					const bool bUseSelectedMaterial = GIsEditor && (ViewFamily.EngineShowFlags.Selection) ? bSelected : 0;
					Mesh.MaterialRenderProxy = Material->GetRenderProxy(bUseSelectedMaterial);
					Mesh.Type = PT_TriangleList;
					Mesh.bCanApplyViewModeOverrides = true;
					Mesh.bUseWireframeSelectionColoring = Proxy->IsSelected();

					Collector.AddMesh(ViewIndex, Mesh);
				}

				const bool bHaveLocalVectorField = Simulation && Simulation->LocalVectorField.Resource;
				if (bHaveLocalVectorField && ViewFamily.EngineShowFlags.VectorFields)
				{
					// Create a vertex factory for visualization if needed.
					Simulation->CreateVectorFieldVisualizationVertexFactory();
					check(Simulation->VectorFieldVisualizationVertexFactory);
					DrawVectorFieldBounds(Collector.GetPDI(ViewIndex), View, &Simulation->LocalVectorField);
					GetVectorFieldMesh(Simulation->VectorFieldVisualizationVertexFactory, &Simulation->LocalVectorField, ViewIndex, Collector);
				}
			}
		}
	}

	/**
	 * Retrieves the material render proxy with which to render sprites.
	 */
	virtual const FMaterialRenderProxy* GetMaterialRenderProxy(bool bSelected) override
	{
		check( Material );
		return Material->GetRenderProxy( bSelected );
	}

	/**
	 * Emitter replay data. A dummy value is returned as data is stored on the GPU.
	 */
	virtual const FDynamicEmitterReplayDataBase& GetSource() const override
	{
		static FDynamicEmitterReplayDataBase DummyData;
		return DummyData;
	}
};

/*-----------------------------------------------------------------------------
	Particle emitter instance for GPU particles.
-----------------------------------------------------------------------------*/

#if TRACK_TILE_ALLOCATIONS
TMap<FFXSystem*,TSet<class FGPUSpriteParticleEmitterInstance*> > GPUSpriteParticleEmitterInstances;
#endif // #if TRACK_TILE_ALLOCATIONS

/**
 * Particle emitter instance for Cascade.
 */
class FGPUSpriteParticleEmitterInstance : public FParticleEmitterInstance
{
	/** Pointer the the FX system with which the instance is associated. */
	FFXSystem* FXSystem;
	/** Information on how to emit and simulate particles. */
	FGPUSpriteEmitterInfo& EmitterInfo;
	/** GPU simulation resources. */
	FParticleSimulationGPU* Simulation;
	/** The list of tiles active for this emitter. */
	TArray<uint32> AllocatedTiles;
	/** Bit array specifying which tiles are free for spawning new particles. */
	TBitArray<> ActiveTiles;
	/** The time at which each active tile will no longer have active particles. */
	TArray<float> TileTimeOfDeath;
	/** The list of tiles that need to be cleared. */
	TArray<uint32> TilesToClear;
	/** The list of new particles generated this time step. */
	TArray<FNewParticle> NewParticles;
	/** The list of force spawned particles from events */
	TArray<FNewParticle> ForceSpawnedParticles;
	/** The list of force spawned particles from events using Bursts */
	TArray<FNewParticle> ForceBurstSpawnedParticles;
	/** The rotation to apply to the local vector field. */
	FRotator LocalVectorFieldRotation;
	/** The strength of the point attractor. */
	float PointAttractorStrength;
	/** The amount of time by which the GPU needs to simulate particles during its next update. */
	float PendingDeltaSeconds;
	/** Tile to allocate new particles from. */
	int32 TileToAllocateFrom;
	/** How many particles are free in the most recently allocated tile. */
	int32 FreeParticlesInTile;
	/** Random stream for this emitter. */
	FRandomStream RandomStream;
	/** The number of times this emitter should loop. */
	int32 AllowedLoopCount;

	// NVCHANGE_BEGIN: JCAO - Support Force Type Noise
	float PendingTotalSeconds;
	// NVCHANGE_END: JCAO - Support Force Type Noise

	/**
	 * Information used to spawn particles.
	 */
	struct FSpawnInfo
	{
		/** Number of particles to spawn. */
		int32 Count;
		/** Time at which the first particle spawned. */
		float StartTime;
		/** Amount by which to increment time for each subsequent particle. */
		float Increment;

		/** Default constructor. */
		FSpawnInfo()
			: Count(0)
			, StartTime(0.0f)
			, Increment(0.0f)
		{
		}
	};

public:

	/** Initialization constructor. */
	FGPUSpriteParticleEmitterInstance(FFXSystem* InFXSystem, FGPUSpriteEmitterInfo& InEmitterInfo)
		: FXSystem(InFXSystem)
		, EmitterInfo(InEmitterInfo)
		, LocalVectorFieldRotation(FRotator::ZeroRotator)
		, PointAttractorStrength(0.0f)
		, PendingDeltaSeconds(0.0f)
		, TileToAllocateFrom(INDEX_NONE)
		, FreeParticlesInTile(0)
		, AllowedLoopCount(0)
		// NVCHANGE_BEGIN: JCAO - Support Force Type Noise
		, PendingTotalSeconds(0.0f)
		// NVCHANGE_END: JCAO - Support Force Type Noise
	{
		Simulation = new FParticleSimulationGPU();
		if (EmitterInfo.LocalVectorField.Field)
		{
			EmitterInfo.LocalVectorField.Field->InitInstance(&Simulation->LocalVectorField, /*bPreviewInstance=*/ false);
		}
		Simulation->bWantsCollision = InEmitterInfo.bEnableCollision;
		Simulation->CollisionMode = InEmitterInfo.CollisionMode;

#if TRACK_TILE_ALLOCATIONS
		TSet<class FGPUSpriteParticleEmitterInstance*>* EmitterSet = GPUSpriteParticleEmitterInstances.Find(FXSystem);
		if (!EmitterSet)
		{
			EmitterSet = &GPUSpriteParticleEmitterInstances.Add(FXSystem,TSet<class FGPUSpriteParticleEmitterInstance*>());
		}
		EmitterSet->Add(this);
#endif // #if TRACK_TILE_ALLOCATIONS
	}

	/** Destructor. */
	virtual ~FGPUSpriteParticleEmitterInstance()
	{
		ReleaseSimulationResources();
		// NVCHANGE_BEGIN: JCAO - Field Sampler Module for GPU particle
#if WITH_APEX_TURBULENCE
		// Destroy Field Sampler after Simulation was remove from Render thread.
		Simulation->DestroyLocalFieldSampler(FXSystem);
#endif
		// NVCHANGE_END: JCAO - Field Sampler Module for GPU particle
		Simulation->Destroy();
		Simulation = NULL;

#if TRACK_TILE_ALLOCATIONS
		TSet<class FGPUSpriteParticleEmitterInstance*>* EmitterSet = GPUSpriteParticleEmitterInstances.Find(FXSystem);
		check(EmitterSet);
		EmitterSet->Remove(this);
		if (EmitterSet->Num() == 0)
		{
			GPUSpriteParticleEmitterInstances.Remove(FXSystem);
		}
#endif // #if TRACK_TILE_ALLOCATIONS
	}

	/**
	 * Returns the number of tiles allocated to this emitter.
	 */
	int32 GetAllocatedTileCount() const
	{
		return AllocatedTiles.Num();
	}

	/**
	 *	Checks some common values for GetDynamicData validity
	 *
	 *	@return	bool		true if GetDynamicData should continue, false if it should return NULL
	 */
	virtual bool IsDynamicDataRequired(UParticleLODLevel* CurrentLODLevel) override
	{
		bool bShouldRender = (ActiveParticles >= 0 || TilesToClear.Num() || NewParticles.Num());
		bool bCanRender = (FXSystem != NULL) && (Component != NULL) && (Component->FXSystem == FXSystem);
		return bShouldRender && bCanRender;
	}

	/**
	 *	Retrieves the dynamic data for the emitter
	 */
	virtual FDynamicEmitterDataBase* GetDynamicData(bool bSelected) override
	{
		check(Component);
		check(SpriteTemplate);
		check(FXSystem);
		check(Component->FXSystem == FXSystem);

		// Grab the current LOD level
		UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
		if (LODLevel->bEnabled == false)
		{
			return NULL;
		}

		const bool bLocalSpace = EmitterInfo.RequiredModule->bUseLocalSpace;
		const FMatrix ComponentToWorld = (bLocalSpace || EmitterInfo.LocalVectorField.bIgnoreComponentTransform) ? FMatrix::Identity : Component->ComponentToWorld.ToMatrixWithScale();
		const FRotationMatrix VectorFieldTransform(LocalVectorFieldRotation);
		const FMatrix VectorFieldToWorld = VectorFieldTransform * EmitterInfo.LocalVectorField.Transform.ToMatrixWithScale() * ComponentToWorld;
		FGPUSpriteDynamicEmitterData* DynamicData = new FGPUSpriteDynamicEmitterData(EmitterInfo.RequiredModule);
		DynamicData->FXSystem = FXSystem;
		DynamicData->Resources = EmitterInfo.Resources;
		DynamicData->Material = GetCurrentMaterial();
		DynamicData->Simulation = Simulation;
		DynamicData->SimulationBounds = Component->Bounds.GetBox();
		DynamicData->LocalVectorFieldToWorld = VectorFieldToWorld;
		DynamicData->LocalVectorFieldIntensity = EmitterInfo.LocalVectorField.Intensity;
		DynamicData->LocalVectorFieldTightness = EmitterInfo.LocalVectorField.Tightness;	
		DynamicData->bLocalVectorFieldTileX = EmitterInfo.LocalVectorField.bTileX;	
		DynamicData->bLocalVectorFieldTileY = EmitterInfo.LocalVectorField.bTileY;	
		DynamicData->bLocalVectorFieldTileZ = EmitterInfo.LocalVectorField.bTileZ;	
		DynamicData->SortMode = EmitterInfo.RequiredModule->SortMode;
		DynamicData->bSelected = bSelected;
		DynamicData->bUseLocalSpace = EmitterInfo.RequiredModule->bUseLocalSpace;

		// NVCHANGE_BEGIN: JCAO - Field Sampler Module for GPU particle
#if WITH_APEX_TURBULENCE
		// Update world matrix for local field sampler
		for (int32 FieldSamplerIndex = 0; FieldSamplerIndex < EmitterInfo.LocalFieldSamplers.Num(); ++FieldSamplerIndex)
		{
			const FMatrix LocalToWorld = EmitterInfo.LocalFieldSamplers[FieldSamplerIndex].Transform.ToMatrixWithScale() * Component->ComponentToWorld.ToMatrixWithScale();
			DynamicData->LocalFieldSamplersToWorld.Add(LocalToWorld);

			if (Simulation->LocalFieldSamplers[FieldSamplerIndex].Actor)
			{
				Simulation->LocalFieldSamplers[FieldSamplerIndex].Actor->UpdatePosition(LocalToWorld);
			}
		}
#endif
		// NVCHANGE_END: JCAO - Field Sampler Module for GPU particle

		// Account for LocalToWorld scaling
		FVector ComponentScale = Component->ComponentToWorld.GetScale3D();
		// Figure out if we need to replicate the X channel of size to Y.
		const bool bSquare = (EmitterInfo.ScreenAlignment == PSA_Square)
			|| (EmitterInfo.ScreenAlignment == PSA_FacingCameraPosition);

		DynamicData->EmitterDynamicParameters.LocalToWorldScale.X = ComponentScale.X;
		DynamicData->EmitterDynamicParameters.LocalToWorldScale.Y = (bSquare) ? ComponentScale.X : ComponentScale.Y;

		// Setup axis lock parameters if required.
		const FMatrix& LocalToWorld = ComponentToWorld;
		const EParticleAxisLock LockAxisFlag = (EParticleAxisLock)EmitterInfo.LockAxisFlag;
		DynamicData->EmitterDynamicParameters.AxisLockRight = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
		DynamicData->EmitterDynamicParameters.AxisLockUp = FVector4(0.0f, 0.0f, 0.0f, 0.0f);

		if(LockAxisFlag != EPAL_NONE)
		{
			FVector AxisLockUp, AxisLockRight;
			const FMatrix& AxisLocalToWorld = bLocalSpace ? LocalToWorld : FMatrix::Identity;
			extern void ComputeLockedAxes(EParticleAxisLock, const FMatrix&, FVector&, FVector&);
			ComputeLockedAxes( LockAxisFlag, AxisLocalToWorld, AxisLockUp, AxisLockRight );

			DynamicData->EmitterDynamicParameters.AxisLockRight = AxisLockRight;
			DynamicData->EmitterDynamicParameters.AxisLockRight.W = 1.0f;
			DynamicData->EmitterDynamicParameters.AxisLockUp = AxisLockUp;
			DynamicData->EmitterDynamicParameters.AxisLockUp.W = 1.0f;
		}

		
		// Setup dynamic color parameter. Only set when using particle parameter distributions.
		FVector4 ColorOverLife(1.0f, 1.0f, 1.0f, 1.0f);
		FVector4 ColorScaleOverLife(1.0f, 1.0f, 1.0f, 1.0f);
		if( EmitterInfo.DynamicColorScale.Distribution )
		{
			ColorScaleOverLife = EmitterInfo.DynamicColorScale.GetValue(0.0f,Component);
		}
		if( EmitterInfo.DynamicAlphaScale.Distribution )
		{
			ColorScaleOverLife.W = EmitterInfo.DynamicAlphaScale.GetValue(0.0f,Component);
		}

		if( EmitterInfo.DynamicColor.Distribution )
		{
			ColorOverLife = EmitterInfo.DynamicColor.GetValue(0.0f,Component);
		}
		if( EmitterInfo.DynamicAlpha.Distribution )
		{
			ColorOverLife.W = EmitterInfo.DynamicAlpha.GetValue(0.0f,Component);
		}
		DynamicData->EmitterDynamicParameters.DynamicColor = ColorOverLife * ColorScaleOverLife;

		const bool bSimulateGPUParticles = 
			FXConsoleVariables::bFreezeGPUSimulation == false &&
			FXConsoleVariables::bFreezeParticleSimulation == false &&
			RHISupportsGPUParticles(FXSystem->GetFeatureLevel());

		if (bSimulateGPUParticles)
		{
			FVector PointAttractorPosition = ComponentToWorld.TransformPosition(EmitterInfo.PointAttractorPosition);
			DynamicData->PerFrameSimulationParameters.PointAttractor = FVector4(PointAttractorPosition, EmitterInfo.PointAttractorRadiusSq);
			DynamicData->PerFrameSimulationParameters.PositionOffsetAndAttractorStrength = FVector4(PositionOffsetThisTick, PointAttractorStrength);
			DynamicData->PerFrameSimulationParameters.LocalToWorldScale = DynamicData->EmitterDynamicParameters.LocalToWorldScale;
			DynamicData->PerFrameSimulationParameters.DeltaSeconds = PendingDeltaSeconds;
			// NVCHANGE_BEGIN: JCAO - Support Force Type Noise
			DynamicData->PerFrameSimulationParameters.TotalSeconds = PendingTotalSeconds;
			// NVCHANGE_END: JCAO - Support Force Type Noise
			Exchange(DynamicData->TilesToClear, TilesToClear);
			Exchange(DynamicData->NewParticles, NewParticles);
		}

		NewParticles.Reset();
		PendingDeltaSeconds = 0.0f;
		PositionOffsetThisTick = FVector::ZeroVector;

		if (Simulation->bDirty_GameThread)
		{
			// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
			Simulation->InitResources(AllocatedTiles, &EmitterInfo.Resources->EmitterSimulationResources, &EmitterInfo.Resources->GridDensityResource);
			// NVCHANGE_END: JCAO - Grid Density with GPU particles
		}
		check(!Simulation->bReleased_GameThread);
		check(!Simulation->bDestroyed_GameThread);

		return DynamicData;
	}

	/**
	 * Initializes parameters for this emitter instance.
	 */
	virtual void InitParameters(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent, bool bClearResources) override
	{
		FParticleEmitterInstance::InitParameters( InTemplate, InComponent, bClearResources );
		SetupEmitterDuration();

		// NVCHANGE_BEGIN: JCAO - Field Sampler Module for GPU particle
#if WITH_APEX_TURBULENCE
		FPhysScene* PhysScene = InComponent->GetWorld()->GetPhysicsScene();
		bool bIsCascade = (Component->GetClass()->GetName() == TEXT("CascadeParticleSystemComponent"));
		Simulation->InitLocalFieldSamplers(PhysScene, bIsCascade ? PST_Sync : PST_Async, Component, FXSystem, EmitterInfo.LocalFieldSamplers);
#endif
		// NVCHANGE_END: JCAO - Field Sampler Module for GPU particle
	}

	/**
	 * Initializes the emitter.
	 */
	virtual void Init() override
	{
		FParticleEmitterInstance::Init();

		if (EmitterInfo.RequiredModule)
		{
			MaxActiveParticles = 0;
			ActiveParticles = 0;
			AllowedLoopCount = EmitterInfo.RequiredModule->EmitterLoops;
		}
		else
		{
			MaxActiveParticles = 0;
			ActiveParticles = 0;
			AllowedLoopCount = 0;
		}

		check(AllocatedTiles.Num() == TileTimeOfDeath.Num());
		FreeParticlesInTile = 0;

		RandomStream.Initialize( FMath::Rand() );

		FParticleSimulationResources* ParticleSimulationResources = FXSystem->GetParticleSimulationResources();
		const int32 MinTileCount = GetMinTileCount();
		int32 NumAllocated = 0;
		while (AllocatedTiles.Num() < MinTileCount)
		{
			uint32 TileIndex = ParticleSimulationResources->AllocateTile();
			if ( TileIndex != INDEX_NONE )
			{
				AllocatedTiles.Add( TileIndex );
				TileTimeOfDeath.Add( 0.0f );
				NumAllocated++;
			}
			else
			{
				break;
			}
		}
		
#if TRACK_TILE_ALLOCATIONS
		UE_LOG(LogParticles,VeryVerbose,
			TEXT("%s|%s|0x%016x [Init] %d tiles"),
			*Component->GetName(),*Component->Template->GetName(),(PTRINT)this, AllocatedTiles.Num());
#endif // #if TRACK_TILE_ALLOCATIONS

		bool bClearExistingParticles = false;
		UParticleLODLevel* LODLevel = SpriteTemplate->LODLevels[0];
		if (LODLevel)
		{
			UParticleModuleTypeDataGpu* TypeDataModule = CastChecked<UParticleModuleTypeDataGpu>(LODLevel->TypeDataModule);
			bClearExistingParticles = TypeDataModule->bClearExistingParticlesOnInit;
		}

		if (bClearExistingParticles || ActiveTiles.Num() != AllocatedTiles.Num())
		{
			ActiveTiles.Init(false, AllocatedTiles.Num());
			ClearAllocatedTiles();
		}

		Simulation->bDirty_GameThread = true;
		FXSystem->AddGPUSimulation(Simulation);

		CurrentMaterial = EmitterInfo.RequiredModule ? EmitterInfo.RequiredModule->Material : UMaterial::GetDefaultMaterial(MD_Surface);

		InitLocalVectorField();
	}

	/**
	 * Simulates the emitter forward by the specified amount of time.
	 */
	virtual void Tick(float DeltaSeconds, bool bSuppressSpawning) override
	{
		SCOPE_CYCLE_COUNTER(STAT_GPUSpriteTickTime);

		check(AllocatedTiles.Num() == TileTimeOfDeath.Num());

		if (FXConsoleVariables::bFreezeGPUSimulation ||
			FXConsoleVariables::bFreezeParticleSimulation ||
			!RHISupportsGPUParticles(FXSystem->GetFeatureLevel()))
		{
			return;
		}

		// Grab the current LOD level
		UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();

		// Handle EmitterTime setup, looping, etc.
		float EmitterDelay = Tick_EmitterTimeSetup( DeltaSeconds, LODLevel );

		// If the emitter is warming up but any particle spawned now will die
		// anyway, suppress spawning.
		if (Component && Component->bWarmingUp &&
			Component->WarmupTime - SecondsSinceCreation > EmitterInfo.MaxLifetime)
		{
			bSuppressSpawning = true;
		}

		// Mark any tiles with all dead particles as free.
		int32 ActiveTileCount = MarkTilesInactive();

		// Update modules
		Tick_ModuleUpdate(DeltaSeconds, LODLevel);

		// NVCHANGE_BEGIN: JCAO - Field Sampler Module for GPU particle
#if WITH_APEX_TURBULENCE
		Tick_LocalFieldSampler(DeltaSeconds);
#endif
		// NVCHANGE_END: JCAO - Field Sampler Module for GPU particle

		// Spawn particles.
		bool bRefreshTiles = false;
		const bool bPreventSpawning = bHaltSpawning || bSuppressSpawning;
		const bool bValidEmitterTime = (EmitterTime >= 0.0f);
		const bool bValidLoop = AllowedLoopCount == 0 || LoopCount < AllowedLoopCount;
		if (!bPreventSpawning && bValidEmitterTime && bValidLoop)
		{
			SCOPE_CYCLE_COUNTER(STAT_GPUSpriteSpawnTime);

			// Determine burst count.
			FSpawnInfo BurstInfo;
			int32 LeftoverBurst = 0;
			{
				float BurstDeltaTime = DeltaSeconds;
				GetCurrentBurstRateOffset(BurstDeltaTime, BurstInfo.Count);

				BurstInfo.Count += ForceBurstSpawnedParticles.Num();

				if (BurstInfo.Count > FXConsoleVariables::MaxGPUParticlesSpawnedPerFrame)
				{
					LeftoverBurst = BurstInfo.Count - FXConsoleVariables::MaxGPUParticlesSpawnedPerFrame;
					BurstInfo.Count = FXConsoleVariables::MaxGPUParticlesSpawnedPerFrame;
				}
			}



			int32 FirstBurstParticleIndex = NewParticles.Num();
			BurstInfo.Count = AllocateTilesForParticles(NewParticles, BurstInfo.Count, ActiveTileCount);

			// Determine spawn count based on rate.
			FSpawnInfo SpawnInfo = GetNumParticlesToSpawn(DeltaSeconds);
			SpawnInfo.Count += ForceSpawnedParticles.Num();

			int32 FirstSpawnParticleIndex = NewParticles.Num();
			SpawnInfo.Count = AllocateTilesForParticles(NewParticles, SpawnInfo.Count, ActiveTileCount);
			SpawnFraction += LeftoverBurst;

			if (BurstInfo.Count > 0)
			{
				// Spawn burst particles.
				BuildNewParticles(NewParticles.GetData() + FirstBurstParticleIndex, BurstInfo, ForceBurstSpawnedParticles);
			}

			if (SpawnInfo.Count > 0)
			{
				// Spawn normal particles.
				BuildNewParticles(NewParticles.GetData() + FirstSpawnParticleIndex, SpawnInfo, ForceSpawnedParticles);
			}

			ForceBurstSpawnedParticles.Empty();
			ForceSpawnedParticles.Empty();

			int32 NewParticleCount = BurstInfo.Count + SpawnInfo.Count;
			INC_DWORD_STAT_BY(STAT_GPUSpritesSpawned, NewParticleCount);
#if STATS
			if (NewParticleCount > FXConsoleVariables::GPUSpawnWarningThreshold)
			{
				UE_LOG(LogParticles,Warning,TEXT("Spawning %d GPU particles in one frame[%d]: %s/%s"),
					NewParticleCount,
					GFrameNumber,
					*SpriteTemplate->GetOuter()->GetName(),
					*SpriteTemplate->EmitterName.ToString()
					);

			}
#endif

			if (Component && Component->bWarmingUp)
			{
				SimulateWarmupParticles(
					NewParticles.GetData() + (NewParticles.Num() - NewParticleCount),
					NewParticleCount,
					Component->WarmupTime - SecondsSinceCreation );
			}
		}

		// Free any tiles that we no longer need.
		FreeInactiveTiles();

		// Update current material.
		if (EmitterInfo.RequiredModule->Material)
		{
			CurrentMaterial = EmitterInfo.RequiredModule->Material;
		}

		// Update the local vector field.
		TickLocalVectorField(DeltaSeconds);

		// Look up the strength of the point attractor.
		EmitterInfo.PointAttractorStrength.GetValue(EmitterTime, &PointAttractorStrength);

		// Store the amount of time by which the GPU needs to update the simulation.
		PendingDeltaSeconds = DeltaSeconds;
		// NVCHANGE_BEGIN: JCAO - Support Force Type Noise
		PendingTotalSeconds += DeltaSeconds;
		// NVCHANGE_END: JCAO - Support Force Type Noise

		// Store the number of active particles.
		ActiveParticles = ActiveTileCount * GParticlesPerTile;
		INC_DWORD_STAT_BY(STAT_GPUSpriteParticles, ActiveParticles);

		// 'Reset' the emitter time so that the delay functions correctly
		EmitterTime += EmitterDelay;

		// Update the bounding box.
		UpdateBoundingBox(DeltaSeconds);

		// Final update for modules.
		Tick_ModuleFinalUpdate(DeltaSeconds, LODLevel);

		// Queue an update to the GPU simulation if needed.
		if (Simulation->bDirty_GameThread)
		{
			// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
			Simulation->InitResources(AllocatedTiles, &EmitterInfo.Resources->EmitterSimulationResources, &EmitterInfo.Resources->GridDensityResource);
			// NVCHANGE_END: JCAO - Grid Density with GPU particles
		}

		check(AllocatedTiles.Num() == TileTimeOfDeath.Num());
	}

	/**
	 * Clears all active particle tiles.
	 */
	void ClearAllocatedTiles()
	{
		TilesToClear.Reset();
		TilesToClear = AllocatedTiles;
		TileToAllocateFrom = INDEX_NONE;
		FreeParticlesInTile = 0;
		ActiveTiles.Init(false,ActiveTiles.Num());
	}

	/**
	 *	Force kill all particles in the emitter.
	 *	@param	bFireEvents		If true, fire events for the particles being killed.
	 */
	virtual void KillParticlesForced(bool bFireEvents) override
	{
		// Clear all active tiles. This will effectively kill all particles.
		ClearAllocatedTiles();
	}

	/**
	 *	Called when the particle system is deactivating...
	 */
	virtual void OnDeactivateSystem() override
	{
	}

	virtual void Rewind() override
	{
		FParticleEmitterInstance::Rewind();
		InitLocalVectorField();
	}

	/**
	 * Returns true if the emitter has completed.
	 */
	virtual bool HasCompleted() override
	{
		if ( AllowedLoopCount == 0 || LoopCount < AllowedLoopCount )
		{
			return false;
		}
		return ActiveParticles == 0;
	}

	/**
	 * Force the bounding box to be updated.
	 *		WARNING: This is an expensive operation for GPU particles. It
	 *		requires syncing with the GPU to read back the emitter's bounds.
	 *		This function should NEVER be called at runtime!
	 */
	virtual void ForceUpdateBoundingBox() override
	{
		if ( !GIsEditor )
		{
			UE_LOG(LogParticles, Warning, TEXT("ForceUpdateBoundingBox called on a GPU sprite emitter outside of the Editor!") );
			return;
		}

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FComputeGPUSpriteBoundsCommand,
			FGPUSpriteParticleEmitterInstance*, EmitterInstance, this,
		{
			
			EmitterInstance->ParticleBoundingBox = ComputeParticleBounds(
				RHICmdList,
				EmitterInstance->Simulation->VertexBuffer.VertexBufferSRV,
				EmitterInstance->FXSystem->GetParticleSimulationResources()->GetCurrentStateTextures().PositionTextureRHI,
				EmitterInstance->Simulation->VertexBuffer.ParticleCount
				);
		});
		FlushRenderingCommands();

		// Take the size of sprites in to account.
		const float MaxSizeX = EmitterInfo.Resources->UniformParameters.MiscScale.X + EmitterInfo.Resources->UniformParameters.MiscBias.X;
		const float MaxSizeY = EmitterInfo.Resources->UniformParameters.MiscScale.Y + EmitterInfo.Resources->UniformParameters.MiscBias.Y;
		const float MaxSize = FMath::Max<float>( MaxSizeX, MaxSizeY );
		ParticleBoundingBox = ParticleBoundingBox.ExpandBy( MaxSize );
	}

private:

	/**
	 * Mark tiles as inactive if all particles in them have died.
	 */
	int32 MarkTilesInactive()
	{
		int32 ActiveTileCount = 0;
		for (TBitArray<>::FConstIterator BitIt(ActiveTiles); BitIt; ++BitIt)
		{
			const int32 BitIndex = BitIt.GetIndex();
			if (TileTimeOfDeath[BitIndex] <= SecondsSinceCreation)
			{
				ActiveTiles.AccessCorrespondingBit(BitIt) = false;
				if ( TileToAllocateFrom == BitIndex )
				{
					TileToAllocateFrom = INDEX_NONE;
					FreeParticlesInTile = 0;
				}
			}
			else
			{
				ActiveTileCount++;
			}
		}
		return ActiveTileCount;
	}

	/**
	 * Initialize the local vector field.
	 */
	void InitLocalVectorField()
	{
		LocalVectorFieldRotation = FMath::Lerp(
			EmitterInfo.LocalVectorField.MinInitialRotation,
			EmitterInfo.LocalVectorField.MaxInitialRotation,
			RandomStream.GetFraction() );

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FResetVectorFieldCommand,
			FParticleSimulationGPU*,Simulation,Simulation,
		{
			if (Simulation && Simulation->LocalVectorField.Resource)
			{
				Simulation->LocalVectorField.Resource->ResetVectorField();
			}
		});
	}

	/**
	 * Computes the minimum number of tiles that should be allocated for this emitter.
	 */
	int32 GetMinTileCount() const
	{
		if (AllowedLoopCount == 0)
		{
			const int32 EstMaxTiles = (EmitterInfo.MaxParticleCount + GParticlesPerTile - 1) / GParticlesPerTile;
			const int32 SlackTiles = FMath::CeilToInt(FXConsoleVariables::ParticleSlackGPU * (float)EstMaxTiles);
			return FMath::Min<int32>(EstMaxTiles + SlackTiles, FXConsoleVariables::MaxParticleTilePreAllocation);
		}
		return 0;
	}

	/**
	 * Release any inactive tiles.
	 * @returns the number of tiles released.
	 */
	int32 FreeInactiveTiles()
	{
		const int32 MinTileCount = GetMinTileCount();
		int32 TilesToFree = 0;
		TBitArray<>::FConstReverseIterator BitIter(ActiveTiles);
		while (BitIter && BitIter.GetIndex() >= MinTileCount)
		{
			if (BitIter.GetValue())
			{
				break;
			}
			++TilesToFree;
			++BitIter;
		}
		if (TilesToFree > 0)
		{
			FParticleSimulationResources* SimulationResources = FXSystem->GetParticleSimulationResources();
			const int32 FirstTileIndex = AllocatedTiles.Num() - TilesToFree;
			const int32 LastTileIndex = FirstTileIndex + TilesToFree;
			for (int32 TileIndex = FirstTileIndex; TileIndex < LastTileIndex; ++TileIndex)
			{
				SimulationResources->FreeTile(AllocatedTiles[TileIndex]);
			}
			ActiveTiles.RemoveAt(FirstTileIndex, TilesToFree);
			AllocatedTiles.RemoveAt(FirstTileIndex, TilesToFree);
			TileTimeOfDeath.RemoveAt(FirstTileIndex, TilesToFree);
			Simulation->bDirty_GameThread = true;
		}
		return TilesToFree;
	}

	/**
	 * Releases resources allocated for GPU simulation.
	 */
	void ReleaseSimulationResources()
	{
		if (FXSystem)
		{
			FXSystem->RemoveGPUSimulation( Simulation );

			// The check for GIsRequestingExit is done because at shut down UWorld can be destroyed before particle emitters(?)
			if (!GIsRequestingExit)
			{
				FParticleSimulationResources* ParticleSimulationResources = FXSystem->GetParticleSimulationResources();
				const int32 TileCount = AllocatedTiles.Num();
				for ( int32 ActiveTileIndex = 0; ActiveTileIndex < TileCount; ++ActiveTileIndex )
				{
					const uint32 TileIndex = AllocatedTiles[ActiveTileIndex];
					ParticleSimulationResources->FreeTile( TileIndex );
				}
				AllocatedTiles.Reset();
#if TRACK_TILE_ALLOCATIONS
				UE_LOG(LogParticles,VeryVerbose,
					TEXT("%s|%s|0x%016p [ReleaseSimulationResources] %d tiles"),
					*Component->GetName(),*Component->Template->GetName(),(PTRINT)this, AllocatedTiles.Num());
#endif // #if TRACK_TILE_ALLOCATIONS
			}
		}
		else if (!GIsRequestingExit)
		{
			UE_LOG(LogParticles,Warning,
				TEXT("%s|%s|0x%016p [ReleaseSimulationResources] LEAKING %d tiles FXSystem=0x%016x"),
				*Component->GetName(),*Component->Template->GetName(),(PTRINT)this, AllocatedTiles.Num(), (PTRINT)FXSystem);
		}


		ActiveTiles.Reset();
		AllocatedTiles.Reset();
		TileTimeOfDeath.Reset();
		TilesToClear.Reset();
		
		TileToAllocateFrom = INDEX_NONE;
		FreeParticlesInTile = 0;

		Simulation->BeginReleaseResources();
	}

	/**
	 * Allocates space in a particle tile for all new particles.
	 * @param NewParticles - Array in which to store new particles.
	 * @param NumNewParticles - The number of new particles that need an allocation.
	 * @param ActiveTileCount - Number of active tiles, incremented each time a new tile is allocated.
	 * @returns the number of particles which were successfully allocated.
	 */
	int32 AllocateTilesForParticles(TArray<FNewParticle>& InNewParticles, int32 NumNewParticles, int32& ActiveTileCount)
	{
		// Need to allocate space in tiles for all new particles.
		FParticleSimulationResources* SimulationResources = FXSystem->GetParticleSimulationResources();
		uint32 TileIndex = (AllocatedTiles.IsValidIndex(TileToAllocateFrom)) ? AllocatedTiles[TileToAllocateFrom] : INDEX_NONE;
		FVector2D TileOffset(
			FMath::Fractional((float)TileIndex / (float)GParticleSimulationTileCountX),
			FMath::Fractional(FMath::TruncToFloat((float)TileIndex / (float)GParticleSimulationTileCountX) / (float)GParticleSimulationTileCountY)
			);

		for (int32 ParticleIndex = 0; ParticleIndex < NumNewParticles; ++ParticleIndex)
		{
			if (FreeParticlesInTile <= 0)
			{
				// Start adding particles to the first inactive tile.
				if (ActiveTileCount < AllocatedTiles.Num())
				{
					TileToAllocateFrom = ActiveTiles.FindAndSetFirstZeroBit();
				}
				else
				{
					uint32 NewTile = SimulationResources->AllocateTile();
					if (NewTile == INDEX_NONE)
					{
						// Out of particle tiles.
						UE_LOG(LogParticles,Warning,
							TEXT("Failed to allocate tiles for %s! %d new particles truncated to %d."),
							*Component->Template->GetName(), NumNewParticles, ParticleIndex);
						return ParticleIndex;
					}

					TileToAllocateFrom = AllocatedTiles.Add(NewTile);
					TileTimeOfDeath.Add(0.0f);
					TilesToClear.Add(NewTile);
					ActiveTiles.Add(true);
					Simulation->bDirty_GameThread = true;
				}

				ActiveTileCount++;
				TileIndex = AllocatedTiles[TileToAllocateFrom];
				TileOffset.X = FMath::Fractional((float)TileIndex / (float)GParticleSimulationTileCountX);
				TileOffset.Y = FMath::Fractional(FMath::TruncToFloat((float)TileIndex / (float)GParticleSimulationTileCountX) / (float)GParticleSimulationTileCountY);
				FreeParticlesInTile = GParticlesPerTile;
			}
			FNewParticle& Particle = *new(InNewParticles) FNewParticle();
			const int32 SubTileIndex = GParticlesPerTile - FreeParticlesInTile;
			const int32 SubTileX = SubTileIndex % GParticleSimulationTileSize;
			const int32 SubTileY = SubTileIndex / GParticleSimulationTileSize;
			Particle.Offset.X = TileOffset.X + ((float)SubTileX / (float)GParticleSimulationTextureSizeX);
			Particle.Offset.Y = TileOffset.Y + ((float)SubTileY / (float)GParticleSimulationTextureSizeY);
			Particle.ResilienceAndTileIndex.AllocatedTileIndex = TileToAllocateFrom;
			FreeParticlesInTile--;
		}

		return NumNewParticles;
	}

	/**
	 * Computes how many particles should be spawned this frame. Does not account for bursts.
	 * @param DeltaSeconds - The amount of time for which to spawn.
	 */
	FSpawnInfo GetNumParticlesToSpawn(float DeltaSeconds)
	{
		UParticleModuleRequired* RequiredModule = EmitterInfo.RequiredModule;
		UParticleModuleSpawn* SpawnModule = EmitterInfo.SpawnModule;

		// Determine spawn rate.
		check(SpawnModule && RequiredModule);
		const float RateScale = CurrentLODLevel->SpawnModule->RateScale.GetValue(EmitterTime, Component) * CurrentLODLevel->SpawnModule->GetGlobalRateScale();
		float SpawnRate = CurrentLODLevel->SpawnModule->Rate.GetValue(EmitterTime, Component) * RateScale;
		SpawnRate = FMath::Max<float>(0.0f, SpawnRate);

		if (EmitterInfo.SpawnPerUnitModule)
		{
			int32 Number = 0;
			float Rate = 0.0f;
			if (EmitterInfo.SpawnPerUnitModule->GetSpawnAmount(this, 0, 0.0f, DeltaSeconds, Number, Rate) == false)
			{
				SpawnRate = Rate;
			}
			else
			{
				SpawnRate += Rate;
			}
		}

		// Determine how many to spawn.
		FSpawnInfo Info;
		float AccumSpawnCount = SpawnFraction + SpawnRate * DeltaSeconds;
		Info.Count = FMath::Min(FMath::TruncToInt(AccumSpawnCount), FXConsoleVariables::MaxGPUParticlesSpawnedPerFrame);
		Info.Increment = (SpawnRate > 0.0f) ? (1.f / SpawnRate) : 0.0f;
		Info.StartTime = DeltaSeconds + SpawnFraction * Info.Increment - Info.Increment;
		SpawnFraction = AccumSpawnCount - Info.Count;

		return Info;
	}

	/**
	 * Perform a simple simulation for particles during the warmup period. This
	 * Simulation only takes in to account constant acceleration, initial
	 * velocity, and initial position.
	 * @param InNewParticles - The first new particle to simulate.
	 * @param ParticleCount - The number of particles to simulate.
	 * @param WarmupTime - The amount of warmup time by which to simulate.
	 */
	void SimulateWarmupParticles(
		FNewParticle* InNewParticles,
		int32 ParticleCount,
		float WarmupTime )
	{
		const FVector Acceleration = EmitterInfo.ConstantAcceleration;
		for (int32 ParticleIndex = 0; ParticleIndex < ParticleCount; ++ParticleIndex)
		{
			FNewParticle* Particle = InNewParticles + ParticleIndex;
			Particle->Position += (Particle->Velocity + 0.5f * Acceleration * WarmupTime) * WarmupTime;
			Particle->Velocity += Acceleration * WarmupTime;
			Particle->RelativeTime += Particle->TimeScale * WarmupTime;
		}
	}

	/**
	 * Builds new particles to be injected in to the GPU simulation.
	 * @param OutNewParticles - Array in which to store new particles.
	 * @param SpawnCount - The number of particles to spawn.
	 * @param SpawnTime - The time at which to begin spawning particles.
	 * @param Increment - The amount by which to increment time for each particle spawned.
	 */
	void BuildNewParticles(FNewParticle* InNewParticles, FSpawnInfo SpawnInfo, TArray<FNewParticle> &ForceSpawned)
	{
		const float OneOverTwoPi = 1.0f / (2.0f * PI);
		UParticleModuleRequired* RequiredModule = EmitterInfo.RequiredModule;

		// Allocate stack memory for a dummy particle.
		const UPTRINT Alignment = 16;
		uint8* Mem = (uint8*)FMemory_Alloca( ParticleSize + (int32)Alignment );
		Mem += Alignment - 1;
		Mem = (uint8*)(UPTRINT(Mem) & ~(Alignment - 1));

		FBaseParticle* TempParticle = (FBaseParticle*)Mem;


		// Figure out if we need to replicate the X channel of size to Y.
		const bool bSquare = (EmitterInfo.ScreenAlignment == PSA_Square)
			|| (EmitterInfo.ScreenAlignment == PSA_FacingCameraPosition);

		// Compute the distance covered by the emitter during this time period.
		const FVector EmitterLocation = (RequiredModule->bUseLocalSpace) ? FVector::ZeroVector : Location;
		const FVector EmitterDelta = (RequiredModule->bUseLocalSpace) ? FVector::ZeroVector : (OldLocation - Location);

		// Construct new particles.
		for (int32 i = SpawnInfo.Count; i > 0; --i)
		{
			// Reset the temporary particle.
			FMemory::Memzero( TempParticle, ParticleSize );

			// Set the particle's location and invoke each spawn module on the particle.
			TempParticle->Location = EmitterToSimulation.GetOrigin();

			int32 ForceSpawnedOffset = SpawnInfo.Count - ForceSpawned.Num();
			if (ForceSpawned.Num() && i > ForceSpawnedOffset)
			{
				TempParticle->Location = ForceSpawned[i - ForceSpawnedOffset - 1].Position;
				TempParticle->RelativeTime = ForceSpawned[i - ForceSpawnedOffset - 1].RelativeTime;
				TempParticle->Velocity += ForceSpawned[i - ForceSpawnedOffset - 1].Velocity;
			}

			for (int32 ModuleIndex = 0; ModuleIndex < EmitterInfo.SpawnModules.Num(); ModuleIndex++)
			{
				UParticleModule* SpawnModule = EmitterInfo.SpawnModules[ModuleIndex];
				if (SpawnModule->bEnabled)
				{
					SpawnModule->Spawn(this, /*Offset=*/ 0, SpawnInfo.StartTime, TempParticle);
				}
			}

			const float RandomOrbit = RandomStream.GetFraction();
			FNewParticle* NewParticle = InNewParticles++;
			int32 AllocatedTileIndex = NewParticle->ResilienceAndTileIndex.AllocatedTileIndex;
			float InterpFraction = (float)i / (float)SpawnInfo.Count;

			NewParticle->Velocity = TempParticle->BaseVelocity;
			NewParticle->Position = TempParticle->Location + InterpFraction * EmitterDelta + SpawnInfo.StartTime * NewParticle->Velocity + EmitterInfo.OrbitOffsetBase + EmitterInfo.OrbitOffsetRange * RandomOrbit;
			NewParticle->RelativeTime = TempParticle->RelativeTime;
			NewParticle->TimeScale = FMath::Max<float>(TempParticle->OneOverMaxLifetime, 0.001f);

			//So here I'm reducing the size to 0-0.5 range and using < 0.5 to indicate flipped UVs.
			FVector BaseSize = GetParticleBaseSize(*TempParticle, true);
			FVector2D UVFlipSizeOffset = FVector2D(BaseSize.X < 0.0f ? 0.0f : 0.5f, BaseSize.Y < 0.0f ? 0.0f : 0.5f);
			NewParticle->Size.X = (FMath::Abs(BaseSize.X) * EmitterInfo.InvMaxSize.X * 0.5f);
			NewParticle->Size.Y = bSquare ? (NewParticle->Size.X) : (FMath::Abs(BaseSize.Y) * EmitterInfo.InvMaxSize.Y * 0.5f);
			NewParticle->Size += UVFlipSizeOffset;

			NewParticle->Rotation = FMath::Fractional( TempParticle->Rotation * OneOverTwoPi );
			NewParticle->RelativeRotationRate = TempParticle->BaseRotationRate * OneOverTwoPi * EmitterInfo.InvRotationRateScale / NewParticle->TimeScale;
			NewParticle->RandomOrbit = RandomOrbit;
			EmitterInfo.VectorFieldScale.GetRandomValue(EmitterTime, &NewParticle->VectorFieldScale, RandomStream);
			EmitterInfo.DragCoefficient.GetRandomValue(EmitterTime, &NewParticle->DragCoefficient, RandomStream);
			EmitterInfo.Resilience.GetRandomValue(EmitterTime, &NewParticle->ResilienceAndTileIndex.Resilience, RandomStream);
			SpawnInfo.StartTime -= SpawnInfo.Increment;

			const float PrevTileTimeOfDeath = TileTimeOfDeath[AllocatedTileIndex];
			const float ParticleTimeOfDeath = SecondsSinceCreation + 1.0f / NewParticle->TimeScale;
			const float NewTileTimeOfDeath = FMath::Max(PrevTileTimeOfDeath, ParticleTimeOfDeath);
			TileTimeOfDeath[AllocatedTileIndex] = NewTileTimeOfDeath;
		}
	}

	/**
	 * Update the local vector field.
	 * @param DeltaSeconds - The amount of time by which to move forward simulation.
	 */
	void TickLocalVectorField(float DeltaSeconds)
	{
		LocalVectorFieldRotation += EmitterInfo.LocalVectorField.RotationRate * DeltaSeconds;
	}

	virtual void UpdateBoundingBox(float DeltaSeconds) override
	{
		// Setup a bogus bounding box at the origin. GPU emitters must use fixed bounds.
		FVector Origin = Component ? Component->GetComponentToWorld().GetTranslation() : FVector::ZeroVector;
		ParticleBoundingBox = FBox::BuildAABB(Origin, FVector(1.0f));
	}

	virtual bool Resize(int32 NewMaxActiveParticles, bool bSetMaxActiveCount = true) override
	{
		return false;
	}

	virtual float Tick_SpawnParticles(float DeltaTime, UParticleLODLevel* CurrentLODLevel, bool bSuppressSpawning, bool bFirstTime) override
	{
		return 0.0f;
	}

	virtual void Tick_ModulePreUpdate(float DeltaTime, UParticleLODLevel* CurrentLODLevel)
	{
	}

	virtual void Tick_ModuleUpdate(float DeltaTime, UParticleLODLevel* CurrentLODLevel) override
	{
		// We cannot update particles that have spawned, but modules such as BoneSocket and Skel Vert/Surface may need to perform calculations each tick.
		UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];
		check(HighestLODLevel);
		for (int32 ModuleIndex = 0; ModuleIndex < CurrentLODLevel->UpdateModules.Num(); ModuleIndex++)
		{
			UParticleModule* CurrentModule	= CurrentLODLevel->UpdateModules[ModuleIndex];
			if (CurrentModule && CurrentModule->bEnabled && CurrentModule->bUpdateModule && CurrentModule->bUpdateForGPUEmitter)
			{
				uint32* Offset = ModuleOffsetMap.Find(HighestLODLevel->UpdateModules[ModuleIndex]);
				CurrentModule->Update(this, Offset ? *Offset : 0, DeltaTime);
			}
		}
	}

	virtual void Tick_ModulePostUpdate(float DeltaTime, UParticleLODLevel* CurrentLODLevel) override
	{
	}

	virtual void Tick_ModuleFinalUpdate(float DeltaTime, UParticleLODLevel* CurrentLODLevel) override
	{
		// We cannot update particles that have spawned, but modules such as BoneSocket and Skel Vert/Surface may need to perform calculations each tick.
		UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];
		check(HighestLODLevel);
		for (int32 ModuleIndex = 0; ModuleIndex < CurrentLODLevel->UpdateModules.Num(); ModuleIndex++)
		{
			UParticleModule* CurrentModule	= CurrentLODLevel->UpdateModules[ModuleIndex];
			if (CurrentModule && CurrentModule->bEnabled && CurrentModule->bFinalUpdateModule && CurrentModule->bUpdateForGPUEmitter)
			{
				uint32* Offset = ModuleOffsetMap.Find(HighestLODLevel->UpdateModules[ModuleIndex]);
				CurrentModule->FinalUpdate(this, Offset ? *Offset : 0, DeltaTime);
			}
		}
	}

	// NVCHANGE_BEGIN: JCAO - Field Sampler Module for GPU particle
#if WITH_APEX_TURBULENCE
	void Tick_LocalFieldSampler(float DeltaTime)
	{
		if (EmitterInfo.LocalFieldSamplers.Num() != Simulation->LocalFieldSamplers.Num())
		{
			Simulation->DestroyLocalFieldSampler(FXSystem);

			FPhysScene* PhysScene = Component->GetWorld()->GetPhysicsScene();
			check(PhysScene);
			bool bIsCascade = (Component->GetClass()->GetName() == TEXT("CascadeParticleSystemComponent"));
			Simulation->InitLocalFieldSamplers(PhysScene, bIsCascade ? PST_Sync : PST_Async, Component, FXSystem, EmitterInfo.LocalFieldSamplers);
		}
	}
#endif
	// NVCHANGE_END: JCAO - Field Sampler Module for GPU particle

	virtual void SetCurrentLODIndex(int32 InLODIndex, bool bInFullyProcess) override
	{
		bool bDifferent = (InLODIndex != CurrentLODLevelIndex);
		FParticleEmitterInstance::SetCurrentLODIndex(InLODIndex, bInFullyProcess);
	}

	virtual uint32 RequiredBytes() override
	{
		return 0;
	}

	virtual uint8* GetTypeDataModuleInstanceData() override
	{
		return NULL;
	}

	virtual uint32 CalculateParticleStride(uint32 ParticleSize) override
	{
		return ParticleSize;
	}

	virtual void ResetParticleParameters(float DeltaTime) override
	{
	}

	void CalculateOrbitOffset(FOrbitChainModuleInstancePayload& Payload, 
		FVector& AccumOffset, FVector& AccumRotation, FVector& AccumRotationRate, 
		float DeltaTime, FVector& Result, FMatrix& RotationMat)
	{
	}

	virtual void UpdateOrbitData(float DeltaTime) override
	{

	}

	virtual void ParticlePrefetch() override
	{
	}

	virtual float Spawn(float DeltaTime) override
	{
		return 0.0f;
	}

	virtual void ForceSpawn(float DeltaTime, int32 InSpawnCount, int32 InBurstCount, FVector& InLocation, FVector& InVelocity) override
	{
		const bool bUseLocalSpace = GetCurrentLODLevelChecked()->RequiredModule->bUseLocalSpace;
		FVector SpawnLocation = bUseLocalSpace ? FVector::ZeroVector : InLocation;

		float Increment = DeltaTime / InSpawnCount;
		for (int32 i = 0; i < InSpawnCount; i++)
		{

			FNewParticle Particle;
			Particle.Position = SpawnLocation;
			Particle.Velocity = InVelocity;
			Particle.RelativeTime = Increment*i;
			ForceSpawnedParticles.Add(Particle);
		}

		for (int32 i = 0; i < InBurstCount; i++)
		{
			FNewParticle Particle;
			Particle.Position = SpawnLocation;
			Particle.Velocity = InVelocity;
			Particle.RelativeTime = 0.0f;
			ForceBurstSpawnedParticles.Add(Particle);
		}
	}

	virtual void PreSpawn(FBaseParticle* Particle, const FVector& InitialLocation, const FVector& InitialVelocity) override
	{
	}

	virtual void PostSpawn(FBaseParticle* Particle, float InterpolationPercentage, float SpawnTime) override
	{
	}

	virtual void KillParticles() override
	{
	}

	virtual void KillParticle(int32 Index) override
	{
	}

	virtual void RemovedFromScene()
	{
	}

	virtual FBaseParticle* GetParticle(int32 Index) override
	{
		return NULL;
	}

	virtual FBaseParticle* GetParticleDirect(int32 InDirectIndex) override
	{
		return NULL;
	}

protected:

	virtual bool FillReplayData(FDynamicEmitterReplayDataBase& OutData) override
	{
		return true;
	}
};

#if TRACK_TILE_ALLOCATIONS
void DumpTileAllocations()
{
	for (TMap<FFXSystem*,TSet<FGPUSpriteParticleEmitterInstance*> >::TConstIterator It(GPUSpriteParticleEmitterInstances); It; ++It)
	{
		FFXSystem* FXSystem = It.Key();
		const TSet<FGPUSpriteParticleEmitterInstance*>& Emitters = It.Value();
		int32 TotalAllocatedTiles = 0;

		UE_LOG(LogParticles,Display,TEXT("---------- GPU Particle Tile Allocations : FXSystem=0x%016x ----------"), (PTRINT)FXSystem);
		for (TSet<FGPUSpriteParticleEmitterInstance*>::TConstIterator It(Emitters); It; ++It)
		{
			FGPUSpriteParticleEmitterInstance* Emitter = *It;
			int32 TileCount = Emitter->GetAllocatedTileCount();
			UE_LOG(LogParticles,Display,
				TEXT("%s|%s|0x%016x %d tiles"),
				*Emitter->Component->GetName(),*Emitter->Component->Template->GetName(),(PTRINT)Emitter, TileCount);
			TotalAllocatedTiles += TileCount;
		}

		UE_LOG(LogParticles,Display,TEXT("---"));
		UE_LOG(LogParticles,Display,TEXT("Total Allocated: %d"), TotalAllocatedTiles);
		UE_LOG(LogParticles,Display,TEXT("Free (est.): %d"), GParticleSimulationTileCount - TotalAllocatedTiles);
		if (FXSystem)
		{
			UE_LOG(LogParticles,Display,TEXT("Free (actual): %d"), FXSystem->GetParticleSimulationResources()->GetFreeTileCount());
			UE_LOG(LogParticles,Display,TEXT("Leaked: %d"), GParticleSimulationTileCount - TotalAllocatedTiles - FXSystem->GetParticleSimulationResources()->GetFreeTileCount());
		}
	}
}

FAutoConsoleCommand DumpTileAllocsCommand(
	TEXT("FX.DumpTileAllocations"),
	TEXT("Dump GPU particle tile allocations."),
	FConsoleCommandDelegate::CreateStatic(DumpTileAllocations)
	);
#endif // #if TRACK_TILE_ALLOCATIONS

/*-----------------------------------------------------------------------------
	Internal interface.
-----------------------------------------------------------------------------*/

void FFXSystem::InitGPUSimulation()
{
	check(ParticleSimulationResources == NULL);
	ParticleSimulationResources = new FParticleSimulationResources();
	InitGPUResources();
}

void FFXSystem::DestroyGPUSimulation()
{
	UE_LOG(LogParticles,Log,
		TEXT("Destroying %d GPU particle simulations for FXSystem 0x%p"),
		GPUSimulations.Num(),
		this
		);
	for ( TSparseArray<FParticleSimulationGPU*>::TIterator It(GPUSimulations); It; ++It )
	{
		FParticleSimulationGPU* Simulation = *It;
		Simulation->SimulationIndex = INDEX_NONE;
	}
	GPUSimulations.Empty();
	ReleaseGPUResources();
	ParticleSimulationResources->Destroy();
	ParticleSimulationResources = NULL;
}

void FFXSystem::InitGPUResources()
{
	if (RHISupportsGPUParticles(FeatureLevel))
	{
		check(ParticleSimulationResources);
		ParticleSimulationResources->Init();
	}
}

void FFXSystem::ReleaseGPUResources()
{
	if (RHISupportsGPUParticles(FeatureLevel))
	{
		check(ParticleSimulationResources);
		ParticleSimulationResources->Release();
	}
}

void FFXSystem::AddGPUSimulation(FParticleSimulationGPU* Simulation)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FAddGPUSimulationCommand,
		FFXSystem*, FXSystem, this,
		FParticleSimulationGPU*, Simulation, Simulation,
	{
		if (Simulation->SimulationIndex == INDEX_NONE)
		{
			FSparseArrayAllocationInfo Allocation = FXSystem->GPUSimulations.AddUninitialized();
			Simulation->SimulationIndex = Allocation.Index;
			FXSystem->GPUSimulations[Allocation.Index] = Simulation;
		}
		check(FXSystem->GPUSimulations[Simulation->SimulationIndex] == Simulation);
	});
}

void FFXSystem::RemoveGPUSimulation(FParticleSimulationGPU* Simulation)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FRemoveGPUSimulationCommand,
		FFXSystem*, FXSystem, this,
		FParticleSimulationGPU*, Simulation, Simulation,
	{
		if (Simulation->SimulationIndex != INDEX_NONE)
		{
			check(FXSystem->GPUSimulations[Simulation->SimulationIndex] == Simulation);
			FXSystem->GPUSimulations.RemoveAt(Simulation->SimulationIndex);
		}
		Simulation->SimulationIndex = INDEX_NONE;
	});
}

int32 FFXSystem::AddSortedGPUSimulation(FParticleSimulationGPU* Simulation, const FVector& ViewOrigin)
{
	check(FeatureLevel == ERHIFeatureLevel::SM5);
	const int32 BufferOffset = ParticleSimulationResources->SortedParticleCount;
	ParticleSimulationResources->SortedParticleCount += Simulation->VertexBuffer.ParticleCount;
	FParticleSimulationSortInfo* SortInfo = new(ParticleSimulationResources->SimulationsToSort) FParticleSimulationSortInfo();
	SortInfo->VertexBufferSRV = Simulation->VertexBuffer.VertexBufferSRV;
	SortInfo->ViewOrigin = ViewOrigin;
	SortInfo->ParticleCount = Simulation->VertexBuffer.ParticleCount;
	return BufferOffset;
}

void FFXSystem::AdvanceGPUParticleFrame()
{
	// We double buffer, so swap the current and previous textures.
	ParticleSimulationResources->FrameIndex ^= 0x1;

	// Reset the list of sorted simulations. As PreRenderView is called on GPU simulations we'll
	// allocate space for them in the sorted index buffer.
	ParticleSimulationResources->SimulationsToSort.Reset();
	ParticleSimulationResources->SortedParticleCount = 0;
}

void FFXSystem::SortGPUParticles(FRHICommandListImmediate& RHICmdList)
{
	if (ParticleSimulationResources->SimulationsToSort.Num() > 0)
	{
		int32 BufferIndex = SortParticlesGPU(
			RHICmdList,
			GParticleSortBuffers,
			ParticleSimulationResources->GetCurrentStateTextures().PositionTextureRHI,
			ParticleSimulationResources->SimulationsToSort,
			GetFeatureLevel()
			);
		ParticleSimulationResources->SortedVertexBuffer.VertexBufferRHI =
			GParticleSortBuffers.GetSortedVertexBufferRHI(BufferIndex);
		ParticleSimulationResources->SortedVertexBuffer.VertexBufferSRV =
			GParticleSortBuffers.GetSortedVertexBufferSRV(BufferIndex);
	}
}

// NVCHANGE_BEGIN: JCAO - Replace vector fields with APEX turbulence velocity fields
#if WITH_APEX_TURBULENCE
// NVCHANGE_BEGIN: JCAO - Add Attractor working with GPU particles
static void SetParametersForAttractorFS(FAttractorFSUniformParameters& OutParameters, FAttractorFSInstance* AttractorFSInstance)
{
	check(AttractorFSInstance);

	OutParameters.Origin = AttractorFSInstance->Origin;
	OutParameters.RadiusAndStrength = FVector(AttractorFSInstance->Radius, AttractorFSInstance->ConstFieldStrength, AttractorFSInstance->VariableFieldStrength);
}
// NVCHANGE_BEGIN: JCAO - Support Force Type Noise
static void SetParametersForNoiseFS(FNoiseFSUniformParameters& OutParameters, FNoiseFSInstance* NoiseFSInstance)
{
	check(NoiseFSInstance);

	OutParameters.NoiseSpaceFreq = NoiseFSInstance->NoiseSpaceFreq;
	OutParameters.NoiseSpaceFreqOctaveMultiplier = NoiseFSInstance->NoiseSpaceFreqOctaveMultiplier;
	OutParameters.NoiseStrengthAndTimeFreq = FVector4(NoiseFSInstance->NoiseStrength, NoiseFSInstance->NoiseTimeFreq, NoiseFSInstance->NoiseStrengthOctaveMultiplier, NoiseFSInstance->NoiseTimeFreqOctaveMultiplier);
	OutParameters.NoiseOctaves = NoiseFSInstance->NoiseOctaves;
	OutParameters.NoiseType = NoiseFSInstance->NoiseType;
	OutParameters.NoiseSeed = NoiseFSInstance->NoiseSeed;
}
// NVCHANGE_END: JCAO - Support Force Type Noise
// NVCHANGE_END: JCAO - Add Attractor working with GPU particles

static void SetParametersForVelocityField(FVectorFieldUniformParameters& OutParameters, FTurbulenceFSInstance* TurbulenceFSInstance, FApexRenderSurfaceBuffer* SurfaceBuffer, int32 Index)
{
	check(TurbulenceFSInstance && SurfaceBuffer);
	check(Index < MAX_VECTOR_FIELDS);

	OutParameters.WorldToVolume[Index] = TurbulenceFSInstance->WorldToVolume;
	OutParameters.VolumeToWorld[Index] = TurbulenceFSInstance->VolumeToWorldNoScale;
	OutParameters.VolumeSize[Index] = FVector(SurfaceBuffer->SizeX, SurfaceBuffer->SizeY, SurfaceBuffer->SizeZ);
	OutParameters.IntensityAndTightness[Index] = FVector2D(TurbulenceFSInstance->VelocityMultiplier, TurbulenceFSInstance->VelocityWeight);
	OutParameters.TilingAxes[Index].X = 0.0f;
	OutParameters.TilingAxes[Index].Y = 0.0f;
	OutParameters.TilingAxes[Index].Z = 0.0f;
}
#endif
// NVCHANGE_END: JCAO - Replace vector fields with APEX turbulence velocity fields

/**
 * Sets parameters for the vector field instance.
 * @param OutParameters - The uniform parameters structure.
 * @param VectorFieldInstance - The vector field instance.
 * @param EmitterScale - Amount to scale the vector field by.
 * @param EmitterTightness - Tightness override for the vector field.
 * @param Index - Index of the vector field.
 */
static void SetParametersForVectorField(FVectorFieldUniformParameters& OutParameters, FVectorFieldInstance* VectorFieldInstance, float EmitterScale, float EmitterTightness, int32 Index)
{
	check(VectorFieldInstance && VectorFieldInstance->Resource);
	check(Index < MAX_VECTOR_FIELDS);

	FVectorFieldResource* Resource = VectorFieldInstance->Resource;
	const float Intensity = VectorFieldInstance->Intensity * Resource->Intensity * EmitterScale;

	// Override vector field tightness if value is set (greater than 0). This override is only used for global vector fields.
	float Tightness = EmitterTightness;
	if(EmitterTightness == -1)
	{
		Tightness = FMath::Clamp<float>(VectorFieldInstance->Tightness, 0.0f, 1.0f);
	}

	OutParameters.WorldToVolume[Index] = VectorFieldInstance->WorldToVolume;
	OutParameters.VolumeToWorld[Index] = VectorFieldInstance->VolumeToWorldNoScale;
	OutParameters.VolumeSize[Index] = FVector(Resource->SizeX, Resource->SizeY, Resource->SizeZ);
	OutParameters.IntensityAndTightness[Index] = FVector2D(Intensity, Tightness );
	OutParameters.TilingAxes[Index].X = VectorFieldInstance->bTileX ? 1.0f : 0.0f;
	OutParameters.TilingAxes[Index].Y = VectorFieldInstance->bTileY ? 1.0f : 0.0f;
	OutParameters.TilingAxes[Index].Z = VectorFieldInstance->bTileZ ? 1.0f : 0.0f;
}

bool FFXSystem::UsesGlobalDistanceFieldInternal() const
{
	for (TSparseArray<FParticleSimulationGPU*>::TConstIterator It(GPUSimulations); It; ++It)
	{
		const FParticleSimulationGPU* Simulation = *It;

		if (Simulation->SimulationPhase == EParticleSimulatePhase::CollisionDistanceField
			&& Simulation->TileVertexBuffer.AlignedTileCount > 0)
		{
			return true;
		}
	}

	return false;
}

// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
static float Distance(const FVector4& V1, const FVector4& V2)
{
	FVector VA(V1.X / V1.W, V1.Y / V1.W, V1.Z / V1.W);
	FVector VB(V2.X / V2.W, V2.Y / V2.W, V2.Z / V2.W);
	return (VB - VA).Size();
}


static void SetGridDensityFrustumUniformParameters(FGridDensityFrustumUniformParameters& OutParameters, const FParticleGridDensityResource* GridDensityResource, const FViewMatrices& ViewMatrices)
{
	FMatrix ViewMatrix = ViewMatrices.ViewMatrix;

	ViewMatrix.M[3][0] *= SCALE_FACTOR;
	ViewMatrix.M[3][1] *= SCALE_FACTOR;
	ViewMatrix.M[3][2] *= SCALE_FACTOR;

	FMatrix ProjMatrix = ViewMatrices.ProjMatrix;

	ProjMatrix.M[2][0] = 0.0f;
	ProjMatrix.M[2][1] = 0.0f;
	ProjMatrix.M[2][2] = 1.0f - 0.001f;
	ProjMatrix.M[3][2] = -ProjMatrix.M[3][2] * ProjMatrix.M[2][2] * SCALE_FACTOR;

	FMatrix MatInverse = (ViewMatrix * ProjMatrix).Inverse();

	const float TargetDepth = GridDensityResource->GridDepth;

	//// to calculate w transform
	float NearDimX = Distance(MatInverse.TransformFVector4(FVector4(-1.f, 0.f, 0.f, 1.f)), MatInverse.TransformFVector4(FVector4(1.f, 0.f, 0.f, 1.f)));
	float NearDimY = Distance(MatInverse.TransformFVector4(FVector4(0.f, -1.f, 0.f, 1.f)), MatInverse.TransformFVector4(FVector4(0.f, 1.f, 0.f, 1.f)));

	float FarDimX = Distance(MatInverse.TransformFVector4(FVector4(-1.f, 0.f, 1.f, 1.f)), MatInverse.TransformFVector4(FVector4(1.f, 0.f, 1.f, 1.f)));
	float FarDimY = Distance(MatInverse.TransformFVector4(FVector4(0.f, -1.f, 1.f, 1.f)), MatInverse.TransformFVector4(FVector4(0.f, 1.f, 1.f, 1.f)));
	float DimZ = Distance(MatInverse.TransformFVector4(FVector4(0.f, 0.f, 0.f, 1.f)), MatInverse.TransformFVector4(FVector4(0.f, 0.f, 1.f, 1.f)));

	float MyFarDimX = NearDimX*(1.f - TargetDepth / DimZ) + FarDimX*(TargetDepth / DimZ);
	float MyFarDimY = NearDimY*(1.f - TargetDepth / DimZ) + FarDimY*(TargetDepth / DimZ);

	//// grab necessary frustum coordinates
	FVector4 Origin4 = MatInverse.TransformFVector4(FVector4(-1.f, 1.f, 0.f, 1.f));
	FVector4 BasisX4 = MatInverse.TransformFVector4(FVector4(1.f, 1.f, 0.f, 1.f));
	FVector4 BasisY4 = MatInverse.TransformFVector4(FVector4(-1.f, -1.f, 0.f, 1.f));
	FVector4 ZDepth4 = MatInverse.TransformFVector4(FVector4(-1.f, 1.f, 1.f, 1.f));

	// create vec3 versions
	FVector Origin3(Origin4.X / Origin4.W, Origin4.Y / Origin4.W, Origin4.Z / Origin4.W);
	FVector BasisX3(BasisX4.X / BasisX4.W, BasisX4.Y / BasisX4.W, BasisX4.Z / BasisX4.W);
	FVector BasisY3(BasisY4.X / BasisY4.W, BasisY4.Y / BasisY4.W, BasisY4.Z / BasisY4.W);
	FVector ZDepth3(ZDepth4.X / ZDepth4.W, ZDepth4.Y / ZDepth4.W, ZDepth4.Z / ZDepth4.W);

	// make everthing relative to origin
	BasisX3 -= Origin3;
	BasisY3 -= Origin3;
	ZDepth3 -= Origin3;
	// find third basis
	FVector BasisZ3 = (BasisX3^BasisY3).GetSafeNormal();
	BasisZ3 *= TargetDepth;
	// see how skewed the eye point is
	FVector Eye;
	{
		// find the eye point
		FVector4 A4 = MatInverse.TransformFVector4(FVector4(1.f, 1.f, 0.00f, 1.f));
		FVector4 B4 = MatInverse.TransformFVector4(FVector4(1.f, 1.f, 0.01f, 1.f));
		FVector4 C4 = MatInverse.TransformFVector4(FVector4(-1.f, -1.f, 0.00f, 1.f));
		FVector4 D4 = MatInverse.TransformFVector4(FVector4(-1.f, -1.f, 0.01f, 1.f));

		FVector A3 = FVector(A4.X / A4.W, A4.Y / A4.W, A4.Z / A4.W);
		FVector B3 = FVector(B4.X / B4.W, B4.Y / B4.W, B4.Z / B4.W);
		FVector C3 = FVector(C4.X / C4.W, C4.Y / C4.W, C4.Z / C4.W);
		FVector D3 = FVector(D4.X / D4.W, D4.Y / D4.W, D4.Z / D4.W);

		FVector A = B3 - A3;
		FVector	B = D3 - C3;
		FVector C = A ^ B;
		FVector D = A3 - C3;

		FMatrix InvEyeMatrix = FMatrix(A, B, C, FVector4(0)).Inverse();
		FVector4 Coord = InvEyeMatrix.TransformFVector4(FVector4(D, 1.0f));
		Eye = C3 + B * Coord.Y;
	}

	// build scale,rotation,translation matrix
	FMatrix Mat1 = FMatrix(BasisX3, BasisY3, BasisZ3, Origin3).Inverse();
	FVector4 EyeOffset = Mat1.TransformFVector4(FVector4(Eye, 1.0f));

	// do perspective transform
	FMatrix Mat2 = FMatrix::Identity;
	{
		float XShift = -2.f*(EyeOffset.X - 0.5f);
		float YShift = -2.f*(EyeOffset.Y - 0.5f);
		float Left = -3.0f + XShift;
		float Right = 1.0f + XShift;
		float Top = 1.0f + YShift;
		float Bottom = -3.0f + YShift;
		float NearVal = NearDimX / (0.5f*(MyFarDimX - NearDimX));

		// build matrix

		Mat2.M[0][0] = -2.f*NearVal / (Right - Left);
		Mat2.M[1][1] = -2.f*NearVal / (Top - Bottom);
		Mat2.M[2][0] = (Right + Left) / (Right - Left);
		Mat2.M[2][1] = (Top + Bottom) / (Top - Bottom);
		Mat2.M[2][3] = -1.f;
		Mat2.M[3][3] = 0.f;
	}

	// shrink to calculate density just outside of frustum
	FMatrix Mat3 = FMatrix::Identity;
	float Factor = FMath::Min((float)(GridDensityResource->GridResolution - 4) / (GridDensityResource->GridResolution), 0.75f);
	{
		Mat3.M[0][0] = Factor;
		Mat3.M[1][1] = Factor;
		Mat3.M[2][2] = Factor;
		Mat3.M[3][0] = (1.0f - Factor)*0.5f;
		Mat3.M[3][1] = (1.0f - Factor)*0.5f;
		Mat3.M[3][2] = (1.0f - Factor)*0.5f;
	}

	// create frustum info
	OutParameters.DimMatrix = Mat1 * Mat2 * Mat3; // create final matrix
	OutParameters.NearDim = FVector2D(Factor*NearDimX, Factor*NearDimY);
	OutParameters.FarDim = FVector2D(Factor*MyFarDimX, Factor*MyFarDimY);
	OutParameters.DimZ = Factor*TargetDepth;
}
// NVCHANGE_END: JCAO - Grid Density with GPU particles

void FFXSystem::SimulateGPUParticles(
	FRHICommandListImmediate& RHICmdList,
	EParticleSimulatePhase::Type Phase,
	const class FSceneView* CollisionView,
	const FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData,
	FTexture2DRHIParamRef SceneDepthTexture,
	FTexture2DRHIParamRef GBufferATexture
	)
{
	check(IsInRenderingThread());
	SCOPE_CYCLE_COUNTER(STAT_GPUParticleTickTime);

	FMemMark Mark(FMemStack::Get());

	// Grab resources.
	FParticleStateTextures& CurrentStateTextures = ParticleSimulationResources->GetCurrentStateTextures();
	FParticleStateTextures& PrevStateTextures = ParticleSimulationResources->GetPreviousStateTextures();

	
	// On some platforms, the textures are filled with garbage after creation, so we need to clear them to black the first time we use them
	if ( !CurrentStateTextures.bTexturesCleared )
	{
		SetRenderTarget(RHICmdList, CurrentStateTextures.PositionTextureTargetRHI, FTextureRHIRef(), ESimpleRenderTargetMode::EClearColorAndDepth);
		SetRenderTarget(RHICmdList, CurrentStateTextures.VelocityTextureTargetRHI, FTextureRHIRef(), ESimpleRenderTargetMode::EClearColorAndDepth);

		CurrentStateTextures.bTexturesCleared = true;
	}

	if ( !PrevStateTextures.bTexturesCleared )
	{
		SetRenderTarget(RHICmdList, PrevStateTextures.PositionTextureTargetRHI, FTextureRHIRef(), ESimpleRenderTargetMode::EClearColorAndDepth);
		SetRenderTarget(RHICmdList, PrevStateTextures.VelocityTextureTargetRHI, FTextureRHIRef(), ESimpleRenderTargetMode::EClearColorAndDepth);
		RHICmdList.CopyToResolveTarget(PrevStateTextures.PositionTextureTargetRHI, PrevStateTextures.PositionTextureTargetRHI, true, FResolveParams());
		RHICmdList.CopyToResolveTarget(PrevStateTextures.VelocityTextureTargetRHI, PrevStateTextures.VelocityTextureTargetRHI, true, FResolveParams());
		
		PrevStateTextures.bTexturesCleared = true;
	}

	// Setup render states.
	FTextureRHIParamRef RenderTargets[2] =
	{
		CurrentStateTextures.PositionTextureTargetRHI,
		CurrentStateTextures.VelocityTextureTargetRHI,
	};
	SetRenderTargets(RHICmdList, 2, RenderTargets, FTextureRHIParamRef(), 0, NULL);
	RHICmdList.SetViewport(0, 0, 0.0f, GParticleSimulationTextureSizeX, GParticleSimulationTextureSizeY, 1.0f);
	RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
	RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
	RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());

	// Simulations that don't use vector fields can share some state.
	FVectorFieldUniformBufferRef EmptyVectorFieldUniformBuffer;
	{
		FVectorFieldUniformParameters VectorFieldParameters;
		FTexture3DRHIParamRef BlackVolumeTextureRHI = (FTexture3DRHIParamRef)(FTextureRHIParamRef)GBlackVolumeTexture->TextureRHI;
		for (int32 Index = 0; Index < MAX_VECTOR_FIELDS; ++Index)
		{
			VectorFieldParameters.WorldToVolume[Index] = FMatrix::Identity;
			VectorFieldParameters.VolumeToWorld[Index] = FMatrix::Identity;
			VectorFieldParameters.VolumeSize[Index] = FVector(1.0f);
			VectorFieldParameters.IntensityAndTightness[Index] = FVector2D::ZeroVector;
		}
		// NVCHANGE_BEGIN: JCAO - Replace vector fields with APEX turbulence velocity fields
		VectorFieldParameters.VectorFieldCount = 0;
		VectorFieldParameters.VelocityFieldCount = 0;
		// NVCHANGE_END: JCAO - Replace vector fields with APEX turbulence velocity fields
		EmptyVectorFieldUniformBuffer = FVectorFieldUniformBufferRef::CreateUniformBufferImmediate(VectorFieldParameters, UniformBuffer_SingleFrame);
	}

	// NVCHANGE_BEGIN: JCAO - Add Attractor working with GPU particles
#if WITH_APEX_TURBULENCE
	FAttractorFSUniformBufferRef EmptyAttractorFSUniformBuffer;
	{
		FAttractorFSUniformParameters AttractorFSParameters;
		AttractorFSParameters.Origin = FVector(1.0f);
		AttractorFSParameters.RadiusAndStrength = FVector(1.0f);
		AttractorFSParameters.Count = 0;
		EmptyAttractorFSUniformBuffer = FAttractorFSUniformBufferRef::CreateUniformBufferImmediate(AttractorFSParameters, UniformBuffer_SingleFrame);
	}
	FNoiseFSUniformBufferRef EmptyNoiseFSUniformBuffer;
	{
		FNoiseFSUniformParameters NoiseFSParameters;
		NoiseFSParameters.NoiseSpaceFreq = FVector(0.0f);
		NoiseFSParameters.NoiseSpaceFreqOctaveMultiplier = FVector(0.0f);
		NoiseFSParameters.NoiseStrengthAndTimeFreq = FVector4(0.0f);
		NoiseFSParameters.NoiseOctaves = 0;
		NoiseFSParameters.NoiseType = 0;
		NoiseFSParameters.NoiseSeed = 0;
		NoiseFSParameters.Count = 0;
		EmptyNoiseFSUniformBuffer = FNoiseFSUniformBufferRef::CreateUniformBufferImmediate(NoiseFSParameters, UniformBuffer_SingleFrame);
	}
#endif // WITH_APEX_TURBULENCE
	// NVCHANGE_END: JCAO - Add Attractor working with GPU particles

	// Gather simulation commands from all active simulations.
	static TArray<FSimulationCommandGPU> SimulationCommands;
	static TArray<uint32> TilesToClear;
	static TArray<FNewParticle> NewParticles;
	for (TSparseArray<FParticleSimulationGPU*>::TIterator It(GPUSimulations); It; ++It)
	{
		SCOPE_CYCLE_COUNTER(STAT_GPUParticleBuildSimCmdsTime);

		FParticleSimulationGPU* Simulation = *It;
		if (Simulation->SimulationPhase == Phase
			&& Simulation->TileVertexBuffer.AlignedTileCount > 0)
		{
			// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
			if (GMaxRHIFeatureLevel == ERHIFeatureLevel::SM5
				&& Simulation->bViewMatricesUpdated
				&& Simulation->GridDensityResource->IsValid())
			{
				// NVCHANGE_BEGIN: JCAO - Add density time in the GPU particle stat
				SCOPE_CYCLE_COUNTER(STAT_GPUParticleDensityTime);
				// NVCHANGE_END: JCAO - Add density time in the GPU particle stat

				// Grab the shader.
				TShaderMapRef<FParticleGridDensityClearCS> GridDensityClearCS(GetGlobalShaderMap(FeatureLevel));
				RHICmdList.SetComputeShader(GridDensityClearCS->GetComputeShader());
				// Dispatch shader to compute bounds.
				GridDensityClearCS->SetOutput(RHICmdList, Simulation->GridDensityResource->GridDensityBufferUAV);
				DispatchComputeShader(
					RHICmdList,
					*GridDensityClearCS,
					Simulation->GridDensityResource->GridResolution * Simulation->GridDensityResource->GridResolution * Simulation->GridDensityResource->GridResolution / PARTICLE_BOUNDS_THREADS,
					1,
					1);
				GridDensityClearCS->UnbindBuffers(RHICmdList);

				FGridDensityFrustumUniformParameters Parameters;
				SetGridDensityFrustumUniformParameters(Parameters, Simulation->GridDensityResource, Simulation->ViewMatrices);
				FGridDensityFrustumUniformBufferRef UniformBuffer =
					FGridDensityFrustumUniformBufferRef::CreateUniformBufferImmediate(Parameters, UniformBuffer_SingleFrame);


				ComputeGridDensityFillFrustum(RHICmdList,
					FeatureLevel,
					UniformBuffer,
					Simulation->GridDensityResource->GridDensityUniformBuffer,
					Simulation->VertexBuffer.VertexBufferSRV,
					PrevStateTextures.PositionTextureRHI,
					Simulation->GridDensityResource->GridDensityBufferUAV,
					Simulation->VertexBuffer.ParticleCount
					);

				TShaderMapRef<FParticleGridDensityLowPassCS> GridDensityLowPassCS(GetGlobalShaderMap(FeatureLevel));
				RHICmdList.SetComputeShader(GridDensityLowPassCS->GetComputeShader());
				GridDensityLowPassCS->SetParameters(RHICmdList, Simulation->GridDensityResource->GridDensityUniformBuffer, Simulation->GridDensityResource->GridDensityBufferSRV);
				GridDensityLowPassCS->SetOutput(RHICmdList, Simulation->GridDensityResource->GridDensityLowPassBufferUAV);
				DispatchComputeShader(
					RHICmdList,
					*GridDensityLowPassCS,
					Simulation->GridDensityResource->GridResolution * Simulation->GridDensityResource->GridResolution * Simulation->GridDensityResource->GridResolution / PARTICLE_BOUNDS_THREADS,
					1,
					1);
				GridDensityLowPassCS->UnbindBuffers(RHICmdList);

				ComputeGridDensityApplyFrustum(RHICmdList,
					FeatureLevel,
					UniformBuffer,
					Simulation->GridDensityResource->GridDensityUniformBuffer,
					Simulation->VertexBuffer.VertexBufferSRV,
					PrevStateTextures.PositionTextureRHI,
					Simulation->GridDensityResource->GridDensityLowPassBufferSRV,
					CurrentStateTextures.DensityTextureUAV,
					Simulation->VertexBuffer.ParticleCount
					);

			}
			// NVCHANGE_END: JCAO - Grid Density with GPU particles

			FSimulationCommandGPU* SimulationCommand = new(SimulationCommands)FSimulationCommandGPU(
				Simulation->TileVertexBuffer.GetShaderParam(),
				Simulation->EmitterSimulationResources->SimulationUniformBuffer,
				Simulation->PerFrameSimulationParameters,
				EmptyVectorFieldUniformBuffer,
#if WITH_APEX_TURBULENCE
				EmptyAttractorFSUniformBuffer,
				EmptyNoiseFSUniformBuffer,
#endif
				Simulation->TileVertexBuffer.AlignedTileCount
				);

			// Determine which vector fields affect this simulation and build the appropriate parameters.
			{
				SCOPE_CYCLE_COUNTER(STAT_GPUParticleVFCullTime);
				FVectorFieldUniformParameters VectorFieldParameters;
				const FBox SimulationBounds = Simulation->Bounds;

				// Add the local vector field.
				// NVCHANGE_BEGIN: JCAO - Replace vector fields with APEX turbulence velocity fields
				VectorFieldParameters.VelocityFieldCount = 0;
				VectorFieldParameters.VectorFieldCount = 0;
				// NVCHANGE_END: JCAO - Replace vector fields with APEX turbulence velocity fields
				if (Simulation->LocalVectorField.Resource)
				{
					const float Intensity = Simulation->LocalVectorField.Intensity * Simulation->LocalVectorField.Resource->Intensity;
					if (FMath::Abs(Intensity) > 0.0f)
					{
						Simulation->LocalVectorField.Resource->Update(RHICmdList, Simulation->PerFrameSimulationParameters.DeltaSeconds);
						SimulationCommand->VectorFieldTexturesRHI[0] = Simulation->LocalVectorField.Resource->VolumeTextureRHI;
						// NVCHANGE_BEGIN: JCAO - Replace vector fields with APEX turbulence velocity fields
						SetParametersForVectorField(VectorFieldParameters, &Simulation->LocalVectorField, /*EmitterScale=*/ 1.0f, /*EmitterTightness=*/ -1, VectorFieldParameters.VectorFieldCount++);
						// NVCHANGE_END: JCAO - Replace vector fields with APEX turbulence velocity fields
					}
				}

				// Add any world vector fields that intersect the simulation.
				const float GlobalVectorFieldScale = Simulation->EmitterSimulationResources->GlobalVectorFieldScale;
				const float GlobalVectorFieldTightness = Simulation->EmitterSimulationResources->GlobalVectorFieldTightness;
				if (FMath::Abs(GlobalVectorFieldScale) > 0.0f)
				{
					// NVCHANGE_BEGIN: JCAO - Replace vector fields with APEX turbulence velocity fields
					for (TSparseArray<FVectorFieldInstance*>::TIterator VectorFieldIt(VectorFields); VectorFieldIt && VectorFieldParameters.VectorFieldCount < MAX_VECTOR_FIELDS; ++VectorFieldIt)
					{
						FVectorFieldInstance* Instance = *VectorFieldIt;
						check(Instance && Instance->Resource);
						const float Intensity = Instance->Intensity * Instance->Resource->Intensity;
						if (SimulationBounds.Intersect(Instance->WorldBounds) &&
							FMath::Abs(Intensity) > 0.0f)
						{
							SimulationCommand->VectorFieldTexturesRHI[VectorFieldParameters.VectorFieldCount] = Instance->Resource->VolumeTextureRHI;
							SetParametersForVectorField(VectorFieldParameters, Instance, GlobalVectorFieldScale, GlobalVectorFieldTightness, VectorFieldParameters.Count++);
						}
					}
					// NVCHANGE_END: JCAO - Replace vector fields with APEX turbulence velocity fields
				}

				// NVCHANGE_BEGIN: JCAO - Replace vector fields with APEX turbulence velocity fields
#if WITH_APEX_TURBULENCE
				{
					for (FTurbulenceFSInstanceList::TIterator TurbulenceFSIt(TurbulenceFSList); TurbulenceFSIt && (VectorFieldParameters.VelocityFieldCount + VectorFieldParameters.VectorFieldCount) < MAX_VECTOR_FIELDS; ++TurbulenceFSIt)
					{
						FTurbulenceFSInstance* Instance = *TurbulenceFSIt;
						check(Instance && Instance->TurbulenceFSActor);

						if (!Instance->bEnabled)
						{
							continue;
						}

						physx::apex::NxTurbulenceFSActor* TurbulenceActor = static_cast<physx::apex::NxTurbulenceFSActor*>(Instance->TurbulenceFSActor->GetApexActor());

						if (SimulationBounds.Intersect(Instance->WorldBounds) && TurbulenceActor)
						{
							FApexRenderSurfaceBuffer* SurfaceBuffer = static_cast<FApexRenderSurfaceBuffer*>(TurbulenceActor->getVelocityFieldRenderSurface());

							if (SurfaceBuffer)
							{
								SimulationCommand->VectorFieldTexturesRHI[VectorFieldParameters.VelocityFieldCount + VectorFieldParameters.VectorFieldCount] = SurfaceBuffer->VolumeTextureRHI;
								SetParametersForVelocityField(VectorFieldParameters, Instance, SurfaceBuffer, VectorFieldParameters.VectorFieldCount + VectorFieldParameters.VelocityFieldCount++);
							}
						}
					}
				}
#endif

				// Fill out any remaining vector field entries.
				if ((VectorFieldParameters.VelocityFieldCount + VectorFieldParameters.VectorFieldCount) > 0)
				{
					int32 PadCount = VectorFieldParameters.VelocityFieldCount + VectorFieldParameters.VectorFieldCount;
					while (PadCount < MAX_VECTOR_FIELDS)
					{
						const int32 Index = PadCount++;
						VectorFieldParameters.WorldToVolume[Index] = FMatrix::Identity;
						VectorFieldParameters.VolumeToWorld[Index] = FMatrix::Identity;
						VectorFieldParameters.VolumeSize[Index] = FVector(1.0f);
						VectorFieldParameters.IntensityAndTightness[Index] = FVector2D::ZeroVector;
					}
					SimulationCommand->VectorFieldsUniformBuffer = FVectorFieldUniformBufferRef::CreateUniformBufferImmediate(VectorFieldParameters, UniformBuffer_SingleFrame);
				}
				// NVCHANGE_END: JCAO - Replace vector fields with APEX turbulence velocity fields

				// NVCHANGE_BEGIN: JCAO - Add Attractor working with GPU particles
#if WITH_APEX_TURBULENCE
				FAttractorFSUniformParameters AttractorFSParameters;

				AttractorFSParameters.Count = 0;

				for (FAttractorFSInstanceList::TIterator AttractorFSIt(AttractorFSList); AttractorFSIt && AttractorFSParameters.Count < 1; ++AttractorFSIt)
				{
					FAttractorFSInstance* Instance = *AttractorFSIt;
					check(Instance);
					if (Instance->bEnabled && SimulationBounds.Intersect(Instance->WorldBounds) && Instance->Radius > 0.0f)
					{
						SetParametersForAttractorFS(AttractorFSParameters, Instance);
						AttractorFSParameters.Count++;
					}
				}

				if (AttractorFSParameters.Count > 0)
				{
					SimulationCommand->AttractorFSUniformBuffer = FAttractorFSUniformBufferRef::CreateUniformBufferImmediate(AttractorFSParameters, UniformBuffer_SingleFrame);
				}

				FNoiseFSUniformParameters NoiseFSParameters;
				NoiseFSParameters.Count = 0;

				for (FNoiseFSInstanceList::TIterator NoiseFSIt(NoiseFSList); NoiseFSIt && NoiseFSParameters.Count < 1; ++NoiseFSIt)
				{
					FNoiseFSInstance* Instance = *NoiseFSIt;
					check(Instance);
					if (Instance->bEnabled && SimulationBounds.Intersect(Instance->WorldBounds))
					{
						SetParametersForNoiseFS(NoiseFSParameters, Instance);
						NoiseFSParameters.Count++;
					}
				}

				if (NoiseFSParameters.Count > 0)
				{
					SimulationCommand->NoiseFSUniformBuffer = FNoiseFSUniformBufferRef::CreateUniformBufferImmediate(NoiseFSParameters, UniformBuffer_SingleFrame);
				}
#endif // WITH_APEX_TURBULENCE
				// NVCHANGE_END: JCAO - Add Attractor working with GPU particles
			}
		
			// Add to the list of tiles to clear.
			TilesToClear.Append(Simulation->TilesToClear);
			Simulation->TilesToClear.Reset();

			// Add to the list of new particles.
			NewParticles.Append(Simulation->NewParticles);
			Simulation->NewParticles.Reset();

			// Reset pending simulation time. This prevents an emitter from simulating twice if we don't get an update from the game thread, e.g. the component didn't tick last frame.
			Simulation->PerFrameSimulationParameters.DeltaSeconds = 0.0f;
		}
	}

	// Simulate particles in all active tiles.
	if ( SimulationCommands.Num() )
	{
		SCOPED_DRAW_EVENT(RHICmdList, ParticleSimulation);

		if (Phase == EParticleSimulatePhase::CollisionDepthBuffer && CollisionView)
		{
			/// ?
			ExecuteSimulationCommands<PCM_DepthBuffer>(
				RHICmdList,
				FeatureLevel,
				SimulationCommands,
				PrevStateTextures,
				ParticleSimulationResources->SimulationAttributesTexture,
				ParticleSimulationResources->RenderAttributesTexture,
				CollisionView,
				GlobalDistanceFieldParameterData,
				SceneDepthTexture,
				GBufferATexture
				);
		}
		else if (Phase == EParticleSimulatePhase::CollisionDistanceField && GlobalDistanceFieldParameterData)
		{
			/// ?
			ExecuteSimulationCommands<PCM_DistanceField>(
				RHICmdList,
				FeatureLevel,
				SimulationCommands,
				PrevStateTextures,
				ParticleSimulationResources->SimulationAttributesTexture,
				ParticleSimulationResources->RenderAttributesTexture,
				CollisionView,
				GlobalDistanceFieldParameterData,
				SceneDepthTexture,
				GBufferATexture
				);
		}
		else
		{
			/// ?
			ExecuteSimulationCommands<PCM_None>(
				RHICmdList,
				FeatureLevel,
				SimulationCommands,
				PrevStateTextures,
				ParticleSimulationResources->SimulationAttributesTexture,
				ParticleSimulationResources->RenderAttributesTexture,
				NULL,
				GlobalDistanceFieldParameterData,
				FTexture2DRHIParamRef(),
				FTexture2DRHIParamRef()
				);
		}
	}

	// Clear any newly allocated tiles.
	if (TilesToClear.Num())
	{
		ClearTiles(RHICmdList, FeatureLevel, TilesToClear);
	}

	// Inject any new particles that have spawned into the simulation.
	if (NewParticles.Num())
	{
		SCOPED_DRAW_EVENT(RHICmdList, ParticleInjection);
		// Set render targets.
		FTextureRHIParamRef InjectRenderTargets[4] =
		{
			CurrentStateTextures.PositionTextureTargetRHI,
			CurrentStateTextures.VelocityTextureTargetRHI,
			ParticleSimulationResources->RenderAttributesTexture.TextureTargetRHI,
			ParticleSimulationResources->SimulationAttributesTexture.TextureTargetRHI
		};
		SetRenderTargets(RHICmdList, 4, InjectRenderTargets, FTextureRHIParamRef(), 0, NULL);
		RHICmdList.SetViewport(0, 0, 0.0f, GParticleSimulationTextureSizeX, GParticleSimulationTextureSizeY, 1.0f);

		// Inject particles.
		InjectNewParticles(RHICmdList, FeatureLevel, NewParticles);

		// Resolve attributes textures. State textures are resolved later.
		RHICmdList.CopyToResolveTarget(
			ParticleSimulationResources->RenderAttributesTexture.TextureTargetRHI,
			ParticleSimulationResources->RenderAttributesTexture.TextureRHI,
			/*bKeepOriginalSurface=*/ false,
			FResolveParams()
			);
		RHICmdList.CopyToResolveTarget(
			ParticleSimulationResources->SimulationAttributesTexture.TextureTargetRHI,
			ParticleSimulationResources->SimulationAttributesTexture.TextureRHI,
			/*bKeepOriginalSurface=*/ false,
			FResolveParams()
			);
	}

	SimulationCommands.Reset();
	TilesToClear.Reset();
	NewParticles.Reset();

	// Resolve all textures.
	RHICmdList.CopyToResolveTarget(CurrentStateTextures.PositionTextureTargetRHI, CurrentStateTextures.PositionTextureRHI, /*bKeepOriginalSurface=*/ false, FResolveParams());
	RHICmdList.CopyToResolveTarget(CurrentStateTextures.VelocityTextureTargetRHI, CurrentStateTextures.VelocityTextureRHI, /*bKeepOriginalSurface=*/ false, FResolveParams());

	// Clear render targets so we can safely read from them.
	SetRenderTarget(RHICmdList, FTextureRHIParamRef(), FTextureRHIParamRef());

	// Stats.
	if (Phase == EParticleSimulatePhase::Last)
	{
		INC_DWORD_STAT_BY(STAT_FreeGPUTiles,ParticleSimulationResources->GetFreeTileCount());
	}
}

void FFXSystem::VisualizeGPUParticles(FCanvas* Canvas)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER(
		FVisualizeGPUParticlesCommand,
		FFXSystem*, FXSystem, this,
		int32, VisualizationMode, FXConsoleVariables::VisualizeGPUSimulation,
		FRenderTarget*, RenderTarget, Canvas->GetRenderTarget(),
		ERHIFeatureLevel::Type, FeatureLevel, GetFeatureLevel(),
	{
		FParticleSimulationResources* Resources = FXSystem->GetParticleSimulationResources();
		FParticleStateTextures& CurrentStateTextures = Resources->GetCurrentStateTextures();
		VisualizeGPUSimulation(RHICmdList, FeatureLevel, VisualizationMode, RenderTarget, CurrentStateTextures, GParticleCurveTexture.GetCurveTexture());
	});
}

/*-----------------------------------------------------------------------------
	External interface.
-----------------------------------------------------------------------------*/

FParticleEmitterInstance* FFXSystem::CreateGPUSpriteEmitterInstance( FGPUSpriteEmitterInfo& EmitterInfo )
{
	return new FGPUSpriteParticleEmitterInstance( this, EmitterInfo );
}

/**
 * Sets GPU sprite resource data.
 * @param Resources - Sprite resources to update.
 * @param InResourceData - Data with which to update resources.
 */
static void SetGPUSpriteResourceData( FGPUSpriteResources* Resources, const FGPUSpriteResourceData& InResourceData )
{
	// Allocate texels for all curves.
	Resources->ColorTexelAllocation = GParticleCurveTexture.AddCurve( InResourceData.QuantizedColorSamples );
	Resources->MiscTexelAllocation = GParticleCurveTexture.AddCurve( InResourceData.QuantizedMiscSamples );
	Resources->SimulationAttrTexelAllocation = GParticleCurveTexture.AddCurve( InResourceData.QuantizedSimulationAttrSamples );
	// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
	Resources->DensityColorTexelAllocation = GParticleCurveTexture.AddCurve(InResourceData.QuantizedDensityColorSamples);
	Resources->DensitySizeTexelAllocation = GParticleCurveTexture.AddCurve(InResourceData.QuantizedDensitySizeSamples);
	// NVCHANGE_END: JCAO - Grid Density with GPU particles

	// Setup uniform parameters for the emitter.
	Resources->UniformParameters.ColorCurve = GParticleCurveTexture.ComputeCurveScaleBias(Resources->ColorTexelAllocation);
	Resources->UniformParameters.ColorScale = InResourceData.ColorScale;
	Resources->UniformParameters.ColorBias = InResourceData.ColorBias;

	Resources->UniformParameters.MiscCurve = GParticleCurveTexture.ComputeCurveScaleBias(Resources->MiscTexelAllocation);
	Resources->UniformParameters.MiscScale = InResourceData.MiscScale;
	Resources->UniformParameters.MiscBias = InResourceData.MiscBias;

	Resources->UniformParameters.SizeBySpeed = InResourceData.SizeBySpeed;
	Resources->UniformParameters.SubImageSize = InResourceData.SubImageSize;

	// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
	// Grid Density
	Resources->GridDensityResource.GridResolution = InResourceData.GridResolution;
	Resources->GridDensityResource.GridMaxCellCount = InResourceData.GridMaxCellCount;
	Resources->GridDensityResource.GridDepth = InResourceData.GridDepth;

	Resources->UniformParameters.GridDensityEnabled = InResourceData.GridResolution > 0 ? 1 : 0;
	Resources->UniformParameters.ColorOverDensityEnabled = InResourceData.bColorOverDensityEnabled;
	Resources->UniformParameters.SizeOverDensityEnabled = InResourceData.bSizeOverDensityEnabled;

	Resources->UniformParameters.DensityColorCurve = GParticleCurveTexture.ComputeCurveScaleBias(Resources->DensityColorTexelAllocation);
	Resources->UniformParameters.DensityColorScale = InResourceData.DensityColorScale;
	Resources->UniformParameters.DensityColorBias = InResourceData.DensityColorBias;

	Resources->UniformParameters.DensitySizeCurve = GParticleCurveTexture.ComputeCurveScaleBias(Resources->DensitySizeTexelAllocation);
	Resources->UniformParameters.DensitySizeScale.X = InResourceData.DensitySizeScale.X;
	Resources->UniformParameters.DensitySizeScale.Y = InResourceData.DensitySizeScale.Y;
	Resources->UniformParameters.DensitySizeBias.X = InResourceData.DensitySizeBias.X;
	Resources->UniformParameters.DensitySizeBias.Y = InResourceData.DensitySizeBias.Y;
	// NVCHANGE_END: JCAO - Grid Density with GPU particles

	// Setup tangent selector parameter.
	const EParticleAxisLock LockAxisFlag = (EParticleAxisLock)InResourceData.LockAxisFlag;
	const bool bRotationLock = (LockAxisFlag >= EPAL_ROTATE_X) && (LockAxisFlag <= EPAL_ROTATE_Z);

	Resources->UniformParameters.TangentSelector = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
	Resources->UniformParameters.RotationBias = 0.0f;

	if (InResourceData.ScreenAlignment == PSA_Velocity)
	{
		Resources->UniformParameters.TangentSelector.Y = 1;
	}
	else if(LockAxisFlag == EPAL_NONE )
	{
		if (InResourceData.ScreenAlignment == PSA_Square)
		{
			Resources->UniformParameters.TangentSelector.X = 1;
		}
		else if (InResourceData.ScreenAlignment == PSA_FacingCameraPosition)
		{
			Resources->UniformParameters.TangentSelector.W = 1;
		}
	}
	else
	{
		if ( bRotationLock )
		{
			Resources->UniformParameters.TangentSelector.Z = 1.0f;
		}
		else
		{
			Resources->UniformParameters.TangentSelector.X = 1.0f;
		}

		// For locked rotation about Z the particle should be rotated by 90 degrees.
		Resources->UniformParameters.RotationBias = (LockAxisFlag == EPAL_ROTATE_Z) ? (0.5f * PI) : 0.0f;
	}

	Resources->UniformParameters.RotationRateScale = InResourceData.RotationRateScale;
	Resources->UniformParameters.CameraMotionBlurAmount = InResourceData.CameraMotionBlurAmount;

	Resources->UniformParameters.PivotOffset = InResourceData.PivotOffset;

	Resources->SimulationParameters.AttributeCurve = GParticleCurveTexture.ComputeCurveScaleBias(Resources->SimulationAttrTexelAllocation);
	Resources->SimulationParameters.AttributeCurveScale = InResourceData.SimulationAttrCurveScale;
	Resources->SimulationParameters.AttributeCurveBias = InResourceData.SimulationAttrCurveBias;
	Resources->SimulationParameters.AttributeScale = FVector4(
		InResourceData.DragCoefficientScale,
		InResourceData.PerParticleVectorFieldScale,
		InResourceData.ResilienceScale,
		1.0f  // OrbitRandom
		);
	Resources->SimulationParameters.AttributeBias = FVector4(
		InResourceData.DragCoefficientBias,
		InResourceData.PerParticleVectorFieldBias,
		InResourceData.ResilienceBias,
		0.0f  // OrbitRandom
		);
	Resources->SimulationParameters.MiscCurve = Resources->UniformParameters.MiscCurve;
	Resources->SimulationParameters.MiscScale = Resources->UniformParameters.MiscScale;
	Resources->SimulationParameters.MiscBias = Resources->UniformParameters.MiscBias;
	Resources->SimulationParameters.Acceleration = InResourceData.ConstantAcceleration;
	Resources->SimulationParameters.OrbitOffsetBase = InResourceData.OrbitOffsetBase;
	Resources->SimulationParameters.OrbitOffsetRange = InResourceData.OrbitOffsetRange;
	Resources->SimulationParameters.OrbitFrequencyBase = InResourceData.OrbitFrequencyBase;
	Resources->SimulationParameters.OrbitFrequencyRange = InResourceData.OrbitFrequencyRange;
	Resources->SimulationParameters.OrbitPhaseBase = InResourceData.OrbitPhaseBase;
	Resources->SimulationParameters.OrbitPhaseRange = InResourceData.OrbitPhaseRange;
	Resources->SimulationParameters.CollisionRadiusScale = InResourceData.CollisionRadiusScale;
	Resources->SimulationParameters.CollisionRadiusBias = InResourceData.CollisionRadiusBias;
	Resources->SimulationParameters.CollisionTimeBias = InResourceData.CollisionTimeBias;
	Resources->SimulationParameters.OneMinusFriction = InResourceData.OneMinusFriction;
	Resources->EmitterSimulationResources.GlobalVectorFieldScale = InResourceData.GlobalVectorFieldScale;
	Resources->EmitterSimulationResources.GlobalVectorFieldTightness = InResourceData.GlobalVectorFieldTightness;
}

/**
 * Clears GPU sprite resource data.
 * @param Resources - Sprite resources to update.
 * @param InResourceData - Data with which to update resources.
 */
static void ClearGPUSpriteResourceData( FGPUSpriteResources* Resources )
{
	// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
	GParticleCurveTexture.RemoveCurve(Resources->DensitySizeTexelAllocation);
	GParticleCurveTexture.RemoveCurve(Resources->DensityColorTexelAllocation);
	// NVCHANGE_END: JCAO - Grid Density with GPU particles
	GParticleCurveTexture.RemoveCurve( Resources->ColorTexelAllocation );
	GParticleCurveTexture.RemoveCurve( Resources->MiscTexelAllocation );
	GParticleCurveTexture.RemoveCurve( Resources->SimulationAttrTexelAllocation );
}

FGPUSpriteResources* BeginCreateGPUSpriteResources( const FGPUSpriteResourceData& InResourceData )
{
	FGPUSpriteResources* Resources = NULL;
	if (RHISupportsGPUParticles(GMaxRHIFeatureLevel))
	{
		Resources = new FGPUSpriteResources;
		SetGPUSpriteResourceData( Resources, InResourceData );
		BeginInitResource( Resources );
	}
	return Resources;
}

void BeginUpdateGPUSpriteResources( FGPUSpriteResources* Resources, const FGPUSpriteResourceData& InResourceData )
{
	check( Resources );
	ClearGPUSpriteResourceData( Resources );
	SetGPUSpriteResourceData( Resources, InResourceData );
	BeginUpdateResourceRHI( Resources );
}

void BeginReleaseGPUSpriteResources( FGPUSpriteResources* Resources )
{
	if ( Resources )
	{
		ClearGPUSpriteResourceData( Resources );
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FReleaseGPUSpriteResourcesCommand,
			FGPUSpriteResources*, Resources, Resources,
		{
			Resources->ReleaseResource();
			delete Resources;
		});
	}
}
