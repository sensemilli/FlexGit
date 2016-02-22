// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FlexContainer.h"
#include "FlexAsset.h"

#include "FlexComponent.generated.h"

struct FFlexContainerInstance;
class FFlexMeshSceneProxy;

struct FlexExtInstance;

UENUM()
enum class EFlexParticleMode : uint8
{
	Shape,
	FillVolume,
	ParticleGrid,
	Custom
};

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Flex)
	bool EnableParticleMode;

	/** The simulation container to spawn any flex data contained in the static mesh into */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FlexTemplate", meta=(ToolTip="If the static mesh has Flex data then it will be spawned into this simulation container."))
	UFlexContainer* ContainerTemplate;

	/** The phase to assign to particles spawned for this mesh */
	UPROPERTY(EditAnywhere, Category = "FlexTemplate")
	FFlexPhase Phase;

	/** The per-particle mass to use for the particles, for clothing this value be multiplied by 0-1 dependent on the vertex color*/
	UPROPERTY(EditAnywhere, Category = "FlexTemplate")
	float Mass;

	/** If true then the particles will be attached to any overlapping shapes on spawn*/
	UPROPERTY(EditAnywhere, Category = "FlexTemplate")
	bool AttachToRigids;

	UPROPERTY(EditAnywhere, Category = "FlexTest")
	TArray<FName> DependentComponents;

	UPROPERTY(EditAnywhere, Category = "FlexParticle")
	EFlexParticleMode ParticleMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FlexParticle")
	UMaterialInterface* ParticleMaterial;

	UPROPERTY(EditAnywhere, Category = "FlexParticle")
	float DiffuseParticleScale;

	UPROPERTY(EditAnywhere, Category = "FlexParticle")
	UMaterialInterface* DiffuseParticleMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FlexParticle")
	UFlexFluidSurface* FluidSurfaceTemplate;

	UPROPERTY(EditAnywhere, Category = "FlexFillVolume", meta = (DisplayName="Radius"))
	float VolumeRadius;
	UPROPERTY(EditAnywhere, Category = "FlexFillVolume", meta = (DisplayName = "Separation"))
	float VolumeSeparation;
	UPROPERTY(EditAnywhere, Category = "FlexFillVolume", meta = (DisplayName = "Max Particles"))
	int32 VolumeMaxParticles;
	UPROPERTY(EditAnywhere, Category = "FlexFillVolume", meta = (DisplayName = "Max Attempts"))
	int32 VolumeMaxAttempts;

	UPROPERTY(EditAnywhere, Category = "FlexParticleGrid", meta = (DisplayName = "Dimesions"))
	FIntVector GridDimensions;
	UPROPERTY(EditAnywhere, Category = "FlexParticleGrid", meta = (DisplayName = "Radius"))
	float GridRadius;
	UPROPERTY(EditAnywhere, Category = "FlexParticleGrid", meta = (DisplayName = "Jitter"))
	float GridJitter;

	uint32 bFlexParticlesSpawned : 1;

	TArray<int32> ParticleIndices;

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

	/* The current phase of this component */
	int32 ComponentPhase;

	// sends updated simulation data to the rendering proxy
	void UpdateSceneProxy(FFlexMeshSceneProxy* SceneProxy);

	// Begin UActorComponent Interface
	void SendRenderDynamicData_Concurrent() override;
	// End UActorComponent Interface

	// Begin USceneComponent Interface
	virtual FVector GetCustomLocation() const override;
	// End USceneComponent Interface

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// Begin UPrimitiveComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual bool ShouldRecreateProxyOnUpdateTransform() const override;
	virtual FMatrix GetRenderMatrix() const override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual bool CanEditSimulatePhysics() override { return false; }
	virtual void AddRadialForce(FVector Origin, float Radius, float Strength, enum ERadialImpulseFalloff Falloff, bool bAccelChange = false) override;
	virtual void AddForce(FVector Force, FName BoneName = NAME_None, bool bAccelChange = false) override;
	virtual void AddRadialImpulse(FVector Origin, float Radius, float Strength, enum ERadialImpulseFalloff Falloff, bool bVelChange = false) override;
	// End UPrimitiveComponent Interface

	// Begin IFlexContainerClient Interface
	virtual bool IsEnabled() { return (EnableParticleMode && bFlexParticlesSpawned) || (AssetInstance != NULL); }
	virtual FBoxSphereBounds GetBounds() { return Bounds; }
	virtual void Synchronize();
	// End IFlexContainerClient Interface

	virtual void DisableSim();
	virtual void EnableSim();
	
	/** */
	UFUNCTION(BlueprintCallable, Category="Flex")
	void CreateParticles(TArray<FVector> InPositions, TArray<float> InMasses, TArray<FVector> InVelocities, TArray<int32>& OutIndices);
};
