// NVCHANGE_BEGIN: JCAO - Grid Density with GPU particles
/*==============================================================================
	ParticleModules_Density.cpp: Density module implementations.
==============================================================================*/

#include "EnginePrivate.h"
#include "Distributions/DistributionFloatParticleParameter.h"
#include "Distributions/DistributionVectorParticleParameter.h"
#include "ParticleDefinitions.h"
#include "Particles/Density/ParticleModuleDensityBase.h"
#include "Particles/Density/ParticleModuleColorOverDensity.h"
#include "Particles/Density/ParticleModuleSizeOverDensity.h"



/*------------------------------------------------------------------------------
	Base Density.
------------------------------------------------------------------------------*/
UParticleModuleDensityBase::UParticleModuleDensityBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

/*-----------------------------------------------------------------------------
	UParticleModuleColorOverDensity implementation.
-----------------------------------------------------------------------------*/
UParticleModuleColorOverDensity::UParticleModuleColorOverDensity(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCurvesAsColor = true;
	bClampAlpha = true;
}

void UParticleModuleColorOverDensity::InitializeDefaults()
{
	if (!ColorOverDensity.Distribution)
	{
		ColorOverDensity.Distribution = NewNamedObject<UDistributionVectorConstantCurve>(this, TEXT("DistributionColorOverDensity"));
	}

	if (!AlphaOverDensity.Distribution)
	{
		UDistributionFloatConstant* DistributionAlphaOverDensity = NewNamedObject<UDistributionFloatConstant>(this, TEXT("DistributionAlphaOverDensity"));
		DistributionAlphaOverDensity->Constant = 1.0f;
		AlphaOverDensity.Distribution = DistributionAlphaOverDensity;
	}
}

void UParticleModuleColorOverDensity::PostInitProperties()
{
	Super::PostInitProperties();
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		ColorOverDensity.Distribution = NewNamedObject<UDistributionVectorConstantCurve>(this, TEXT("DistributionColorOverDensity"));

		UDistributionFloatConstant* DistributionAlphaOverDensity = NewNamedObject<UDistributionFloatConstant>(this, TEXT("DistributionAlphaOverDensity"));
		DistributionAlphaOverDensity->Constant = 1.0f;
		AlphaOverDensity.Distribution = DistributionAlphaOverDensity;
	}
}

void UParticleModuleColorOverDensity::CompileModule( FParticleEmitterBuildInfo& EmitterInfo )
{
	EmitterInfo.bColorOverDensity = true;
	EmitterInfo.DensityColor.Initialize( ColorOverDensity.Distribution );
	EmitterInfo.DensityAlpha.Initialize( AlphaOverDensity.Distribution );
}

#if WITH_EDITOR
void UParticleModuleColorOverDensity::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	InitializeDefaults();

	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if (PropertyThatChanged)
	{
		if (PropertyThatChanged->GetFName() == FName(TEXT("bClampAlpha")))
		{
			UObject* OuterObj = GetOuter();
			check(OuterObj);
			UParticleLODLevel* LODLevel = Cast<UParticleLODLevel>(OuterObj);
			if (LODLevel)
			{
				// The outer is incorrect - warn the user and handle it
				UE_LOG(LogParticles, Warning, TEXT("UParticleModuleColorOverDensity has an incorrect outer... run FixupEmitters on package %s"),
					*(OuterObj->GetOutermost()->GetPathName()));
				OuterObj = LODLevel->GetOuter();
				UParticleEmitter* Emitter = Cast<UParticleEmitter>(OuterObj);
				check(Emitter);
				OuterObj = Emitter->GetOuter();
			}
			UParticleSystem* PartSys = CastChecked<UParticleSystem>(OuterObj);

			PartSys->UpdateDensityModuleClampAlpha(this);
		}
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

bool UParticleModuleColorOverDensity::AddModuleCurvesToEditor(UInterpCurveEdSetup* EdSetup, TArray<const FCurveEdEntry*>& OutCurveEntries)
{
	bool bNewCurve = false;
#if WITH_EDITORONLY_DATA
	// Iterate over object and find any InterpCurveFloats or UDistributionFloats
	for (TFieldIterator<UStructProperty> It(GetClass()); It; ++It)
	{
		// attempt to get a distribution from a random struct property
		UObject* Distribution = FRawDistribution::TryGetDistributionObjectFromRawDistributionProperty(*It, (uint8*)this);
		if (Distribution)
		{
			FCurveEdEntry* Curve = NULL;
			if(Distribution->IsA(UDistributionFloat::StaticClass()))
			{
				// We are assuming that this is the alpha...
				if (bClampAlpha == true)
				{
					bNewCurve |= EdSetup->AddCurveToCurrentTab(Distribution, It->GetName(), ModuleEditorColor, &Curve, true, true, true, 0.0f, 1.0f);
				}
				else
				{
					bNewCurve |= EdSetup->AddCurveToCurrentTab(Distribution, It->GetName(), ModuleEditorColor, &Curve, true, true);
				}
			}
			else
			{
				// We are assuming that this is the color...
				bNewCurve |= EdSetup->AddCurveToCurrentTab(Distribution, It->GetName(), ModuleEditorColor, &Curve, true, true);
			}
			OutCurveEntries.Add( Curve );
		}
	}
#endif // WITH_EDITORONLY_DATA
	return bNewCurve;
}

void UParticleModuleColorOverDensity::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	ColorOverDensity.Distribution = Cast<UDistributionVectorConstantCurve>(StaticConstructObject(UDistributionVectorConstantCurve::StaticClass(), this));
	UDistributionVectorConstantCurve* ColorOverDensityDist = Cast<UDistributionVectorConstantCurve>(ColorOverDensity.Distribution);
	if (ColorOverDensityDist)
	{
		// Add two points, one at time 0.0f and one at 1.0f
		for (int32 Key = 0; Key < 2; Key++)
		{
			int32	KeyIndex = ColorOverDensityDist->CreateNewKey(Key * 1.0f);
			for (int32 SubIndex = 0; SubIndex < 3; SubIndex++)
			{
				if (Key == 0)
				{
					ColorOverDensityDist->SetKeyOut(SubIndex, KeyIndex, 1.0f);
				}
				else
				{
					ColorOverDensityDist->SetKeyOut(SubIndex, KeyIndex, 0.0f);
				}
			}
		}
		ColorOverDensityDist->bIsDirty = true;
	}

	AlphaOverDensity.Distribution = Cast<UDistributionFloatConstantCurve>(StaticConstructObject(UDistributionFloatConstantCurve::StaticClass(), this));
	UDistributionFloatConstantCurve* AlphaOverDensityDist = Cast<UDistributionFloatConstantCurve>(AlphaOverDensity.Distribution);
	if (AlphaOverDensityDist)
	{
		// Add two points, one at time 0.0f and one at 1.0f
		for (int32 Key = 0; Key < 2; Key++)
		{
			int32	KeyIndex = AlphaOverDensityDist->CreateNewKey(Key * 1.0f);
			if (Key == 0)
			{
				AlphaOverDensityDist->SetKeyOut(0, KeyIndex, 1.0f);
			}
			else
			{
				AlphaOverDensityDist->SetKeyOut(0, KeyIndex, 0.0f);
			}
		}
		AlphaOverDensityDist->bIsDirty = true;
	}
}

#if WITH_EDITOR
bool UParticleModuleColorOverDensity::IsValidForLODLevel(UParticleLODLevel* LODLevel, FString& OutErrorString)
{
	if (LODLevel->TypeDataModule && LODLevel->TypeDataModule->IsA(UParticleModuleTypeDataGpu::StaticClass()))
	{
		if(!IsDistributionAllowedOnGPU(ColorOverDensity.Distribution))
		{
			OutErrorString = GetDistributionNotAllowedOnGPUText(StaticClass()->GetName(), "ColorOverDensity" ).ToString();
			return false;
		}

		if(!IsDistributionAllowedOnGPU(AlphaOverDensity.Distribution))
		{
			OutErrorString = GetDistributionNotAllowedOnGPUText(StaticClass()->GetName(), "AlphaOverDensity" ).ToString();
			return false;
		}
	}

	return true;
}
#endif

/*-----------------------------------------------------------------------------
	UParticleModuleSizeOverDensity implementation.
-----------------------------------------------------------------------------*/
UParticleModuleSizeOverDensity::UParticleModuleSizeOverDensity(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UParticleModuleSizeOverDensity::InitializeDefaults()
{
	if (!SizeOverDensity.Distribution)
	{
		SizeOverDensity.Distribution = NewNamedObject<UDistributionVectorConstant>(this, TEXT("DistributionSizeOverDensity"));
	}
}

void UParticleModuleSizeOverDensity::PostInitProperties()
{
	Super::PostInitProperties();
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		InitializeDefaults();
	}
}

void UParticleModuleSizeOverDensity::CompileModule( FParticleEmitterBuildInfo& EmitterInfo )
{
	EmitterInfo.bSizeOverDensity = true;
	EmitterInfo.DensitySize.Initialize( SizeOverDensity.Distribution );
}

#if WITH_EDITOR
void UParticleModuleSizeOverDensity::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	InitializeDefaults();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool UParticleModuleSizeOverDensity::IsValidForLODLevel(UParticleLODLevel* LODLevel, FString& OutErrorString)
{
	if (LODLevel->TypeDataModule && LODLevel->TypeDataModule->IsA(UParticleModuleTypeDataGpu::StaticClass()))
	{
		if(!IsDistributionAllowedOnGPU(SizeOverDensity.Distribution))
		{
			OutErrorString = GetDistributionNotAllowedOnGPUText(StaticClass()->GetName(), "SizeOverDensity" ).ToString();
			return false;
		}
	}

	return true;
}

#endif // WITH_EDITOR


void UParticleModuleSizeOverDensity::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	SizeOverDensity.Distribution = Cast<UDistributionVectorConstantCurve>(StaticConstructObject(UDistributionVectorConstantCurve::StaticClass(), this));
	UDistributionVectorConstantCurve* SizeOverDensityDist = Cast<UDistributionVectorConstantCurve>(SizeOverDensity.Distribution);
	if (SizeOverDensityDist)
	{
		// Add two points, one at time 0.0f and one at 1.0f
		for (int32 Key = 0; Key < 2; Key++)
		{
			int32	KeyIndex = SizeOverDensityDist->CreateNewKey(Key * 1.0f);
			for (int32 SubIndex = 0; SubIndex < 3; SubIndex++)
			{
				SizeOverDensityDist->SetKeyOut(SubIndex, KeyIndex, 1.0f);
			}
		}
		SizeOverDensityDist->bIsDirty = true;
	}
}

// NVCHANGE_END: JCAO - Grid Density with GPU particles