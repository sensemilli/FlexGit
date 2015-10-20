// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WaveWorks.cpp: UWaveWorks implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "GFSDK_WaveWorks.h"
#include "WaveWorksResource.h"

/*-----------------------------------------------------------------------------
UWaveWorks
-----------------------------------------------------------------------------*/

UWaveWorks::UWaveWorks(const class FObjectInitializer& PCIP)
	: Super(PCIP)
	, Time(0.0f)
	, Settings(new GFSDK_WaveWorks_Simulation_Settings)
	, Params(new GFSDK_WaveWorks_Simulation_Params)
	, Id(FGuid::NewGuid())
{
	memset(Settings, 0, sizeof(*Settings));
	memset(Params, 0, sizeof(*Params));

	DetailLevel = WaveWorksSimulationDetailLevel::High;
	TimeScale = 0.25f;
	Wind = { 2.0f, 1.0f };
	WindDependency = 0.95f;
	SmallWaveFraction = 0.0f;
	bUseBeaufortScale = true;
	WaveAmplitude = 0.8f;
	ChoppyScale = 1.2f;
	FoamGenerationThreshold = 0.0f;
	FoamGenerationAmount = 0.8f;
	FoamDissipationSpeed = 0.05f;
	FoamFalloffSpeed = 0.95f;
}

void UWaveWorks::BeginDestroy()
{
	// this will also release the FWaveWorksResource
	Super::BeginDestroy();

	delete Settings;
	Settings = nullptr;

	delete Params;
	Params = nullptr;

	// synchronize with the rendering thread by inserting a fence
	if( !ReleaseCodecFence )
	{
		ReleaseCodecFence = new FRenderCommandFence();
	}
	ReleaseCodecFence->BeginFence();
}

bool UWaveWorks::IsReadyForFinishDestroy()
{
	// ready to call FinishDestroy if the codec flushing fence has been hit
	return(
		Super::IsReadyForFinishDestroy() &&
		ReleaseCodecFence &&
		ReleaseCodecFence->IsFenceComplete()
		);
}

void UWaveWorks::FinishDestroy()
{
	delete ReleaseCodecFence;
	ReleaseCodecFence = NULL;

	Super::FinishDestroy();
}

#if WITH_EDITOR
void UWaveWorks::PreEditChange(UProperty* PropertyAboutToChange)
{
	// this will release the FWaveWorksResource
	Super::PreEditChange(PropertyAboutToChange);

	// synchronize with the rendering thread by flushing all render commands
	FlushRenderingCommands();
}

void UWaveWorks::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property->GetName() == TEXT("bUseBeaufortScale"))
	{
		UpdateProperties();
	}

	// this will recreate the FWaveWorksResource
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UWaveWorks::UpdateProperties()
{
	void (UProperty::*UpdatePropertyFlag)(uint64) = bUseBeaufortScale ?
		&UProperty::SetPropertyFlags : &UProperty::ClearPropertyFlags;

	for (TFieldIterator<UProperty> PropIt(GetClass()); PropIt; ++PropIt)
	{
		UProperty* Property = *PropIt;
		FString Name = Property->GetName();
		if (Name.Equals(TEXT("WaveAmplitude")) || 
			Name.Equals(TEXT("ChoppyScale")) ||
			Name.StartsWith(TEXT("Foam")))
			(Property->*UpdatePropertyFlag)(CPF_EditConst);
	}
}
#endif // WITH_EDITOR


void UWaveWorks::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	UpdateProperties();
#endif

	if ( !HasAnyFlags(RF_ClassDefaultObject) && !GIsBuildMachine)  // we won't initialize this on build machines
	{
		// recreate the FWaveWorksResource
		this->UpdateResource();
	}
}


void UWaveWorks::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << Id;
}

FString UWaveWorks::GetDesc()
{
	return FString::Printf(
		TEXT("WaveWorks")
		);
}


SIZE_T UWaveWorks::GetResourceSize(EResourceSizeMode::Type Mode)
{
	int32 ResourceSize = 0;

	return ResourceSize;
}

void UWaveWorks::Tick(float DeltaTime)
{
	Time += DeltaTime;
}

bool UWaveWorks::IsTickable() const
{
	return true;
}

TStatId UWaveWorks::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UWaveWorks, STATGROUP_Tickables);
}

class FWaveWorksResource* UWaveWorks::GetWaveWorksResource()
{
	FWaveWorksResource* Result = NULL;
	if( Resource &&
		Resource->IsInitialized() )
	{
		Result = (FWaveWorksResource*)Resource;
	}
	return Result;
}

bool UWaveWorks::PropertiesChanged() const
{
	FVector2D WindDir = Wind.GetSafeNormal();
	return Settings->detail_level != (GFSDK_WaveWorks_Simulation_DetailLevel)(int)DetailLevel
		|| Settings->use_Beaufort_scale != bUseBeaufortScale
		|| Params->wave_amplitude != WaveAmplitude
		|| Params->wind_dir.x != WindDir.X
		|| Params->wind_dir.y != WindDir.Y
		|| Params->wind_speed != Wind.Size()
		|| Params->wind_dependency != WindDependency
		|| Params->choppy_scale != ChoppyScale
		|| Params->small_wave_fraction != SmallWaveFraction
		|| Params->time_scale != TimeScale
		|| Params->foam_generation_threshold != FoamGenerationThreshold
		|| Params->foam_generation_amount != FoamGenerationAmount
		|| Params->foam_dissipation_speed != FoamDissipationSpeed
		|| Params->foam_falloff_speed != FoamFalloffSpeed;
}

const GFSDK_WaveWorks_Simulation_Settings& UWaveWorks::GetSettings() const
{
	Settings->detail_level = (GFSDK_WaveWorks_Simulation_DetailLevel)(int)DetailLevel;
	Settings->fft_period = 4000.0f;
	Settings->use_Beaufort_scale = bUseBeaufortScale;
	Settings->readback_displacements = false;
	Settings->aniso_level = 4;
	Settings->CPU_simulation_threading_model = GFSDK_WaveWorks_Simulation_CPU_Threading_Model_Automatic;
	Settings->num_GPUs = 0;

	return *Settings;
}

const GFSDK_WaveWorks_Simulation_Params& UWaveWorks::GetParams() const
{
	Params->wave_amplitude = WaveAmplitude;
	FVector2D WindDir = Wind.GetSafeNormal();
	Params->wind_dir = { WindDir.X, WindDir.Y };
	Params->wind_speed = Wind.Size();
	Params->wind_dependency = WindDependency;
	Params->choppy_scale = ChoppyScale;
	Params->small_wave_fraction = SmallWaveFraction;
	Params->time_scale = TimeScale;
	Params->foam_generation_threshold = FoamGenerationThreshold;
	Params->foam_generation_amount = FoamGenerationAmount;
	Params->foam_dissipation_speed = FoamDissipationSpeed;
	Params->foam_falloff_speed = FoamFalloffSpeed;

	return *Params;
}

void UWaveWorks::ReleaseResource()
{
	if (Resource)
	{
		// Free the resource.
		ReleaseResourceAndFlush(Resource);
		delete Resource;
		Resource = NULL;
	}
}

void UWaveWorks::UpdateResource()
{
	// Release the existing texture resource.
	ReleaseResource();

	//Dedicated servers have no texture internals
	if (FApp::CanEverRender() && !HasAnyFlags(RF_ClassDefaultObject))
	{
		// Create a new texture resource.
		Resource = new FWaveWorksResource(this);
		if (Resource)
		{
			BeginInitResource(Resource);
		}
	}
}

/*-----------------------------------------------------------------------------
FWaveWorksResource
-----------------------------------------------------------------------------*/

/**
* Initializes the dynamic RHI resource and/or RHI render target used by this resource.
* Called when the resource is initialized, or when reseting all RHI resources.
* This is only called by the rendering thread.
*/
void FWaveWorksResource::InitDynamicRHI()
{
	WaveWorksRHI = GDynamicRHI->RHIGetDefaultContext()->RHICreateWaveWorks(Owner->GetSettings(), Owner->GetParams());

	// add to the list of global deferred updates (updated during scene rendering)
	AddToDeferredUpdateList(false);
}

/**
* Release the dynamic RHI resource and/or RHI render target used by this resource.
* Called when the resource is released, or when reseting all RHI resources.
* This is only called by the rendering thread.
*/
void FWaveWorksResource::ReleaseDynamicRHI()
{
	// release the FTexture RHI resources here as well
	ReleaseRHI();

	WaveWorksRHI.SafeRelease();

	// remove from global list of deferred updates
	RemoveFromDeferredUpdateList();
}

/**
* Updates the WaveWorks simulation.
* This is only called by the rendering thread.
*/
void FWaveWorksResource::UpdateDeferredResource(FRHICommandListImmediate & CMLI, bool bClearRenderTarget)
{
	SCOPED_DRAW_EVENTF(CMLI, DEC_SCENE_ITEMS, TEXT("WaveWorks[%s]"), *Owner->GetName());

	if (Owner->PropertiesChanged())
		GFSDK_WaveWorks_Simulation_UpdateProperties(WaveWorksRHI->Simulation, Owner->GetSettings(), Owner->GetParams());
	GFSDK_WaveWorks_Simulation_SetTime(WaveWorksRHI->Simulation, Owner->Time);
	WaveWorksRHI->UpdateTick();
}
