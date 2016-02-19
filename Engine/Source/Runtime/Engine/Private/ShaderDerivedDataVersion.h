// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ShaderDerivedDataVersion.h: Shader derived data version.
=============================================================================*/

#pragma once

// In case of merge conflicts with DDC versions, you MUST generate a new GUID and set this new
// guid as version

// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI

//Our FShaderResource class is not compatible with this define on
#define GLOBALSHADERMAP_DERIVEDDATA_VER			TEXT("6b89d48045c847d9b06bf15e0a9ee14b")
#define MATERIALSHADERMAP_DERIVEDDATA_VER		TEXT("ef708e23e35941b4b650b73d73b1a6cb")

#else
// NVCHANGE_END: Add VXGI

#define GLOBALSHADERMAP_DERIVEDDATA_VER			TEXT("af1606fcec114373844d2b6cf9df71c8")
#define MATERIALSHADERMAP_DERIVEDDATA_VER		TEXT("ac5a4b716cd4426c830db487880c8fb0")

// NVCHANGE_BEGIN: Add VXGI
#endif
// NVCHANGE_END: Add VXGI
