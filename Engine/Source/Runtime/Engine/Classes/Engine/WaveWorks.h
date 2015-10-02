// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * WaveWorks
 * WaveWorks support base class.
 */

#pragma once
#include "WaveWorks.generated.h"

UENUM()
namespace WaveWorksSimulationDetailLevel
{
	enum Type
	{
		Normal, 
		High, 
		Extreme
	};
}

UCLASS(hidecategories=Object,BlueprintType,MinimalAPI)
class UWaveWorks : public UObject, public FTickableGameObject
{
	GENERATED_UCLASS_BODY()

public:
	/** Set in order to synchronize codec access to this WaveWorks resource from the render thread */
	FRenderCommandFence* ReleaseCodecFence;

	float Time;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks)
	TEnumAsByte<WaveWorksSimulationDetailLevel::Type> DetailLevel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks, meta = (ClampMin = "0.0"))
	float TimeScale;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks)
	FVector2D Wind;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WindDependency;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SmallWaveFraction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks)
	bool bUseBeaufortScale;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks, meta = (ClampMin = "0.0"))
	float WaveAmplitude;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks, meta = (ClampMin = "0.0"))
	float ChoppyScale;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FoamGenerationThreshold;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FoamGenerationAmount;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FoamDissipationSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WaveWorks, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FoamFalloffSpeed;

	// Begin UObject interface.
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	virtual void BeginDestroy() override;
#if WITH_EDITOR
	virtual void PreEditChange(UProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
private:
	void UpdateProperties();
public:
#endif // WITH_EDITOR
	virtual bool IsReadyForFinishDestroy() override;
	virtual void FinishDestroy() override;
	virtual FString GetDesc() override;
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) override;
	// End UObject interface.

	// Begin FTickableGameObject interface.
	virtual void Tick(float DeltaTime);
	virtual bool IsTickable() const;
	virtual TStatId GetStatId() const;
	// End FTickableGameObject interface

	/**
	 * Access the WaveWorks target resource for this movie texture object
	 *
	 * @return pointer to resource or NULL if not initialized
	 */
	class FWaveWorksResource* GetWaveWorksResource();

	bool PropertiesChanged() const;
	const struct GFSDK_WaveWorks_Simulation_Settings& GetSettings() const;
	const struct GFSDK_WaveWorks_Simulation_Params& GetParams() const;

public:
	class FWaveWorksResource* Resource;

	struct GFSDK_WaveWorks_Simulation_Settings* Settings; // updated in GetSettings
	struct GFSDK_WaveWorks_Simulation_Params* Params; // updated in GetParams
	FGuid Id;

	/**
	* Resets the resource for the texture.
	*/
	ENGINE_API void ReleaseResource();

	/**
	* Creates a new resource for the texture, and updates any cached references to the resource.
	*/
	virtual void UpdateResource();

};

