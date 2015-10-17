// NVCHANGE_BEGIN: JCAO - Using Component Visualizer to display the field sampler component

#include "ComponentVisualizersPrivatePCH.h"

#include "VelocitySourceComponentVisualizer.h"


void FVelocitySourceComponentVisualizer::DrawVisualization( const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI )
{
	const UVelocitySourceComponent* VelocitySourceComp = Cast<const UVelocitySourceComponent>(Component);
	if(VelocitySourceComp != NULL && VelocitySourceComp->VelocitySourceAsset != NULL)
	{
		FTransform TM = VelocitySourceComp->ComponentToWorld;
		TM.RemoveScaling();

		if (VelocitySourceComp->VelocitySourceAsset->GeometryType == EGeometryType_Sphere)
		{
			float SphereRadius = VelocitySourceComp->VelocitySourceAsset->Radius;
			int32 SphereSides =  FMath::Clamp<int32>(SphereRadius/4.f, 16, 64);
			DrawCircle( PDI, TM.GetTranslation(), TM.GetScaledAxis( EAxis::X ), TM.GetScaledAxis( EAxis::Y ), FColor(200, 255, 255), SphereRadius, SphereSides, SDPG_World );
			DrawCircle( PDI, TM.GetTranslation(), TM.GetScaledAxis( EAxis::X ), TM.GetScaledAxis( EAxis::Z ), FColor(200, 255, 255), SphereRadius, SphereSides, SDPG_World );
			DrawCircle( PDI, TM.GetTranslation(), TM.GetScaledAxis( EAxis::Y ), TM.GetScaledAxis( EAxis::Z ), FColor(200, 255, 255), SphereRadius, SphereSides, SDPG_World );
		}
		else if (VelocitySourceComp->VelocitySourceAsset->GeometryType == EGeometryType_Box)
		{
			FTransform TM = VelocitySourceComp->ComponentToWorld;
			FVector BoxExtents = VelocitySourceComp->VelocitySourceAsset->Extents;
			DrawOrientedWireBox(PDI, TM.GetTranslation(), TM.GetScaledAxis( EAxis::X ), TM.GetScaledAxis( EAxis::Y ), TM.GetScaledAxis( EAxis::Z ), BoxExtents, FColor(200, 255, 255), SDPG_World);
		}
	}
}
// NVCHANGE_END: JCAO - Using Component Visualizer to display the field sampler component
