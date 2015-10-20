// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
WaveWorksRendering.cpp: WaveWorks rendering
=============================================================================*/

#pragma once

#include "RendererPrivate.h"
#include "GFSDK_WaveWorks.h"
#include "StringConv.h"

// Determines ShaderInputMappings based on ParameterMap
void FWaveWorksShaderParameters::Bind(const FShaderParameterMap& ParameterMap, EShaderFrequency Frequency, EShaderParameterFlags Flags)
{
	const TArray<WaveWorksShaderInput>& ShaderInput = GDynamicRHI->RHIGetDefaultContext()->RHIGetWaveWorksShaderInput();

	uint32 Count = ShaderInput.Num();
	ShaderInputMappings.Empty(Count);
	uint32 NumFound = 0;
	for (uint32 Index = 0; Index < Count; ++Index)
	{
		uint32 InputMapping = GFSDK_WaveWorks_UnusedShaderInputRegisterMapping;
		if (Frequency == ShaderInput[Index].Frequency)
		{
			uint16 BufferIndex = 0;
			uint16 BaseIndex = 0;
			uint16 NumBytes = 0;

			const TCHAR* Name = ANSI_TO_TCHAR(ShaderInput[Index].Name.GetPlainANSIString());
			if (ParameterMap.FindParameterAllocation(Name, BufferIndex, BaseIndex, NumBytes))
			{
				++NumFound;
				check(BufferIndex == 0 || BaseIndex == 0);
				InputMapping = BufferIndex + BaseIndex;
			}
		}

		ShaderInputMappings.Push(InputMapping);
	}

	bIsBound = NumFound > 0;

	if (!bIsBound && Flags == SPF_Mandatory)
	{
		if (!UE_LOG_ACTIVE(LogShaders, Log))
		{
			UE_LOG(LogShaders, Fatal, TEXT("Failure to bind non-optional WaveWorks shader resources!  The parameters are either not present in the shader, or the shader compiler optimized it out."));
		}
		else
		{
			// We use a non-Slate message box to avoid problem where we haven't compiled the shaders for Slate.
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, *(NSLOCTEXT("UnrealEd", "Error_FailedToBindShaderParameter",
				"Failure to bind non-optional WaveWorks shader resources! The parameter is either not present in the shader, or the shader compiler optimized it out. This will be an assert with LogShaders suppressed!").ToString()), TEXT("Warning"));
		}
	}

	const TArray<WaveWorksShaderInput>& QuadTreeShaderInput = GDynamicRHI->RHIGetDefaultContext()->RHIGetWaveWorksQuadTreeShaderInput();

	Count = QuadTreeShaderInput.Num();
	QuadTreeShaderInputMappings.Empty(Count);
	NumFound = 0;
	for (uint32 Index = 0; Index < Count; ++Index)
	{
		uint32 InputMapping = GFSDK_WaveWorks_UnusedShaderInputRegisterMapping;
		if (Frequency == QuadTreeShaderInput[Index].Frequency)
		{
			uint16 BufferIndex = 0;
			uint16 BaseIndex = 0;
			uint16 NumBytes = 0;

			const TCHAR* Name = ANSI_TO_TCHAR(QuadTreeShaderInput[Index].Name.GetPlainANSIString());
			if (ParameterMap.FindParameterAllocation(Name, BufferIndex, BaseIndex, NumBytes))
			{
				++NumFound;
				check(BufferIndex == 0 || BaseIndex == 0);
				InputMapping = BufferIndex + BaseIndex;
			}
		}

		QuadTreeShaderInputMappings.Push(InputMapping);
	}

	//bIsBound = NumFound > 0;

	if (!bIsBound && Flags == SPF_Mandatory)
	{
		if (!UE_LOG_ACTIVE(LogShaders, Log))
		{
			UE_LOG(LogShaders, Fatal, TEXT("Failure to bind non-optional WaveWorks shader resources!  The parameters are either not present in the shader, or the shader compiler optimized it out."));
		}
		else
		{
			// We use a non-Slate message box to avoid problem where we haven't compiled the shaders for Slate.
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, *(NSLOCTEXT("UnrealEd", "Error_FailedToBindShaderParameter",
				"Failure to bind non-optional WaveWorks shader resources! The parameter is either not present in the shader, or the shader compiler optimized it out. This will be an assert with LogShaders suppressed!").ToString()), TEXT("Warning"));
		}
	}
}

void FWaveWorksShaderParameters::Serialize(FArchive& Ar)
{
	Ar << bIsBound << ShaderInputMappings << QuadTreeShaderInputMappings;
}
