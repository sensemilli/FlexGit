// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	D3D11WaveWorks.cpp: D3D WaveWorks RHI implementation.
=============================================================================*/

#include "D3D11RHIPrivate.h"
#include "GFSDK_WaveWorks.h"

DECLARE_LOG_CATEGORY_EXTERN(LogD3D11WaveWorks, Log, All);
DEFINE_LOG_CATEGORY(LogD3D11WaveWorks);

DECLARE_FLOAT_COUNTER_STAT_EXTERN(TEXT("Simulation CPU Main thread wait time"), STAT_WaveWorksD3D11SimulationWaitTime, STATGROUP_WaveWorksD3D11, );
DECLARE_FLOAT_COUNTER_STAT_EXTERN(TEXT("Simulation CPU Threads start to finish time"), STAT_WaveWorksD3D11SimulationStartFinishTime, STATGROUP_WaveWorksD3D11, );
DECLARE_FLOAT_COUNTER_STAT_EXTERN(TEXT("Simulation CPU Threads total time"), STAT_WaveWorksD3D11TotalTime, STATGROUP_WaveWorksD3D11, );
DECLARE_FLOAT_COUNTER_STAT_EXTERN(TEXT("Simulation GPU Simulation time"), STAT_WaveWorksD3D11GPUSimulationTime, STATGROUP_WaveWorksD3D11, );
DECLARE_FLOAT_COUNTER_STAT_EXTERN(TEXT("Simulation GPU FFT Simulation time"), STAT_WaveWorksD3D11GPUFFTSimulationTime, STATGROUP_WaveWorksD3D11, );
DECLARE_FLOAT_COUNTER_STAT_EXTERN(TEXT("Simulation GPU GFX Time"), STAT_WaveWorksD3D11GPUGFXTime, STATGROUP_WaveWorksD3D11, );
DECLARE_FLOAT_COUNTER_STAT_EXTERN(TEXT("Simulation GPU Update time"), STAT_WaveWorksD3D11GPUUpdateTime, STATGROUP_WaveWorksD3D11, );

DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Quadtree Patches drawn"), STAT_WaveWorksD3D11QuadtreePatchesDrawn, STATGROUP_WaveWorksD3D11, );
DECLARE_FLOAT_COUNTER_STAT_EXTERN(TEXT("Quadtree CPU Update time"), STAT_WaveWorksD3D11QuadtreeUpdateTime, STATGROUP_WaveWorksD3D11, );

DEFINE_STAT(STAT_WaveWorksD3D11SimulationWaitTime);
DEFINE_STAT(STAT_WaveWorksD3D11SimulationStartFinishTime);
DEFINE_STAT(STAT_WaveWorksD3D11TotalTime);
DEFINE_STAT(STAT_WaveWorksD3D11GPUSimulationTime);
DEFINE_STAT(STAT_WaveWorksD3D11GPUFFTSimulationTime);
DEFINE_STAT(STAT_WaveWorksD3D11GPUGFXTime);
DEFINE_STAT(STAT_WaveWorksD3D11GPUUpdateTime);

DEFINE_STAT(STAT_WaveWorksD3D11QuadtreePatchesDrawn);
DEFINE_STAT(STAT_WaveWorksD3D11QuadtreeUpdateTime);

namespace
{
	FString Platform = PLATFORM_64BITS ? TEXT("win64") : TEXT("win32");
	FString WaveWorksBinariesDir = FPaths::EngineDir() / TEXT("Binaries/ThirdParty/WaveWorks/") + Platform;
	FString WaveWorksDLLName = FString("gfsdk_waveworks") + /*(UE_BUILD_DEBUG ? "_debug." : ".") +*/ "." + Platform + ".dll";
	FString CuFFTDLLName = FString("cufft") + Platform.Right(2) + "_55.dll";

	struct DllHandle
	{
		DllHandle(const FString Name)
		{
			FWindowsPlatformProcess::PushDllDirectory(*WaveWorksBinariesDir);
			Handle = FPlatformProcess::GetDllHandle(*Name);
			FWindowsPlatformProcess::PopDllDirectory(*WaveWorksBinariesDir);
		}
		~DllHandle()
		{
			FPlatformProcess::FreeDllHandle(Handle);
		}
		void* Handle;
	};
}

class FD3D11WaveWorks : public FRHIWaveWorks
{
public:
	FD3D11WaveWorks(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext,
		const struct GFSDK_WaveWorks_Simulation_Settings& Settings,
		const struct GFSDK_WaveWorks_Simulation_Params& Params)
		: WaveWorksDllHandle(WaveWorksDLLName)
		, CuFFTDllHandle(CuFFTDLLName)
		, Device(Device)
		, DeviceContext(DeviceContext)
	{
		GFSDK_WaveWorks_InitD3D11(Device, nullptr, GFSDK_WAVEWORKS_API_GUID);
		GFSDK_WaveWorks_Savestate_CreateD3D11(GFSDK_WaveWorks_StatePreserve_All, Device, &SaveState);
		GFSDK_WaveWorks_Simulation_CreateD3D11(Settings, Params, Device, &Simulation);
		if(Simulation == NULL)
		{
			UE_LOG(LogD3D11WaveWorks, Warning, TEXT("FD3D11WaveWorks Simulation_CreateD3D11 FAIL"));
		}
	}		

	~FD3D11WaveWorks()
	{
		GFSDK_WaveWorks_Simulation_Destroy(Simulation);
		GFSDK_WaveWorks_Savestate_Destroy(SaveState);
		GFSDK_WaveWorks_ReleaseD3D11(Device);
	}
	
	virtual void UpdateTick()
	{
		if(Simulation)
		{		
			do {
				GFSDK_WaveWorks_Simulation_KickD3D11(Simulation, NULL, DeviceContext, SaveState);
			} while (GFSDK_WaveWorks_Simulation_GetStagingCursor(Simulation, NULL) == gfsdk_waveworks_result_NONE);
			GFSDK_WaveWorks_Savestate_RestoreD3D11(SaveState, DeviceContext);

#if WITH_EDITOR
			GFSDK_WaveWorks_Simulation_Stats Stats;
			GFSDK_WaveWorks_Simulation_GetStats(Simulation, Stats);

			SET_FLOAT_STAT(STAT_WaveWorksD3D11SimulationWaitTime, Stats.CPU_main_thread_wait_time);
			SET_FLOAT_STAT(STAT_WaveWorksD3D11SimulationStartFinishTime, Stats.CPU_threads_start_to_finish_time);
			SET_FLOAT_STAT(STAT_WaveWorksD3D11TotalTime, Stats.CPU_threads_total_time);
			SET_FLOAT_STAT(STAT_WaveWorksD3D11GPUSimulationTime, Stats.GPU_simulation_time);
			SET_FLOAT_STAT(STAT_WaveWorksD3D11GPUFFTSimulationTime, Stats.GPU_FFT_simulation_time);
			SET_FLOAT_STAT(STAT_WaveWorksD3D11GPUGFXTime, Stats.GPU_gfx_time);
			SET_FLOAT_STAT(STAT_WaveWorksD3D11GPUUpdateTime, Stats.GPU_update_time);
#endif
		}		
	}

	virtual void SetRenderState(const FMatrix ViewMatrix, const TArray<uint32>& ShaderInputMappings)
	{
		if(Simulation)
		{
			GFSDK_WaveWorks_Simulation_SetRenderStateD3D11(Simulation, DeviceContext,
				reinterpret_cast<const gfsdk_float4x4&>(ViewMatrix), ShaderInputMappings.GetData(), NULL);
		}
	}

	virtual void CreateQuadTree(GFSDK_WaveWorks_Quadtree** OutWaveWorksQuadTreeHandle, int32 MeshDim, float MinPatchLength, uint32 AutoRootLOD, float UpperGridCoverage, float SeaLevel, bool UseTessellation, float TessellationLOD, float GeoMoprhingDegree)
	{
		GFSDK_WaveWorks_Quadtree_Params Params;
		Params.mesh_dim = MeshDim;
		Params.min_patch_length = MinPatchLength;
		Params.patch_origin = { 0.0f, 0.0f };
		Params.auto_root_lod = AutoRootLOD;
		Params.upper_grid_coverage = UpperGridCoverage;
		Params.sea_level = SeaLevel;
		Params.use_tessellation = UseTessellation;
		Params.tessellation_lod = TessellationLOD;
		Params.geomorphing_degree = GeoMoprhingDegree;

		gfsdk_waveworks_result Result = GFSDK_WaveWorks_Quadtree_CreateD3D11(Params, Device, OutWaveWorksQuadTreeHandle);
		if (Result != gfsdk_waveworks_result_OK)
		{
			UE_LOG(LogD3D11RHI, Error, TEXT("WaveWorks: Failed to create QuadTree"));
			*OutWaveWorksQuadTreeHandle = nullptr;
		}
	}

	virtual void DrawQuadTree(GFSDK_WaveWorks_Quadtree* WaveWorksQuadTreeHandle, FMatrix ViewMatrix, FMatrix ProjMatrix, const TArray<uint32>& ShaderInputMappings)
	{
		GFSDK_WaveWorks_SavestateHandle QuadTreeSaveState;
		GFSDK_WaveWorks_Savestate_CreateD3D11(GFSDK_WaveWorks_StatePreserve_All, Device, &QuadTreeSaveState);

		FD3D11DynamicRHI* D3D11RHI = static_cast<FD3D11DynamicRHI*>(GDynamicRHI);
		D3D11RHI->SetWaveWorksState();

		GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(
			WaveWorksQuadTreeHandle,
			GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(Simulation)
			);

		gfsdk_waveworks_result Result = GFSDK_WaveWorks_Quadtree_DrawD3D11(
			WaveWorksQuadTreeHandle,
			DeviceContext,
			reinterpret_cast<const gfsdk_float4x4&>(ViewMatrix),
			reinterpret_cast<const gfsdk_float4x4&>(ProjMatrix),
			ShaderInputMappings.GetData(),
			QuadTreeSaveState
			);

		if (Result != gfsdk_waveworks_result_OK)
		{
			UE_LOG(LogD3D11RHI, Error, TEXT("WaveWorks: Failed to Draw QuadTree"));
			return;
		}

#if WITH_EDITOR
		GFSDK_WaveWorks_Quadtree_Stats Stats;
		GFSDK_WaveWorks_Quadtree_GetStats(WaveWorksQuadTreeHandle, Stats);

		SET_DWORD_STAT(STAT_WaveWorksD3D11QuadtreePatchesDrawn, Stats.num_patches_drawn);
		SET_FLOAT_STAT(STAT_WaveWorksD3D11QuadtreeUpdateTime, Stats.CPU_quadtree_update_time);
#endif

		GFSDK_WaveWorks_Savestate_RestoreD3D11(QuadTreeSaveState, DeviceContext);
		GFSDK_WaveWorks_Savestate_Destroy(QuadTreeSaveState);
	}

	virtual void DestroyQuadTree(GFSDK_WaveWorks_Quadtree* WaveWorksQuadTreeHandle)
	{
		GFSDK_WaveWorks_Quadtree_Destroy(WaveWorksQuadTreeHandle);
	}

	virtual void GetDisplacements(TArray<FVector2D> InSamplePoints, TArray<FVector4>& OutDisplacements)
	{
		if (Simulation)
		{
			OutDisplacements.AddUninitialized(InSamplePoints.Num());
			gfsdk_waveworks_result Result = GFSDK_WaveWorks_Simulation_GetDisplacements(
				Simulation,
				reinterpret_cast<gfsdk_float2*>(InSamplePoints.GetData()),
				reinterpret_cast<gfsdk_float4*>(OutDisplacements.GetData()),
				InSamplePoints.Num()
				);
		}
	}

private:
	DllHandle WaveWorksDllHandle;
	DllHandle CuFFTDllHandle;
	ID3D11Device* Device;
	ID3D11DeviceContext* DeviceContext;
	GFSDK_WaveWorks_SavestateHandle SaveState;
};

FWaveWorksRHIRef FD3D11DynamicRHI::RHICreateWaveWorks(
	const struct GFSDK_WaveWorks_Simulation_Settings& Settings,
	const struct GFSDK_WaveWorks_Simulation_Params& Params)
{
	return new FD3D11WaveWorks(GetDevice(), GetDeviceContext(), Settings, Params);
}

namespace
{
	TArray<WaveWorksShaderInput> InitializeShaderInput()
	{
		DllHandle WaveWorksDllHandle(WaveWorksDLLName);

		// maps GFSDK_WaveWorks_ShaderInput_Desc::InputType to EShaderFrequency
		EShaderFrequency TypeToFrequencyMap[] =
		{
			SF_Vertex,
			SF_Vertex,
			SF_Vertex,
			SF_Vertex,
			SF_Hull,
			SF_Hull,
			SF_Hull,
			SF_Hull,
			SF_Domain,
			SF_Domain,
			SF_Domain,
			SF_Domain,
			SF_Pixel,
			SF_Pixel,
			SF_Pixel,
			SF_Pixel
		};

		// maps GFSDK_WaveWorks_ShaderInput_Desc::InputType to ERHIResourceType
		ERHIResourceType TypeToResourceMap[] =
		{
			RRT_None,
			RRT_UniformBuffer,
			RRT_ShaderResourceView,
			RRT_SamplerState,
			RRT_None,
			RRT_UniformBuffer,
			RRT_ShaderResourceView,
			RRT_SamplerState,
			RRT_None,
			RRT_UniformBuffer,
			RRT_ShaderResourceView,
			RRT_SamplerState,
			RRT_None,
			RRT_UniformBuffer,
			RRT_ShaderResourceView,
			RRT_SamplerState,
		};

		uint32 Count = GFSDK_WaveWorks_Simulation_GetShaderInputCountD3D11();
		TArray<WaveWorksShaderInput> Result;
		for (uint32 i = 0; i<Count; ++i)
		{
			GFSDK_WaveWorks_ShaderInput_Desc Desc;
			GFSDK_WaveWorks_Simulation_GetShaderInputDescD3D11(i, &Desc);
			check(Desc.Type < ARRAY_COUNT(TypeToFrequencyMap));
			check(Desc.Type < ARRAY_COUNT(TypeToResourceMap));
			WaveWorksShaderInput Input = { 
				TypeToFrequencyMap[Desc.Type], 
				TypeToResourceMap[Desc.Type], 
				Desc.Name, 
			};
			Result.Push(Input);
		}

		return Result;
	}
	TArray<WaveWorksShaderInput> InitializeQuadTreeShaderInput()
	{
		DllHandle WaveWorksDllHandle(WaveWorksDLLName);

		// maps GFSDK_WaveWorks_ShaderInput_Desc::InputType to EShaderFrequency
		EShaderFrequency TypeToFrequencyMap[] =
		{
			SF_Vertex,
			SF_Vertex,
			SF_Vertex,
			SF_Vertex,
			SF_Hull,
			SF_Hull,
			SF_Hull,
			SF_Hull,
			SF_Domain,
			SF_Domain,
			SF_Domain,
			SF_Domain,
			SF_Pixel,
			SF_Pixel,
			SF_Pixel,
			SF_Pixel
		};

		// maps GFSDK_WaveWorks_ShaderInput_Desc::InputType to ERHIResourceType
		ERHIResourceType TypeToResourceMap[] =
		{
			RRT_None,
			RRT_UniformBuffer,
			RRT_ShaderResourceView,
			RRT_SamplerState,
			RRT_None,
			RRT_UniformBuffer,
			RRT_ShaderResourceView,
			RRT_SamplerState,
			RRT_None,
			RRT_UniformBuffer,
			RRT_ShaderResourceView,
			RRT_SamplerState,
			RRT_None,
			RRT_UniformBuffer,
			RRT_ShaderResourceView,
			RRT_SamplerState,
		};

		uint32 Count = GFSDK_WaveWorks_Quadtree_GetShaderInputCountD3D11();
		TArray<WaveWorksShaderInput> Result;
		for (uint32 i = 0; i < Count; ++i)
		{
			GFSDK_WaveWorks_ShaderInput_Desc Desc;
			GFSDK_WaveWorks_Quadtree_GetShaderInputDescD3D11(i, &Desc);
			check(Desc.Type < ARRAY_COUNT(TypeToFrequencyMap));
			check(Desc.Type < ARRAY_COUNT(TypeToResourceMap));
			WaveWorksShaderInput Input = {
				TypeToFrequencyMap[Desc.Type],
				TypeToResourceMap[Desc.Type],
				Desc.Name,
			};
			Result.Push(Input);
		}

		return Result;
	}

	TArray<WaveWorksShaderInput> ShaderInput = InitializeShaderInput();
	TArray<WaveWorksShaderInput> QuadTreeShaderInput = InitializeQuadTreeShaderInput();
}

const TArray<WaveWorksShaderInput>& FD3D11DynamicRHI::RHIGetWaveWorksShaderInput()
{
	return ShaderInput;
}

const TArray<WaveWorksShaderInput>& FD3D11DynamicRHI::RHIGetWaveWorksQuadTreeShaderInput()
{
	return QuadTreeShaderInput;
}
