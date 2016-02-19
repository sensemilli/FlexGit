// NVCHANGE_BEGIN: JCAO - Using Component Visualizer to display the field sampler component

#include "ComponentVisualizersPrivatePCH.h"

#include "NoiseComponentVisualizer.h"


void FNoiseComponentVisualizer::DrawVisualization( const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI )
{
	const UNoiseComponent* NoiseComp = Cast<const UNoiseComponent>(Component);
	if(NoiseComp != NULL && NoiseComp->NoiseAsset != NULL)
	{
		FTransform TM = NoiseComp->ComponentToWorld;
		FVector BoxExtents = NoiseComp->NoiseAsset->BoundarySize * (NoiseComp->NoiseAsset->BoundaryScale * 0.5f);
		DrawOrientedWireBox(PDI, TM.GetTranslation(), TM.GetScaledAxis( EAxis::X ), TM.GetScaledAxis( EAxis::Y ), TM.GetScaledAxis( EAxis::Z ), BoxExtents, FColor(200, 255, 255), SDPG_World);
	}
}
// NVCHANGE_END: JCAO - Using Component Visualizer to display the field sampler component
