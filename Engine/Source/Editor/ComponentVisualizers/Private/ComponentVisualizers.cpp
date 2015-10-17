// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ComponentVisualizersPrivatePCH.h"
#include "ComponentVisualizers.h"

#include "SoundDefinitions.h"
#include "Perception/PawnSensingComponent.h"
#include "PhysicsEngine/PhysicsSpringComponent.h"

#include "PointLightComponentVisualizer.h"
#include "SpotLightComponentVisualizer.h"
#include "AudioComponentVisualizer.h"
#include "RadialForceComponentVisualizer.h"
#include "ConstraintComponentVisualizer.h"
#include "SpringArmComponentVisualizer.h"
#include "SplineComponentVisualizer.h"
#include "SplineMeshComponentVisualizer.h"
#include "SensingComponentVisualizer.h"
#include "SpringComponentVisualizer.h"
#include "PrimitiveComponentVisualizer.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/AudioComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SplineComponent.h"
#include "Components/PrimitiveComponent.h"

// NVCHANGE_BEGIN: JCAO - Using Component Visualizer to display the field sampler component
#include "AttractorComponentVisualizer.h"
#include "GridComponentVisualizer.h"
#include "JetComponentVisualizer.h"
#include "NoiseComponentVisualizer.h"
#include "VortexComponentVisualizer.h"
#include "HeatSourceComponentVisualizer.h"
#include "VelocitySourceComponentVisualizer.h"
// NVCHANGE_END: JCAO - Using Component Visualizer to display the field sampler component

IMPLEMENT_MODULE( FComponentVisualizersModule, ComponentVisualizers );

void FComponentVisualizersModule::StartupModule()
{
	RegisterComponentVisualizer(UPointLightComponent::StaticClass()->GetFName(), MakeShareable(new FPointLightComponentVisualizer));
	RegisterComponentVisualizer(USpotLightComponent::StaticClass()->GetFName(), MakeShareable(new FSpotLightComponentVisualizer));
	RegisterComponentVisualizer(UAudioComponent::StaticClass()->GetFName(), MakeShareable(new FAudioComponentVisualizer));
	RegisterComponentVisualizer(URadialForceComponent::StaticClass()->GetFName(), MakeShareable(new FRadialForceComponentVisualizer));
	RegisterComponentVisualizer(UPhysicsConstraintComponent::StaticClass()->GetFName(), MakeShareable(new FConstraintComponentVisualizer));
	RegisterComponentVisualizer(USpringArmComponent::StaticClass()->GetFName(), MakeShareable(new FSpringArmComponentVisualizer));
	RegisterComponentVisualizer(USplineComponent::StaticClass()->GetFName(), MakeShareable(new FSplineComponentVisualizer));
	RegisterComponentVisualizer(USplineMeshComponent::StaticClass()->GetFName(), MakeShareable(new FSplineMeshComponentVisualizer));
	RegisterComponentVisualizer(UPawnSensingComponent::StaticClass()->GetFName(), MakeShareable(new FSensingComponentVisualizer));
	RegisterComponentVisualizer(UPhysicsSpringComponent::StaticClass()->GetFName(), MakeShareable(new FSpringComponentVisualizer));
	RegisterComponentVisualizer(UPrimitiveComponent::StaticClass()->GetFName(), MakeShareable(new FPrimitiveComponentVisualizer));

	// NVCHANGE_BEGIN: JCAO - Using Component Visualizer to display the field sampler component	
	RegisterComponentVisualizer(UAttractorComponent::StaticClass()->GetFName(), MakeShareable(new FAttractorComponentVisualizer));
	RegisterComponentVisualizer(UGridComponent::StaticClass()->GetFName(), MakeShareable(new FGridComponentVisualizer));
	RegisterComponentVisualizer(UJetComponent::StaticClass()->GetFName(), MakeShareable(new FJetComponentVisualizer));
	RegisterComponentVisualizer(UNoiseComponent::StaticClass()->GetFName(), MakeShareable(new FNoiseComponentVisualizer));
	RegisterComponentVisualizer(UVortexComponent::StaticClass()->GetFName(), MakeShareable(new FVortexComponentVisualizer));
	RegisterComponentVisualizer(UHeatSourceComponent::StaticClass()->GetFName(), MakeShareable(new FHeatSourceComponentVisualizer));
	RegisterComponentVisualizer(UVelocitySourceComponent::StaticClass()->GetFName(), MakeShareable(new FVelocitySourceComponentVisualizer));
	// NVCHANGE_END: JCAO - Using Component Visualizer to display the field sampler component
}

void FComponentVisualizersModule::ShutdownModule()
{
	if(GUnrealEd != NULL)
	{
		// Iterate over all class names we registered for
		for(FName ClassName : RegisteredComponentClassNames)
		{
			GUnrealEd->UnregisterComponentVisualizer(ClassName);
		}
	}
}

void FComponentVisualizersModule::RegisterComponentVisualizer(FName ComponentClassName, TSharedPtr<FComponentVisualizer> Visualizer)
{
	if (GUnrealEd != NULL)
	{
		GUnrealEd->RegisterComponentVisualizer(ComponentClassName, Visualizer);
	}

	RegisteredComponentClassNames.Add(ComponentClassName);

	if (Visualizer.IsValid())
	{
		Visualizer->OnRegister();
	}
}
