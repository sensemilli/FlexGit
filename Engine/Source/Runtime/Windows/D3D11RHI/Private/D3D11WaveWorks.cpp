// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	D3D11WaveWorks.cpp: D3D WaveWorks RHI implementation.
=============================================================================*/

#include "D3D11RHIPrivate.h"
#include "GFSDK_WaveWorks.h"

DECLARE_LOG_CATEGORY_EXTERN(LogD3D11WaveWorks, Log, All);
DEFINE_LOG_CATEGORY(LogD3D11WaveWorks);
namespace
{
	FString Platform = PLATFORM_64BITS ? TEXT("win64") : TEXT("win32");
	FString WaveWorksBinariesDir = FPaths::EngineDir() / TEXT("Binaries/ThirdParty/WaveWorks/") + Platform;
	FString WaveWorksDLLName = FString("gfsdk_waveworks") + (UE_BUILD_DEBUG ? "_debug." : ".") + Platform + ".dll";
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
		}		
	}

	virtual void SetRenderState(const FMatrix ViewMatrix, const TArray<uint32>& ShaderInputMappings)
	{
		if(Simulation)
		{
			// GFSDK_WaveWorks_Simulation_AdvanceStagingCursorD3D11(Simulation, true, DeviceContext, NULL);
			GFSDK_WaveWorks_Simulation_SetRenderStateD3D11(Simulation, DeviceContext,
				reinterpret_cast<const gfsdk_float4x4&>(ViewMatrix), ShaderInputMappings.GetData(), NULL);
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

	TArray<WaveWorksShaderInput> ShaderInput = InitializeShaderInput();
}

const TArray<WaveWorksShaderInput>& FD3D11DynamicRHI::RHIGetWaveWorksShaderInput()
{
	return ShaderInput;
}
