// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "FlexComponentDetails.h"
#include "PhysicsEngine/FlexComponent.h"

#define LOCTEXT_NAMESPACE "FlexComponentDetails"


TSharedRef<IDetailCustomization> FFlexComponentDetails::MakeInstance()
{
	return MakeShareable(new FFlexComponentDetails);
}

void FFlexComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Create a category so this is displayed early in the properties
	DetailBuilder.EditCategory("Flex", FText::GetEmpty(), ECategoryPriority::TypeSpecific);

	// Hide categories to start off with
	DetailBuilder.HideCategory("FlexTest");
	DetailBuilder.HideCategory("FlexTemplate");
	DetailBuilder.HideCategory("FlexParticle");
	DetailBuilder.HideCategory("FlexFillVolume");
	DetailBuilder.HideCategory("FlexParticleGrid");

	TAttribute<EVisibility> FlexStoredSimulationVisibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FFlexComponentDetails::FlexStoredSimulationVisible));
	TAttribute<EVisibility> FlexTemplateVisibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FFlexComponentDetails::FlexTemplateVisible));
	TAttribute<EVisibility> FlexParticleVisibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FFlexComponentDetails::FlexParticleVisible));
	TAttribute<EVisibility> FlexParticleFillVolumeVisibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FFlexComponentDetails::FlexParticleFillVolumeVisible));
	TAttribute<EVisibility> FlexParticleGridVisibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FFlexComponentDetails::FlexParticleGridVisible));
	TAttribute<EVisibility> FlexParticleShapeVisibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FFlexComponentDetails::FlexParticleShapeVisible));
	TAttribute<EVisibility> FlexParticleCustomVisibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FFlexComponentDetails::FlexParticleCustomVisible));
	TAttribute<EVisibility> FlexFluidTemplateVisibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FFlexComponentDetails::FlexFluidTemplateVisible));
	IDetailCategoryBuilder& FlexCategory = DetailBuilder.EditCategory("Flex");
	
	FlexCategory.AddCustomRow(FText::FromString(TEXT("SimulationData")))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)

			+SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(0, 4, 0, 4)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("This FlexComponent contains stored simulation data")))
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
				]
		].Visibility(FlexStoredSimulationVisibility);

	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, OverrideAsset)));
	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, EnableParticleMode)));

	FlexCategory.AddCustomRow(FText::FromString(TEXT("Template")))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(0, 4, 0, 4)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Template")))
				.Font(IDetailLayoutBuilder::GetDetailFontBold())
			]
		].Visibility(FlexTemplateVisibility);

	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, ContainerTemplate))).Visibility(FlexTemplateVisibility);
	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, Phase))).Visibility(FlexTemplateVisibility);
	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, Mass))).Visibility(FlexTemplateVisibility);
	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, AttachToRigids))).Visibility(FlexTemplateVisibility);

	FlexCategory.AddCustomRow(FText::FromString(TEXT("Particle")))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(0, 4, 0, 4)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Particle")))
				.Font(IDetailLayoutBuilder::GetDetailFontBold())
			]
		].Visibility(FlexParticleVisibility);

	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, ParticleMode))).Visibility(FlexParticleVisibility);
	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, ParticleMaterial))).Visibility(FlexParticleVisibility);
	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, DiffuseParticleMaterial))).Visibility(FlexParticleVisibility);
	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, DiffuseParticleScale))).Visibility(FlexParticleVisibility);
	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, FluidSurfaceTemplate))).Visibility(FlexFluidTemplateVisibility);

	FlexCategory.AddCustomRow(FText::FromString(TEXT("Particle Properties")))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(0, 4, 0, 4)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Particle Properties")))
				.Font(IDetailLayoutBuilder::GetDetailFontBold())
			]
		].Visibility(FlexParticleVisibility);

	/* Volume Fill */
	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, VolumeRadius))).Visibility(FlexParticleFillVolumeVisibility);
	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, VolumeSeparation))).Visibility(FlexParticleFillVolumeVisibility);
	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, VolumeMaxParticles))).Visibility(FlexParticleFillVolumeVisibility);
	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, VolumeMaxAttempts))).Visibility(FlexParticleFillVolumeVisibility);

	/* Particle Grid */
	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, GridDimensions))).Visibility(FlexParticleGridVisibility);
	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, GridRadius))).Visibility(FlexParticleGridVisibility);
	FlexCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlexComponent, GridJitter))).Visibility(FlexParticleGridVisibility);

	/* Shape */
	FlexCategory.AddCustomRow(FText::FromString(TEXT("ShapeProperties")))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(0, 4, 0, 4)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Configure the shape using the Static Mesh property")))
				.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
		].Visibility(FlexParticleShapeVisibility);

		/* Custom */
		FlexCategory.AddCustomRow(FText::FromString(TEXT("CustomProperties")))
			.WholeRowContent()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(0, 4, 0, 4)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Spawn particles in Blueprints")))
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
				]
			].Visibility(FlexParticleCustomVisibility);

	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	FlexComponent = Cast<UFlexComponent>(ObjectsBeingCustomized[0].Get());
}

EVisibility FFlexComponentDetails::FlexStoredSimulationVisible() const
{
	return (FlexComponent->PreSimPositions.Num() > 0) ? EVisibility::Visible : EVisibility::Hidden;
}

EVisibility FFlexComponentDetails::FlexTemplateVisible() const
{
	return (FlexComponent->OverrideAsset) ? EVisibility::Visible : EVisibility::Hidden;
}

EVisibility FFlexComponentDetails::FlexParticleVisible() const
{
	return (FlexComponent->EnableParticleMode) ? EVisibility::Visible : EVisibility::Hidden;
}

EVisibility FFlexComponentDetails::FlexParticleFillVolumeVisible() const
{
	return (FlexComponent->EnableParticleMode && FlexComponent->ParticleMode == EFlexParticleMode::FillVolume) ? EVisibility::Visible : EVisibility::Hidden;
}

EVisibility FFlexComponentDetails::FlexParticleGridVisible() const
{
	return (FlexComponent->EnableParticleMode && FlexComponent->ParticleMode == EFlexParticleMode::ParticleGrid) ? EVisibility::Visible : EVisibility::Hidden;
}

EVisibility FFlexComponentDetails::FlexParticleShapeVisible() const
{
	return (FlexComponent->EnableParticleMode && FlexComponent->ParticleMode == EFlexParticleMode::Shape) ? EVisibility::Visible : EVisibility::Hidden;
}

EVisibility FFlexComponentDetails::FlexParticleCustomVisible() const
{
	return (FlexComponent->EnableParticleMode && FlexComponent->ParticleMode == EFlexParticleMode::Custom) ? EVisibility::Visible : EVisibility::Hidden;
}

EVisibility FFlexComponentDetails::FlexFluidTemplateVisible() const
{
	return (FlexComponent->EnableParticleMode && FlexComponent->Phase.Fluid) ? EVisibility::Visible : EVisibility::Hidden;
}

#undef LOCTEXT_NAMESPACE
