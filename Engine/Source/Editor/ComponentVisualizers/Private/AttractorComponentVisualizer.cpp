// NVCHANGE_BEGIN: JCAO - Using Component Visualizer to display the field sampler component

#include "ComponentVisualizersPrivatePCH.h"

#include "AttractorComponentVisualizer.h"


void FAttractorComponentVisualizer::DrawVisualization( const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI )
{
	const UAttractorComponent* AttractorComp = Cast<const UAttractorComponent>(Component);
	if(AttractorComp != NULL && AttractorComp->AttractorAsset != NULL)
	{
		FTransform TM = AttractorComp->ComponentToWorld;
		TM.RemoveScaling();
		float SphereRadius = AttractorComp->AttractorAsset->Radius;
		int32 SphereSides =  FMath::Clamp<int32>(SphereRadius/4.f, 16, 64);
		DrawCircle( PDI, TM.GetTranslation(), TM.GetScaledAxis( EAxis::X ), TM.GetScaledAxis( EAxis::Y ), FColor(200, 255, 255), SphereRadius, SphereSides, SDPG_World );
		DrawCircle( PDI, TM.GetTranslation(), TM.GetScaledAxis( EAxis::X ), TM.GetScaledAxis( EAxis::Z ), FColor(200, 255, 255), SphereRadius, SphereSides, SDPG_World );
		DrawCircle( PDI, TM.GetTranslation(), TM.GetScaledAxis( EAxis::Y ), TM.GetScaledAxis( EAxis::Z ), FColor(200, 255, 255), SphereRadius, SphereSides, SDPG_World );
	}
}
// NVCHANGE_END: JCAO - Using Component Visualizer to display the field sampler component

