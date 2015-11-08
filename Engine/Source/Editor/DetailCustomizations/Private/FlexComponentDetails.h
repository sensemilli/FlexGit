// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class FFlexComponentDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	/* Visibility */
	EVisibility FlexStoredSimulationVisible() const;
	EVisibility FlexTemplateVisible() const;
	EVisibility FlexParticleVisible() const;
	EVisibility FlexParticleFillVolumeVisible() const;
	EVisibility FlexParticleGridVisible() const;
	EVisibility FlexParticleShapeVisible() const;
	EVisibility FlexParticleCustomVisible() const;
	EVisibility FlexFluidTemplateVisible() const;

private:

	class UFlexComponent* FlexComponent;
};
