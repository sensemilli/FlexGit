// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FXSystem.cpp: Implementation of the effects system.
=============================================================================*/

#include "EnginePrivate.h"
#include "SystemSettings.h"
#include "RHI.h"
#include "RHIStaticStates.h"
#include "RenderResource.h"
#include "FXSystemPrivate.h"
#include "../VectorField.h"
#include "../GPUSort.h"
#include "ParticleCurveTexture.h"
#include "VectorField/VectorField.h"
#include "Components/VectorFieldComponent.h"

/*-----------------------------------------------------------------------------
	External FX system interface.
-----------------------------------------------------------------------------*/

FFXSystemInterface* FFXSystemInterface::Create(ERHIFeatureLevel::Type InFeatureLevel, EShaderPlatform InShaderPlatform)
{
	return new FFXSystem(InFeatureLevel, InShaderPlatform);
}

void FFXSystemInterface::Destroy( FFXSystemInterface* FXSystem )
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		FDestroyFXSystemCommand,
		FFXSystemInterface*, FXSystem, FXSystem,
	{
		delete FXSystem;
	});
}

FFXSystemInterface::~FFXSystemInterface()
{
}

/*------------------------------------------------------------------------------
	FX system console variables.
------------------------------------------------------------------------------*/

namespace FXConsoleVariables
{
	int32 VisualizeGPUSimulation = 0;
	int32 bAllowGPUSorting = true;
	int32 bAllowCulling = true;
	int32 bFreezeGPUSimulation = false;
	int32 bFreezeParticleSimulation = false;
	int32 bAllowAsyncTick = false;
	float ParticleSlackGPU = 0.02f;
	int32 MaxParticleTilePreAllocation = 100;

#if WITH_FLEX	
	int32 MaxCPUParticlesPerEmitter = 16 * 1024;
#else
	int32 MaxCPUParticlesPerEmitter = 1000;
#endif

	int32 MaxGPUParticlesSpawnedPerFrame = 1024 * 1024;
	int32 GPUSpawnWarningThreshold = 20000;
	float GPUCollisionDepthBounds = 500.0f;
	TAutoConsoleVariable<int32> TestGPUSort(TEXT("FX.TestGPUSort"),0,TEXT("Test GPU sort. 1: Small, 2: Large, 3: Exhaustive, 4: Random"),ECVF_Cheat);

	/** Register references to flags. */
	FAutoConsoleVariableRef CVarVisualizeGPUSimulation(
		TEXT("FX.VisualizeGPUSimulation"),
		VisualizeGPUSimulation,
		TEXT("Visualize the current state of GPU simulation.\n")
		TEXT("0 = off\n")
		TEXT("1 = visualize particle state\n")
		TEXT("2 = visualize curve texture"),
		ECVF_Cheat
		);
	FAutoConsoleVariableRef CVarAllowGPUSorting(
		TEXT("FX.AllowGPUSorting"),
		bAllowGPUSorting,
		TEXT("Allow particles to be sorted on the GPU."),
		ECVF_Cheat
		);
	FAutoConsoleVariableRef CVarFreezeGPUSimulation(
		TEXT("FX.FreezeGPUSimulation"),
		bFreezeGPUSimulation,
		TEXT("Freeze particles simulated on the GPU."),
		ECVF_Cheat
		);
	FAutoConsoleVariableRef CVarFreezeParticleSimulation(
		TEXT("FX.FreezeParticleSimulation"),
		bFreezeParticleSimulation,
		TEXT("Freeze particle simulation."),
		ECVF_Cheat
		);
	FAutoConsoleVariableRef CVarAllowAsyncTick(
		TEXT("FX.AllowAsyncTick"),
		bAllowAsyncTick,
		TEXT("allow parallel ticking of particle systems."),
		ECVF_Cheat
		);
	FAutoConsoleVariableRef CVarParticleSlackGPU(
		TEXT("FX.ParticleSlackGPU"),
		ParticleSlackGPU,
		TEXT("Amount of slack to allocate for GPU particles to prevent tile churn as percentage of total particles."),
		ECVF_Cheat
		);
	FAutoConsoleVariableRef CVarMaxParticleTilePreAllocation(
		TEXT("FX.MaxParticleTilePreAllocation"),
		MaxParticleTilePreAllocation,
		TEXT("Maximum tile preallocation for GPU particles."),
		ECVF_Cheat
		);
	FAutoConsoleVariableRef CVarMaxCPUParticlesPerEmitter(
		TEXT("FX.MaxCPUParticlesPerEmitter"),
		MaxCPUParticlesPerEmitter,
		TEXT("Maximum number of CPU particles allowed per-emitter."),
		ECVF_Cheat
		);
	FAutoConsoleVariableRef CVarMaxGPUParticlesSpawnedPerFrame(
		TEXT("FX.MaxGPUParticlesSpawnedPerFrame"),
		MaxGPUParticlesSpawnedPerFrame,
		TEXT("Maximum number of GPU particles allowed to spawn per-frame per-emitter."),
		ECVF_Cheat
		);
	FAutoConsoleVariableRef CVarGPUSpawnWarningThreshold(
		TEXT("FX.GPUSpawnWarningThreshold"),
		GPUSpawnWarningThreshold,
		TEXT("Warning threshold for spawning of GPU particles."),
		ECVF_Cheat
		);
	FAutoConsoleVariableRef CVarGPUCollisionDepthBounds(
		TEXT("FX.GPUCollisionDepthBounds"),
		GPUCollisionDepthBounds,
		TEXT("Limits the depth bounds when searching for a collision plane."),
		ECVF_Cheat
		);
	FAutoConsoleVariableRef CVarAllowCulling(
		TEXT("FX.AllowCulling"),
		bAllowCulling,
		TEXT("Allow emitters to be culled."),
		ECVF_Cheat
		);
}

/*------------------------------------------------------------------------------
	FX system.
------------------------------------------------------------------------------*/

FFXSystem::FFXSystem(ERHIFeatureLevel::Type InFeatureLevel, EShaderPlatform InShaderPlatform)
	: ParticleSimulationResources(NULL)
	, FeatureLevel(InFeatureLevel)
	, ShaderPlatform(InShaderPlatform)
#if WITH_EDITOR
	, bSuspended(false)
#endif // #if WITH_EDITOR
{
	InitGPUSimulation();
}

FFXSystem::~FFXSystem()
{
	DestroyGPUSimulation();
}

void FFXSystem::Tick(float DeltaSeconds)
{
	if (RHISupportsGPUParticles(FeatureLevel))
	{
		// Test GPU sorting if requested.
		if (FXConsoleVariables::TestGPUSort.GetValueOnGameThread() != 0)
		{
			TestGPUSort((EGPUSortTest)FXConsoleVariables::TestGPUSort.GetValueOnGameThread(), GetFeatureLevel());
			// Reset CVar
			static IConsoleVariable* CVarTestGPUSort = IConsoleManager::Get().FindConsoleVariable(TEXT("FX.TestGPUSort"));

			// todo: bad use of console variables, this should be a console command 
			CVarTestGPUSort->Set(0, ECVF_SetByCode);
		}

		// Before ticking GPU particles, ensure any pending curves have been
		// uploaded.
		GParticleCurveTexture.SubmitPendingCurves();
	}
}

#if WITH_EDITOR
void FFXSystem::Suspend()
{
	if (!bSuspended && RHISupportsGPUParticles(FeatureLevel))
	{
		ReleaseGPUResources();
		bSuspended = true;
	}
}

void FFXSystem::Resume()
{
	if (bSuspended && RHISupportsGPUParticles(FeatureLevel))
	{
		bSuspended = false;
		InitGPUResources();
	}
}
#endif // #if WITH_EDITOR

/*------------------------------------------------------------------------------
	Vector field instances.
------------------------------------------------------------------------------*/

void FFXSystem::AddVectorField( UVectorFieldComponent* VectorFieldComponent )
{
	if (RHISupportsGPUParticles(FeatureLevel))
	{
		check( VectorFieldComponent->VectorFieldInstance == NULL );
		check( VectorFieldComponent->FXSystem == this );

		if ( VectorFieldComponent->VectorField )
		{
			FVectorFieldInstance* Instance = new FVectorFieldInstance();
			VectorFieldComponent->VectorField->InitInstance(Instance, /*bPreviewInstance=*/ false);
			VectorFieldComponent->VectorFieldInstance = Instance;
			Instance->WorldBounds = VectorFieldComponent->Bounds.GetBox();
			Instance->Intensity = VectorFieldComponent->Intensity;
			Instance->Tightness = VectorFieldComponent->Tightness;

			ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
				FAddVectorFieldCommand,
				FFXSystem*, FXSystem, this,
				FVectorFieldInstance*, Instance, Instance,
				FMatrix, ComponentToWorld, VectorFieldComponent->ComponentToWorld.ToMatrixWithScale(),
			{
				Instance->UpdateTransforms( ComponentToWorld );
				Instance->Index = FXSystem->VectorFields.AddUninitialized().Index;
				FXSystem->VectorFields[ Instance->Index ] = Instance;
			});
		}
	}
}

void FFXSystem::RemoveVectorField( UVectorFieldComponent* VectorFieldComponent )
{
	if (RHISupportsGPUParticles(FeatureLevel))
	{
		check( VectorFieldComponent->FXSystem == this );

		FVectorFieldInstance* Instance = VectorFieldComponent->VectorFieldInstance;
		VectorFieldComponent->VectorFieldInstance = NULL;

		if ( Instance )
		{
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
				FRemoveVectorFieldCommand,
				FFXSystem*, FXSystem, this,
				FVectorFieldInstance*, Instance, Instance,
			{
				if ( Instance->Index != INDEX_NONE )
				{
					FXSystem->VectorFields.RemoveAt( Instance->Index );
					delete Instance;
				}
			});
		}
	}
}

void FFXSystem::UpdateVectorField( UVectorFieldComponent* VectorFieldComponent )
{
	if (RHISupportsGPUParticles(FeatureLevel))
	{
		check( VectorFieldComponent->FXSystem == this );

		FVectorFieldInstance* Instance = VectorFieldComponent->VectorFieldInstance;

		if ( Instance )
		{
			struct FUpdateVectorFieldParams
			{
				FBox Bounds;
				FMatrix ComponentToWorld;
				float Intensity;
				float Tightness;
			};

			FUpdateVectorFieldParams UpdateParams;
			UpdateParams.Bounds = VectorFieldComponent->Bounds.GetBox();
			UpdateParams.ComponentToWorld = VectorFieldComponent->ComponentToWorld.ToMatrixWithScale();
			UpdateParams.Intensity = VectorFieldComponent->Intensity;
			UpdateParams.Tightness = VectorFieldComponent->Tightness;

			ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
				FUpdateVectorFieldCommand,
				FFXSystem*, FXSystem, this,
				FVectorFieldInstance*, Instance, Instance,
				FUpdateVectorFieldParams, UpdateParams, UpdateParams,
			{
				Instance->WorldBounds = UpdateParams.Bounds;
				Instance->Intensity = UpdateParams.Intensity;
				Instance->Tightness = UpdateParams.Tightness;
				Instance->UpdateTransforms( UpdateParams.ComponentToWorld );
			});
		}
	}
}

// NVCHANGE_BEGIN: JCAO - Replace vector fields with APEX turbulence velocity fields
void FFXSystem::AddFieldSampler(UFieldSamplerComponent* FSComponent)
{
	if (RHISupportsGPUParticles(FeatureLevel))
	{
		check(FSComponent->FieldSamplerInstance == NULL);
		check(FSComponent->FXSystem == this);

		if (FSComponent->CreateFieldSamplerInstance())
		{
			AddFieldSampler(FSComponent->FieldSamplerInstance, FSComponent->ComponentToWorld.ToMatrixWithScale());
		}
	}
}

void FFXSystem::RemoveFieldSampler(UFieldSamplerComponent* FSComponent)
{
	if (RHISupportsGPUParticles(FeatureLevel))
	{
		check(FSComponent->FXSystem == this);

		RemoveFieldSampler(FSComponent->FieldSamplerInstance);
		FSComponent->FieldSamplerInstance = NULL;
	}
}

// NVCHANGE_BEGIN: JCAO - Field Sampler Module for GPU particle
void FFXSystem::AddFieldSampler(FFieldSamplerInstance* FieldSamplerInstance, const FMatrix& LocalToWorld)
{
#if WITH_APEX_TURBULENCE
	if (RHISupportsGPUParticles(FeatureLevel))
	{
		switch (FieldSamplerInstance->FSType)
		{
		case EFieldSamplerAssetType::EFSAT_GRID:
		{
			FTurbulenceFSInstance* Instance = static_cast<FTurbulenceFSInstance*>(FieldSamplerInstance);
			check(Instance);
			if (Instance)
			{
				ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
					FAddTurbulenceFSCommand,
					FFXSystem*, FXSystem, this,
					FTurbulenceFSInstance*, Instance, Instance,
					FMatrix, LocalToWorld, LocalToWorld,
					{
						if (Instance->Index == INDEX_NONE)
						{
							Instance->UpdateTransforms(LocalToWorld);
							Instance->Index = FXSystem->TurbulenceFSList.AddUninitialized().Index;
							FXSystem->TurbulenceFSList[Instance->Index] = Instance;
						}
					});
			}
		}
		break;
		case EFieldSamplerAssetType::EFSAT_ATTRACTOR:
		{
			FAttractorFSInstance* Instance = static_cast<FAttractorFSInstance*>(FieldSamplerInstance);
			check(Instance);
			if (Instance)
			{
				ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
					FAddAttractorFSCommand,
					FFXSystem*, FXSystem, this,
					FAttractorFSInstance*, Instance, Instance,
					{
						if (Instance->Index == INDEX_NONE)
						{
							Instance->Index = FXSystem->AttractorFSList.AddUninitialized().Index;
							FXSystem->AttractorFSList[Instance->Index] = Instance;
						}
					});
			}
		}
		break;
		case EFieldSamplerAssetType::EFSAT_NOISE:
		{
			FNoiseFSInstance* Instance = static_cast<FNoiseFSInstance*>(FieldSamplerInstance);
			check(Instance);
			if (Instance)
			{
				ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
					FAddNoiseFSCommand,
					FFXSystem*, FXSystem, this,
					FNoiseFSInstance*, Instance, Instance,
					{
						if (Instance->Index == INDEX_NONE)
						{
							Instance->Index = FXSystem->NoiseFSList.AddUninitialized().Index;
							FXSystem->NoiseFSList[Instance->Index] = Instance;
						}
					});
			}
		}
		break;
		default:
			check(0);
		}
	}
#endif // WITH_APEX_TURBULENCE
}

void FFXSystem::RemoveFieldSampler(FFieldSamplerInstance* Instance)
{
#if WITH_APEX_TURBULENCE
	if (RHISupportsGPUParticles(FeatureLevel))
	{
		if (Instance)
		{
			switch (Instance->FSType)
			{
			case EFieldSamplerAssetType::EFSAT_GRID:
			{
				FTurbulenceFSInstance* TurbulenceInstance = static_cast<FTurbulenceFSInstance*>(Instance);
				if (TurbulenceInstance)
				{
					ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
						FRemoveTurbulenceFSCommand,
						FFXSystem*, FXSystem, this,
						FTurbulenceFSInstance*, Instance, TurbulenceInstance,
						{
							if (Instance->Index != INDEX_NONE)
							{
								FXSystem->TurbulenceFSList.RemoveAt(Instance->Index);
								delete Instance;
							}
						});
				}
			}
			break;
			case EFieldSamplerAssetType::EFSAT_ATTRACTOR:
			{
				FAttractorFSInstance* AttractorInstance = static_cast<FAttractorFSInstance*>(Instance);
				if (AttractorInstance)
				{
					ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
						FRemoveAttractorFSCommand,
						FFXSystem*, FXSystem, this,
						FAttractorFSInstance*, Instance, AttractorInstance,
						{
							if (Instance->Index != INDEX_NONE)
							{
								FXSystem->AttractorFSList.RemoveAt(Instance->Index);
								delete Instance;
							}
						});
				}
			}
			break;
			case EFieldSamplerAssetType::EFSAT_NOISE:
			{
				FNoiseFSInstance* NoiseInstance = static_cast<FNoiseFSInstance*>(Instance);
				if (NoiseInstance)
				{
					ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
						FRemoveNoiseFSCommand,
						FFXSystem*, FXSystem, this,
						FNoiseFSInstance*, Instance, NoiseInstance,
						{
							if (Instance->Index != INDEX_NONE)
							{
								FXSystem->NoiseFSList.RemoveAt(Instance->Index);
								delete Instance;
							}
						});
				}
			}
			break;
			}
		}
	}
#endif // WITH_APEX_TURBULENCE
}
// NVCHANGE_END: JCAO - Field Sampler Module for GPU particle

void FFXSystem::UpdateFieldSampler(UFieldSamplerComponent* FSComponent)
{
#if WITH_APEX_TURBULENCE
	if (RHISupportsGPUParticles(FeatureLevel))
	{
		check(FSComponent->FXSystem == this);

		FFieldSamplerInstance* Instance = FSComponent->FieldSamplerInstance;

		if (Instance)
		{
			switch (Instance->FSType)
			{
			case EFieldSamplerAssetType::EFSAT_GRID:
			{
				struct FUpdateParams
				{
					FBox Bounds;
					FMatrix ComponentToWorld;
					bool bEnabled;
				};

				FUpdateParams UpdateParams;
				UpdateParams.Bounds = FSComponent->Bounds.GetBox();
				UpdateParams.ComponentToWorld = FSComponent->ComponentToWorld.ToMatrixWithScale();
				UpdateParams.bEnabled = FSComponent->bEnabled;

				FTurbulenceFSInstance* TurbulenceInstance = static_cast<FTurbulenceFSInstance*>(Instance);
				check(TurbulenceInstance);
				ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
					FUpdateTurbulenceFSCommand,
					FFXSystem*, FXSystem, this,
					FTurbulenceFSInstance*, Instance, TurbulenceInstance,
					FUpdateParams, UpdateParams, UpdateParams,
					{
						Instance->WorldBounds = UpdateParams.Bounds;
						Instance->UpdateTransforms(UpdateParams.ComponentToWorld);
						Instance->bEnabled = UpdateParams.bEnabled;
					});
			}
			break;
			case EFieldSamplerAssetType::EFSAT_ATTRACTOR:
			{
				FAttractorFSInstance* AttractorInstance = static_cast<FAttractorFSInstance*>(Instance);
				check(AttractorInstance);

				struct FUpdateParams
				{
					FBox Bounds;
					FVector Origin;
					float	ConstFieldStrength;
					float	VariableFieldStrength;
					bool bEnabled;
				};

				FUpdateParams UpdateParams;
				UpdateParams.Bounds = FSComponent->Bounds.GetBox();
				UpdateParams.Origin = FSComponent->ComponentToWorld.ToMatrixWithScale().GetOrigin();
				UAttractorComponent* AttractorComponent = Cast<UAttractorComponent>(FSComponent);
				check(AttractorComponent);
				UpdateParams.ConstFieldStrength = AttractorComponent->ConstFieldStrength;
				UpdateParams.VariableFieldStrength = AttractorComponent->VariableFieldStrength;
				UpdateParams.bEnabled = AttractorComponent->bEnabled;

				ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
					FUpdateAttractorFSCommand,
					FFXSystem*, FXSystem, this,
					FAttractorFSInstance*, Instance, AttractorInstance,
					FUpdateParams, UpdateParams, UpdateParams,
					{
						Instance->WorldBounds = UpdateParams.Bounds;
						Instance->Origin = UpdateParams.Origin;
						Instance->ConstFieldStrength = UpdateParams.ConstFieldStrength;
						Instance->VariableFieldStrength = UpdateParams.VariableFieldStrength;
						Instance->bEnabled = UpdateParams.bEnabled;
					});
			}
			break;
			case EFieldSamplerAssetType::EFSAT_NOISE:
			{
				FNoiseFSInstance* NoiseInstance = static_cast<FNoiseFSInstance*>(Instance);
				check(NoiseInstance);

				struct FUpdateParams
				{
					FBox Bounds;
					float NoiseStrength;
					bool bEnabled;
				};

				FUpdateParams UpdateParams;
				UpdateParams.Bounds = FSComponent->Bounds.GetBox();

				UNoiseComponent* NoiseComponent = Cast<UNoiseComponent>(FSComponent);
				check(NoiseComponent);
				UpdateParams.NoiseStrength = NoiseComponent->NoiseStrength;
				UpdateParams.bEnabled = NoiseComponent->bEnabled;

				ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
					FUpdateNoiseFSCommand,
					FFXSystem*, FXSystem, this,
					FNoiseFSInstance*, Instance, NoiseInstance,
					FUpdateParams, UpdateParams, UpdateParams,
					{
						Instance->WorldBounds = UpdateParams.Bounds;
						Instance->NoiseStrength = UpdateParams.NoiseStrength;
						Instance->bEnabled = UpdateParams.bEnabled;
					});
			}
			break;
			}
		}
	}
#endif // WITH_APEX_TURBULENCE
}
// NVCHANGE_END: JCAO - Replace vector fields with APEX turbulence velocity fields

/*-----------------------------------------------------------------------------
	Render related functionality.
-----------------------------------------------------------------------------*/

void FFXSystem::DrawDebug( FCanvas* Canvas )
{
	if (FXConsoleVariables::VisualizeGPUSimulation > 0
		&& RHISupportsGPUParticles(FeatureLevel))
	{
		VisualizeGPUParticles(Canvas);
	}
}

void FFXSystem::PreInitViews()
{
	if (RHISupportsGPUParticles(FeatureLevel))
	{
		AdvanceGPUParticleFrame();
	}
}

bool FFXSystem::UsesGlobalDistanceField() const
{
	if (RHISupportsGPUParticles(FeatureLevel))
	{
		return UsesGlobalDistanceFieldInternal();
	}

	return false;
}

void FFXSystem::PreRender(FRHICommandListImmediate& RHICmdList, const FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData)
{
	if (RHISupportsGPUParticles(FeatureLevel))
	{
		SimulateGPUParticles(RHICmdList, EParticleSimulatePhase::Main, NULL, NULL, FTexture2DRHIParamRef(), FTexture2DRHIParamRef());
		SimulateGPUParticles(RHICmdList, EParticleSimulatePhase::CollisionDistanceField, NULL, GlobalDistanceFieldParameterData, FTexture2DRHIParamRef(), FTexture2DRHIParamRef());
	}
}

void FFXSystem::PostRenderOpaque(FRHICommandListImmediate& RHICmdList, const class FSceneView* CollisionView, FTexture2DRHIParamRef SceneDepthTexture, FTexture2DRHIParamRef GBufferATexture)
{
	if (RHISupportsGPUParticles(FeatureLevel))
	{
		SimulateGPUParticles(RHICmdList, EParticleSimulatePhase::CollisionDepthBuffer, CollisionView, NULL, SceneDepthTexture, GBufferATexture);
		SortGPUParticles(RHICmdList);
	}
}
