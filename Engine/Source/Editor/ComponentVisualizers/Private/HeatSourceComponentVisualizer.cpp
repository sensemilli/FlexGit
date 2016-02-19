// NVCHANGE_BEGIN: JCAO - Using Component Visualizer to display the field sampler component

#include "ComponentVisualizersPrivatePCH.h"

#include "HeatSourceComponentVisualizer.h"


void FHeatSourceComponentVisualizer::DrawVisualization( const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI )
{
	const UHeatSourceComponent* HeatSourceComp = Cast<const UHeatSourceComponent>(Component);
	if(HeatSourceComp != NULL && HeatSourceComp->HeatSourceAsset != NULL)
	{
		FTransform TM = HeatSourceComp->ComponentToWorld;
		TM.RemoveScaling();

		if (HeatSourceComp->HeatSourceAsset->GeometryType == EGeometryType_Sphere)
		{
			float SphereRadius = HeatSourceComp->HeatSourceAsset->Radius;
			int32 SphereSides =  FMath::Clamp<int32>(SphereRadius/4.f, 16, 64);
			DrawCircle( PDI, TM.GetTranslation(), TM.GetScaledAxis( EAxis::X ), TM.GetScaledAxis( EAxis::Y ), FColor(200, 255, 255), SphereRadius, SphereSides, SDPG_World );
			DrawCircle( PDI, TM.GetTranslation(), TM.GetScaledAxis( EAxis::X ), TM.GetScaledAxis( EAxis::Z ), FColor(200, 255, 255), SphereRadius, SphereSides, SDPG_World );
			DrawCircle( PDI, TM.GetTranslation(), TM.GetScaledAxis( EAxis::Y ), TM.GetScaledAxis( EAxis::Z ), FColor(200, 255, 255), SphereRadius, SphereSides, SDPG_World );
		}
		else if (HeatSourceComp->HeatSourceAsset->GeometryType == EGeometryType_Box)
		{
			FTransform TM = HeatSourceComp->ComponentToWorld;
			FVector BoxExtents = HeatSourceComp->HeatSourceAsset->Extents;
			DrawOrientedWireBox(PDI, TM.GetTranslation(), TM.GetScaledAxis( EAxis::X ), TM.GetScaledAxis( EAxis::Y ), TM.GetScaledAxis( EAxis::Z ), BoxExtents, FColor(200, 255, 255), SDPG_World);
		}
	}
}
// NVCHANGE_END: JCAO - Using Component Visualizer to display the field sampler component
