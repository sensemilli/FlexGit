// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionWaveWorks.generated.h"

UCLASS(collapsecategories, hidecategories=Object)
class UMaterialExpressionWaveWorks : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=MaterialExpressionWaveWorks)
	class UWaveWorks* WaveWorks;

	UPROPERTY()
	FExpressionInput Coordinates;

	UPROPERTY(EditAnywhere, Category = MaterialExpressionWaveWorks)
	float DistanceScale;

	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	// End UMaterialExpression Interface
};
