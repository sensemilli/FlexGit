// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/**
 *	Base component for APEX Turbulence
 */
#pragma once
#include "FieldSamplerComponent.generated.h"

#if WITH_PHYSX
namespace physx
{
#if WITH_APEX
	namespace apex
	{
		class	NxApexActor;
	}
#endif

	struct PxFilterData;
}
#endif
#if WITH_APEX
namespace NxParameterized
{
	class Interface;
}
#endif

class FApexFieldSamplerActor;

#define COLLISION_FILTER_DATA_NAME "CollisionFilterData"
#define FIELD_SAMPLER_FILTER_DATA_NAME "FieldSamplerFilterData"
#define FIELD_BOUNDARY_FILTER_DATA_NAME "FieldBoundaryFilterData"

UENUM()
enum ETurbulenceViewMode
{
	ETVM_VelocityField,
	ETVM_Streamlines,
	ETVM_BBox,
	ETVM_Jet,
	ETVM_Noise,
	ETVM_Vortex,

	ETVM_Max
};

UCLASS(ClassGroup=Physics, hidecategories=(Object,Mesh,Physics,Lighting), config=Engine, editinlinenew, abstract)
class ENGINE_API UFieldSamplerComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	/** Enable/Disable Field Sampler */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=FieldSampler)
	uint32	bEnabled : 1;

	/** Duration in seconds the field sampler should be exist.  0 = Infinite*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=FieldSampler)
	float   Duration;

	UFUNCTION(BlueprintCallable, Category=FieldSampler)
	void SetEnabled(bool Enabled);

	virtual bool CreateFieldSamplerInstance()
	{
		return false;
	}

	virtual void RemoveFieldSamplerInstance();

	/** The FX system with which this field sampler is associated. */
	class FFXSystemInterface* FXSystem;
	class FFieldSamplerInstance* FieldSamplerInstance;

	// UObject interface
	virtual void CreatePhysicsState() override;
	virtual void DestroyPhysicsState() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void SetFieldSamplerAsset( class UFieldSamplerAsset* ) {}

#if WITH_EDITOR	
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
#endif
	// End of UObject interface

	// USceneComponent interface
	virtual void OnUpdateTransform(bool bSkipPhysicsMove, ETeleportType Teleport = ETeleportType::None) override;

	virtual void OnDeferredCreated() {}


#if WITH_APEX_TURBULENCE
	virtual void SetupApexActorParams(NxParameterized::Interface* ActorParams, physx::PxFilterData& PQueryFilterData);
#endif
protected:

	virtual void UpdateApexActor() {}
	virtual class UFieldSamplerAsset* GetFieldSamplerAsset() { return 0; }
	void CreateApexObjects();
	void DestroyApexObjects();

	virtual void CreateApexActor(FPhysScene* InPhysScene, class UFieldSamplerAsset* InFieldSamplerAsset) {}

	/** Internal variable for storing the elapsed time of the field sampler */
	float   ElapsedTime;

#if WITH_APEX_TURBULENCE
	FApexFieldSamplerActor* ApexFieldSamplerActor;
#endif	//WITH_APEX_TURBULENCE
};



// NVCHANGE_END: JCAO - Add Turbulence actors and components