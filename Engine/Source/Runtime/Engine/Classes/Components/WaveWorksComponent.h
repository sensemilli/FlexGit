// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
* WaveWorks
* WaveWorks component for QuadTree drawing
*/

#pragma once
#include "WaveWorksComponent.generated.h"

UCLASS(ClassGroup = Rendering, meta = (BlueprintSpawnableComponent), HideCategories = (Collision, Base, Object, PhysicsVolume))
class ENGINE_API UWaveWorksComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:

	/* Default constructor */
	UWaveWorksComponent(const class FObjectInitializer& ObjectInitializer);

	/* Begin UPrimitiveComponent interface */
	virtual class FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	/* End UPrimitiveComponent interface */

	UFUNCTION(BlueprintCallable, Category="WaveWorks")
	virtual void SampleDisplacements(TArray<FVector2D> InSamplePoints, TArray<FVector4>& OutDisplacements);

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WaveWorks")
	class UMaterialInterface* WaveWorksMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks)
	int32 MeshDim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks)
	float MinPatchLength;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks)
	int32 AutoRootLOD;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks)
	float UpperGridCoverage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks)
	float SeaLevel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks)
	uint32 UseTessellation : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks)
	float TessellationLOD;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks)
	float GeoMorphingDegree;
};