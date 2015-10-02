// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

/**
 *	ParticleModuleFlexFluidSpawn
 *
 *	Spawns particles at fluid rest density with appropriate velocity
 *
 */

#pragma once

#include "Particles/Spawn/ParticleModuleSpawnBase.h"
#include "ParticleModuleFlexFluidSpawn.generated.h"

UCLASS(editinlinenew, hidecategories = Object, MinimalAPI, meta = (DisplayName = "Spawn Flex Fluid"))
class UParticleModuleFlexFluidSpawn : public UParticleModuleSpawnBase
{
	GENERATED_UCLASS_BODY()

	struct InstancePayload
	{
		float LayerLeftOver;
		int32 LayerIndex;
	};

	/** The number of particles to emit horizontally  */
	UPROPERTY(EditAnywhere, Category=Spawn)
	int32 DimX;

	/** The number of particles to emit vertically */
	UPROPERTY(EditAnywhere, Category=Spawn)
	int32 DimY;

	/** Velocity to emit particles with, note that this increases the required spawn rate */
	UPROPERTY(EditAnywhere, Category=Velocity)
	float Velocity;

	// Begin UParticleModuleSpawnBase Interface
	virtual bool	GetSpawnAmount(FParticleEmitterInstance* Owner, int32 Offset, float OldLeftover, float DeltaTime, int32& Number, float& Rate) override;
	virtual bool	GetBurstCount(FParticleEmitterInstance* Owner, int32 Offset, float OldLeftover, float DeltaTime, int32& Number) override;
	virtual int32	GetMaximumBurstCount() override;
	// End UParticleModuleSpawnBase Interface

	// Begin UParticleModule Interface
	virtual uint32	RequiredBytesPerInstance(FParticleEmitterInstance* Owner = NULL) override;
	virtual uint32	PrepPerInstanceBlock(FParticleEmitterInstance* Owner, void* InstData) override;
	virtual void	Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
	virtual void	Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
	// End UParticleModule Interface
};



