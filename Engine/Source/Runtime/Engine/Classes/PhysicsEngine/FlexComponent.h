// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FlexContainer.h"
#include "FlexAsset.h"

#include "FlexComponent.generated.h"

struct FFlexContainerInstance;
class FFlexMeshSceneProxy;

struct FlexExtInstance;

UCLASS(hidecategories = (Object), meta = (BlueprintSpawnableComponent))
class ENGINE_API UFlexComponent : public UStaticMeshComponent, public IFlexContainerClient
{
	GENERATED_UCLASS_BODY()		

public:

	struct FlexParticleAttachment
	{
		TWeakObjectPtr<UPrimitiveComponent> Primitive;
		int32 ShapeIndex;
		int32 ParticleIndex;
		float OldMass;
		FVector LocalPos;
	};

	/** Override the FlexAsset's container / phase / attachment properties */
	UPROPERTY(EditAnywhere, Category = Flex)
	bool OverrideAsset;

	/** The simulation container to spawn any flex data contained in the static mesh into */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Flex, meta=(editcondition = "OverrideAsset", ToolTip="If the static mesh has Flex data then it will be spawned into this simulation container."))
	UFlexContainer* ContainerTemplate;

	/** The phase to assign to particles spawned for this mesh */
	UPROPERTY(EditAnywhere, Category = Flex, meta = (editcondition = "OverrideAsset"))
	FFlexPhase Phase;

	/** The per-particle mass to use for the particles, for clothing this value be multiplied by 0-1 dependent on the vertex color*/
	UPROPERTY(EditAnywhere, Category = Flex, meta = (editcondition = "OverrideAsset"))
	float Mass;

	/** If true then the particles will be attached to any overlapping shapes on spawn*/
	UPROPERTY(EditAnywhere, Category = Flex, meta = (editcondition = "OverrideAsset"))
	bool AttachToRigids;

	/** Instance of a FlexAsset referencing particles and constraints in a solver */
	FlexExtInstance* AssetInstance;

	/* The simulation container the instance belongs to */
	FFlexContainerInstance* ContainerInstance;

	/* Simulated particle positions  */
	TArray<FVector4> SimPositions;
	/* Simulated particle normals */
	TArray<FVector> SimNormals;

	/* Pre-simulated particle positions  */
	UPROPERTY(NonPIEDuplicateTransient)
	TArray<FVector> PreSimPositions;

	/* Shape transforms */
	TArray<FQuat> ShapeRotations;
	TArray<FVector> ShapeTranslations;

	/* Attachments to rigid bodies */
	TArray<FlexParticleAttachment> Attachments;

	/* Cached local bounds */
	FBoxSphereBounds LocalBounds;

	// sends updated simulation data to the rendering proxy
	void UpdateSceneProxy(FFlexMeshSceneProxy* SceneProxy);

	// Begin UActorComponent Interface
	void SendRenderDynamicData_Concurrent() override;
	// End UActorComponent Interface

	// Begin UPrimitiveComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual bool ShouldRecreateProxyOnUpdateTransform() const override;
	virtual FMatrix GetRenderMatrix() const override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual bool CanEditSimulatePhysics() override { return false; }
	// End UPrimitiveComponent Interface

	// Begin IFlexContainerClient Interface
	virtual bool IsEnabled() { return AssetInstance != NULL; }
	virtual FBoxSphereBounds GetBounds() { return Bounds; }
	virtual void Synchronize();
	// End IFlexContainerClient Interface

	virtual void DisableSim();
	virtual void EnableSim();
	
};
