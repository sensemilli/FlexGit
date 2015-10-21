// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
WaveWorksComponent.cpp: UWaveWorksComponent implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "GFSDK_WaveWorks.h"
#include "WaveWorksRender.h"
#include "Materials/MaterialExpressionWaveWorks.h"
#include "WaveWorksResource.h"

/*-----------------------------------------------------------------------------
UWaveWorksComponent
-----------------------------------------------------------------------------*/

UWaveWorksComponent::UWaveWorksComponent(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	MeshDim = 128;
	MinPatchLength = 128.0f;
	AutoRootLOD = 20;
	UpperGridCoverage = 1.0f;
	SeaLevel = 0.0f;
	UseTessellation = true;
	TessellationLOD = 2.0f;
	GeoMorphingDegree = 0.0f;
}

FPrimitiveSceneProxy* UWaveWorksComponent::CreateSceneProxy()
{
	if (WaveWorksMaterial != nullptr)
	{
		TArray<const UMaterialExpressionWaveWorks*> WaveWorksExpressions;
		WaveWorksMaterial->GetMaterial()->GetAllExpressionsOfType<UMaterialExpressionWaveWorks>(WaveWorksExpressions);

		if (WaveWorksExpressions.Num() > 0)
		{
			const UMaterialExpressionWaveWorks* WaveWorksExpression = WaveWorksExpressions[0];
			UWaveWorks* WaveWorksAsset = WaveWorksExpression->WaveWorks;

			if (WaveWorksAsset->GetWaveWorksResource() && WaveWorksAsset->GetWaveWorksResource()->GetWaveWorksRHI())
			{
				return new FWaveWorksSceneProxy(this, WaveWorksExpression->WaveWorks);
			}
		}
	}

	return nullptr;
}

FBoxSphereBounds UWaveWorksComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds NewBounds;
	NewBounds.Origin = FVector::ZeroVector;
	NewBounds.BoxExtent = FVector(HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX);
	NewBounds.SphereRadius = FMath::Sqrt(3.0f * FMath::Square(HALF_WORLD_MAX));
	return NewBounds;
}