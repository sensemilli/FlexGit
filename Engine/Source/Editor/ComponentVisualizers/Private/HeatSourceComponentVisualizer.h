// NVCHANGE_BEGIN: JCAO - Using Component Visualizer to display the field sampler component

#pragma once

#include "ComponentVisualizer.h"

class FHeatSourceComponentVisualizer : public FComponentVisualizer
{
public:
	// Begin FComponentVisualizer interface
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	// End FComponentVisualizer interface
};
// NVCHANGE_END: JCAO - Using Component Visualizer to display the field sampler component
