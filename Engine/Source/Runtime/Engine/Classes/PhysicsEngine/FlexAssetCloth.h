// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FlexAssetCloth.generated.h"

/* A Flex cloth asset is a specialized Flex asset that creates one particle per mesh vertex and contains parameter 
to configure cloth behavior. */
UCLASS(config = Engine, editinlinenew, meta = (DisplayName = "Flex Cloth Asset"))
class ENGINE_API UFlexAssetCloth : public UFlexAsset
{
public:

	GENERATED_UCLASS_BODY()

	/** How much the cloth resists stretching */
	UPROPERTY(EditAnywhere, Category = Flex)
	float StretchStiffness;

	/** How much the cloth resists bending */
	UPROPERTY(EditAnywhere, Category = Flex)
	float BendStiffness;

	/** How strong tethers resist stretching */
	UPROPERTY(EditAnywhere, Category = Flex)
	float TetherStiffness;

	/** How much tethers have to stretch past their rest-length before becoming enabled, 0.1=10% elongation */
	UPROPERTY(EditAnywhere, Category = Flex)
	float TetherGive;

	/** Can be enabled for closed meshes, a volume conserving constraint will be added to the simulation */
	UPROPERTY(EditAnywhere, Category = Flex)
	bool EnableInflatable;

	/** The inflatable pressure, a value of 1.0 corresponds to the rest volume, 0.5 corressponds to being deflated by half, and values > 1.0 correspond to over-inflation */
	UPROPERTY(EditAnywhere, Category = Flex, meta = (editcondition = "EnableInflatable"))
	float OverPressure;

	/** The stiffness coefficient for the inflable, this will automatically be calculated */
	UPROPERTY()
	float InflatableStiffness;

	/* The rest volume of the inflatable, this will automatically be calculated */
	UPROPERTY()
	float InflatableVolume;

	/** The rigid body stiffness */
	UPROPERTY(EditAnywhere, Category = Flex)
	float RigidStiffness;

	/** Store the rigid body center of mass, not editable */
	UPROPERTY()
	FVector RigidCenter;

	virtual void ReImport(const UStaticMesh* Parent) override;
	virtual FlexExtAsset* GetFlexAsset() override;
};
