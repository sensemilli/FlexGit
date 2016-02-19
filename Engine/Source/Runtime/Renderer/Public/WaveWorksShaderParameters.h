// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WaveWorksRendering.h: WaveWorks rendering
=============================================================================*/

#pragma once

/** Shader parameters needed for WaveWorks. */
class FWaveWorksShaderParameters
{
public:
	FWaveWorksShaderParameters() 
	:	bIsBound(false)
	{}

	void Bind(const FShaderParameterMap& ParameterMap, EShaderFrequency Frequency, EShaderParameterFlags Flags = SPF_Optional);

	template<typename ShaderRHIParamRef>
	void Set(FRHICommandList& RHICmdList, const ShaderRHIParamRef ShaderRHI, const FSceneView& View, FWaveWorksRHIRef WaveWorksRHI) const
	{
		if (bIsBound && WaveWorksRHI)
		{
			RHICmdList.SetWaveWorksState(WaveWorksRHI, View.ViewMatrices.ViewMatrix, ShaderInputMappings);
		}
	}

	friend FArchive& operator<<(FArchive& Ar, FWaveWorksShaderParameters& Parameters)
	{
		Parameters.Serialize(Ar);
		return Ar;
	}

	bool IsBound() const { return bIsBound; }

	void Serialize(FArchive& Ar);

public:
	bool bIsBound;
	// mapping of WaveWorks shader input (see RHIGetWaveWorksShaderInput()) to resource slot
	TArray<uint32> ShaderInputMappings;
	TArray<uint32> QuadTreeShaderInputMappings;
};
