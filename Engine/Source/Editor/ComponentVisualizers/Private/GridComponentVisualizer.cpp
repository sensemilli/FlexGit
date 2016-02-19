// NVCHANGE_BEGIN: JCAO - Using Component Visualizer to display the field sampler component

#include "ComponentVisualizersPrivatePCH.h"

#include "GridComponentVisualizer.h"


void FGridComponentVisualizer::DrawVisualization( const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI )
{
	const UGridComponent* GridComp = Cast<const UGridComponent>(Component);
	if(GridComp != NULL && GridComp->GridAsset != NULL)
	{
		FTransform TM = GridComp->ComponentToWorld;
		FVector BoxExtents = GridComp->GridAsset->GridSize3D * (GridComp->GridAsset->GridScale * 0.5f);
		DrawOrientedWireBox(PDI, TM.GetTranslation(), TM.GetScaledAxis( EAxis::X ), TM.GetScaledAxis( EAxis::Y ), TM.GetScaledAxis( EAxis::Z ), BoxExtents, FColor(200, 255, 255), SDPG_World);
	}
}
// NVCHANGE_END: JCAO - Using Component Visualizer to display the field sampler component
