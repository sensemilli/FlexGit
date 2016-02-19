// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/**
 *	APEX Turbulence - Grid Component
 */
#pragma once

#include "FieldSampler/FieldSamplerComponent.h"
#include "GridComponent.generated.h"

UCLASS(ClassGroup=Physics, config=Engine, editinlinenew, meta=(BlueprintSpawnableComponent),MinimalAPI)
class UGridComponent : public UFieldSamplerComponent
{
	GENERATED_UCLASS_BODY()

	/** The field sampler asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Grid)
	class UGridAsset* GridAsset;

	virtual void SetFieldSamplerAsset( class UFieldSamplerAsset* Asset ) override;
	virtual void OnDeferredCreated();

#if WITH_APEX_TURBULENCE
	virtual void SetupApexActorParams(NxParameterized::Interface* ActorParams, physx::PxFilterData& PQueryFilterData) override;
#endif
protected:
#if WITH_APEX_TURBULENCE
	virtual void UpdateApexActor() override;
	virtual class UFieldSamplerAsset* GetFieldSamplerAsset() override;
	virtual bool CreateFieldSamplerInstance() override;
	virtual void CreateApexActor(FPhysScene* InPhysScene, class UFieldSamplerAsset* InFieldSamplerAsset) override;
#endif

	// Begin UActorComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const override;
	virtual void SendRenderTransform_Concurrent() override;
	// End UActorComponent Interface
};


// NVCHANGE_END: JCAO - Add Turbulence actors and components