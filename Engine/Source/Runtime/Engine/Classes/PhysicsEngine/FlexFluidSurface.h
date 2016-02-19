// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "FlexFluidSurface.generated.h"

UCLASS(hidecategories = Object, MinimalAPI, BlueprintType)
class UFlexFluidSurface : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = Flex, meta = (DisplayName = "Smoothing Radius", UIMin = "0.0"))
	float SmoothingRadius;

	UPROPERTY(EditAnywhere, Category = Flex, meta = (DisplayName = "Max Radial Samples", UIMin = "0"))
	int32 MaxRadialSamples;

	UPROPERTY(EditAnywhere, Category = Flex, meta = (DisplayName = "Depth Edge Falloff", UIMin = "0.0"))
	float DepthEdgeFalloff;

	/* Relative scale applied to particles for thickness rendering. Higher values result in smoother thickness, but can reduce definition.
	A value of 0.0 disables thickness rendering. Default is 2.0*/
	UPROPERTY(EditAnywhere, Category = Flex, meta = (DisplayName = "Thickness Particle Scale", UIMin = "0.0"))
	float ThicknessParticleScale;

	/* Relative scale applied to particles for depth rendering. Default is 1.0*/
	UPROPERTY(EditAnywhere, Category = Flex, meta = (DisplayName = "Depth Particle Scale", UIMin = "0.0"))
	float DepthParticleScale;

	/** Enables shadowing from static geometry*/
	UPROPERTY(EditAnywhere, Category = Flex)
	bool ReceiveShadows;

	/* Material used to render the surface*/
	UPROPERTY(EditAnywhere, Category = Flex)
	UMaterial* Material;

public:
	// Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	// End UObject Interface
};



