// NVCHANGE_BEGIN: JCAO - Using Component Visualizer to display the field sampler component

#include "ComponentVisualizersPrivatePCH.h"

#include "JetComponentVisualizer.h"


void FJetComponentVisualizer::DrawVisualization( const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI )
{
	const UJetComponent* JetComp = Cast<const UJetComponent>(Component);
	if(JetComp != NULL && JetComp->JetAsset != NULL)
	{
		FTransform TM = JetComp->ComponentToWorld;
		float CapsuleHalfHeight = JetComp->JetAsset->GridShapeHeight;
		float CapsuleRadius = JetComp->JetAsset->GridShapeRadius;
		const int32 CapsuleSides =  FMath::Clamp<int32>(CapsuleRadius/4.f, 16, 64);

		const FQuat RelativeRotQuat = TM.GetRotation();
		const FQuat DeltaRotQuat = FRotator(-90.0f, 0, 0).Quaternion();
		const FQuat NewRelRotQuat = RelativeRotQuat * DeltaRotQuat;
		TM.SetRotation(NewRelRotQuat);

		DrawWireCapsule( PDI, TM.GetTranslation(), TM.GetUnitAxis( EAxis::X ), TM.GetUnitAxis( EAxis::Y ), TM.GetUnitAxis( EAxis::Z ), FColor(200, 255, 255), CapsuleRadius, CapsuleHalfHeight, CapsuleSides, SDPG_World);

	}
}
// NVCHANGE_END: JCAO - Using Component Visualizer to display the field sampler component
