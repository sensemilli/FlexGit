// NVCHANGE_BEGIN: JCAO - Using Component Visualizer to display the field sampler component

#include "ComponentVisualizersPrivatePCH.h"

#include "VortexComponentVisualizer.h"


void FVortexComponentVisualizer::DrawVisualization( const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI )
{
	const UVortexComponent* VortexComp = Cast<const UVortexComponent>(Component);
	if(VortexComp != NULL && VortexComp->VortexAsset != NULL)
	{
		float CapsuleBottomRadius = VortexComp->VortexAsset->CapsuleFieldBottomRadius;
		float CapsuleTopRadius = VortexComp->VortexAsset->CapsuleFieldTopRadius;
		float CapsuleHalfHeight = VortexComp->VortexAsset->CapsuleFieldHeight * 0.5f;

		FTransform TM = VortexComp->ComponentToWorld;
		const FQuat RelativeRotQuat = TM.GetRotation();
		const FQuat DeltaRotQuat = FRotator(-90.0f, 0, 0).Quaternion();
		const FQuat NewRelRotQuat = RelativeRotQuat * DeltaRotQuat;
		TM.SetRotation(NewRelRotQuat);

		const FMatrix& LocalToWorld = TM.ToMatrixNoScale();
		const int32 CapsuleSides =  FMath::Clamp<int32>(CapsuleBottomRadius/4.f, 16, 64);
		FMatrix rot(LocalToWorld.GetScaledAxis( EAxis::X ), LocalToWorld.GetScaledAxis( EAxis::Y ), LocalToWorld.GetScaledAxis( EAxis::Z ), FVector(0,0,0));
		FVector Offset = FVector( 0, 0, CapsuleHalfHeight );
		Offset = rot.TransformVector(Offset);

		DrawCircle( PDI, LocalToWorld.GetOrigin()-Offset, LocalToWorld.GetScaledAxis( EAxis::X ), LocalToWorld.GetScaledAxis( EAxis::Y ), FColor(200, 255, 255), CapsuleBottomRadius, 32, SDPG_World );	
		DrawCircle( PDI, LocalToWorld.GetOrigin()-Offset, LocalToWorld.GetScaledAxis( EAxis::X ), LocalToWorld.GetScaledAxis( EAxis::Z ), FColor(200, 255, 255), CapsuleBottomRadius, 32, SDPG_World );	
		DrawCircle( PDI, LocalToWorld.GetOrigin()-Offset, LocalToWorld.GetScaledAxis( EAxis::Y ), LocalToWorld.GetScaledAxis( EAxis::Z ), FColor(200, 255, 255), CapsuleBottomRadius, 32, SDPG_World );	

		DrawCircle( PDI, LocalToWorld.GetOrigin()+Offset, LocalToWorld.GetScaledAxis( EAxis::X ), LocalToWorld.GetScaledAxis( EAxis::Y ), FColor(200, 255, 255), CapsuleTopRadius, 32, SDPG_World );	
		DrawCircle( PDI, LocalToWorld.GetOrigin()+Offset, LocalToWorld.GetScaledAxis( EAxis::X ), LocalToWorld.GetScaledAxis( EAxis::Z ), FColor(200, 255, 255), CapsuleTopRadius, 32, SDPG_World );	
		DrawCircle( PDI, LocalToWorld.GetOrigin()+Offset, LocalToWorld.GetScaledAxis( EAxis::Y ), LocalToWorld.GetScaledAxis( EAxis::Z ), FColor(200, 255, 255), CapsuleTopRadius, 32, SDPG_World );	

		DrawWireChoppedCone( PDI, LocalToWorld.GetOrigin(), LocalToWorld.GetScaledAxis( EAxis::X ), LocalToWorld.GetScaledAxis( EAxis::Y ), LocalToWorld.GetScaledAxis( EAxis::Z ), FColor(200, 255, 255), CapsuleBottomRadius, CapsuleTopRadius, CapsuleHalfHeight, CapsuleSides, SDPG_World );

	}
}
// NVCHANGE_END: JCAO - Using Component Visualizer to display the field sampler component
