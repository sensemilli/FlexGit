// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ParticleModules_Flex.cpp: 
	Flex-related particle module implementations.
=============================================================================*/

#include "EnginePrivate.h"
#include "ParticleDefinitions.h"

#include "Particles/Modules/Flex/ParticleModuleFlexShapeSpawn.h"
#include "Particles/Modules/Flex/ParticleModuleFlexFluidSpawn.h"

#include "PhysicsEngine/FlexContainer.h"
#include "PhysicsEngine/FlexAsset.h"

/*-----------------------------------------------------------------------------
UParticleModuleFlexShapeSpawn implementation.
-----------------------------------------------------------------------------*/

UParticleModuleFlexShapeSpawn::UParticleModuleFlexShapeSpawn(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bSpawnModule = true;
	bSupported3DDrawMode = false;

	Mesh = NULL;
	Velocity = 0.0f;
}

bool UParticleModuleFlexShapeSpawn::GetSpawnAmount(FParticleEmitterInstance* Owner, int32 Offset, float OldLeftover, float DeltaTime, int32& Number, float& Rate)
{
	return false;
}

bool UParticleModuleFlexShapeSpawn::GetBurstCount(FParticleEmitterInstance* Owner, int32 Offset, float OldLeftover, float DeltaTime, int32& Number)
{
	if (Mesh && Mesh->FlexAsset)
	{
		Number = Mesh->FlexAsset->Particles.Num();
		return true;
	}

	return false;
}

void UParticleModuleFlexShapeSpawn::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
}

void UParticleModuleFlexShapeSpawn::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	SPAWN_INIT;

	if (Mesh && Mesh->FlexAsset)
	{
		// calculate spawn position based on the particle counter
		int32 ParticlesPerShape = Mesh->FlexAsset->Particles.Num();
		int32 ParticleShapeIndex = Owner->ParticleCounter%ParticlesPerShape;

		FVector Vel = Owner->EmitterToSimulation.TransformVector(FVector(0.0f, 0.0f, Velocity));

		Particle.Location += Owner->EmitterToSimulation.TransformVector(FVector(Mesh->FlexAsset->Particles[ParticleShapeIndex]));
		Particle.Velocity += Vel;
		Particle.BaseVelocity += Vel;
	}
}

int32 UParticleModuleFlexShapeSpawn::GetMaximumBurstCount()
{
	if (Mesh && Mesh->FlexAsset)
		return Mesh->FlexAsset->Particles.Num();
	else
		return 0;
}


/*-----------------------------------------------------------------------------
	UParticleModuleFlexFluidSpawn implementation.
-----------------------------------------------------------------------------*/

UParticleModuleFlexFluidSpawn::UParticleModuleFlexFluidSpawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSpawnModule = true;
	bSupported3DDrawMode = false;

	DimX = 4;
	DimY = 4;
	Velocity = 200.0f;
}

bool UParticleModuleFlexFluidSpawn::GetSpawnAmount(FParticleEmitterInstance* Owner, int32 Offset, float OldLeftover, float DeltaTime, int32& Number, float& Rate)
{
	return false;
}

bool UParticleModuleFlexFluidSpawn::GetBurstCount(FParticleEmitterInstance* Owner, int32 Offset, float OldLeftover, float DeltaTime, int32& Number)
{
	// how many layers we need to emit
	InstancePayload& Payload = *((InstancePayload*)Owner->GetModuleInstanceData(this));

	UFlexContainer* Template = Owner->SpriteTemplate->FlexContainerTemplate;

	if (Template)
	{
		// ensure a constant spacing between layers
		float Spacing = Template->Radius;

		if (Template->Fluid)
			Spacing *= Template->RestDistance;

		float LayersPerSecond = Velocity/Spacing;
		float LayerCount = Payload.LayerLeftOver + LayersPerSecond*DeltaTime;
		
		int32 NumLayers = int32(LayerCount);

		// currently only spawn 1 layer per-frame, this can cause gaps when frame rate changes
		NumLayers = FMath::Min(1, NumLayers);
	
		Number =  DimX*DimY*int32(NumLayers);

		Payload.LayerLeftOver = LayerCount - NumLayers;
	}

	return true;
}

void UParticleModuleFlexFluidSpawn::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
}

void UParticleModuleFlexFluidSpawn::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	SPAWN_INIT;

	UFlexContainer* Template = Owner->SpriteTemplate->FlexContainerTemplate;

	if (Template)
	{
		// ensure a constant spacing between layers
		float Spacing = Template->Radius;

		if (Template->Fluid)
			Spacing *= Template->RestDistance;

		// calculate spawn position based on the particle counter
		int32 ParticlesPerLayer = DimX*DimY;
		int32 LayerIndex = Owner->ParticleCounter%ParticlesPerLayer;

		int32 X = LayerIndex%DimY;
		int32 Y = LayerIndex/DimX;

		FVector Vel = Owner->EmitterToSimulation.TransformVector(FVector(0.0f, 0.0f, Velocity));

		Particle.Location += Owner->EmitterToSimulation.TransformVector(FVector(X*Spacing, Y*Spacing, 0.0f));
		Particle.Velocity += Vel;
		Particle.BaseVelocity += Vel;
	}	
}

int32 UParticleModuleFlexFluidSpawn::GetMaximumBurstCount()
{
	return DimX*DimY;
}

uint32 UParticleModuleFlexFluidSpawn::RequiredBytesPerInstance(FParticleEmitterInstance* Owner)
{
	return sizeof(InstancePayload);
}

uint32 UParticleModuleFlexFluidSpawn::PrepPerInstanceBlock(FParticleEmitterInstance* Owner, void* InstData)
{
	InstancePayload& PayLoad = *((InstancePayload*)InstData);

	PayLoad.LayerLeftOver = 0.0f;
	PayLoad.LayerIndex = 0;

	return 0;
}

