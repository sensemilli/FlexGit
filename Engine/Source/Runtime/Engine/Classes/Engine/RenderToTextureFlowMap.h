

#pragma once

#include "GameFramework/Actor.h"
#include "RenderToTextureFlowMap.generated.h"

class UMaterialInstanceDynamic;
class UCanvasRenderTarget2D;

/**
 * Helper class to implement texture coordinate advection.
 * The coordinate map is advected by binding it to a user provided material and rendering
 * that material into a canvas render target. The result acts as input during the next timestep.
 * The map needs to be reset regularly to limit the amount of coordinate skew
 * accumulated during advection. For this, three coordinate maps with staggered reset timers
 * are advected simultaneously, allowing spatially varying blend phase offsets.
 */
UCLASS()
class ARenderToTextureFlowMap : public AActor
{
	GENERATED_UCLASS_BODY()

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void BindResetInstance(int Index);
	void BindSourceInstance(int Index);

	UFUNCTION()
	void OnReceiveUpdate(UCanvas* Canvas, int32 Width, int32 Height);

	UPROPERTY(EditDefaultsOnly, Category = Default)
	float BlendTimeScale;

	UPROPERTY(EditDefaultsOnly, Category = Default)
	int32 SurfaceWidth;

	UPROPERTY(EditDefaultsOnly, Category = Default)
	int32 SurfaceHeight;

	UPROPERTY(EditDefaultsOnly, Category = Default)
	FString ParameterBaseName;

	UPROPERTY(EditDefaultsOnly, Category = Materials)
	UMaterialInterface* ResetMaterial;

	UPROPERTY(EditDefaultsOnly, Category = Materials)
	UMaterialInterface* SourceMaterial;

	UPROPERTY(EditAnywhere, Category = Materials)
	UMaterialInterface* TargetMaterial;

	UPROPERTY(BlueprintReadOnly, Category = Materials)
	UMaterialInstanceDynamic* TargetInstance;

	UPROPERTY()
	TArray<UCanvasRenderTarget2D*> RenderTargets;

	UPROPERTY()
	UMaterialInstanceDynamic* ResetInstance;

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> SourceInstances;

	// Transient, to pass material (ResetMaterial or SourceInstance) to OnReceiveUpdate()
	UMaterialInterface* RenderSource;

	float BlendTime;
	int FlipFlop;

	UFUNCTION(BlueprintCallable, Category = Render)
	static void RenderMaterial(UMaterialInterface* Material, UCanvas* Canvas, int32 Width, int32 Height);
};
