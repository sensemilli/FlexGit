// NVCHANGE_BEGIN: JCAO - 
/*==============================================================================
	ApexFieldSamplerActor.h: 
==============================================================================*/

#pragma once

#if WITH_APEX_TURBULENCE
namespace physx
{
	namespace apex
	{
		class NxApexActor;
		class NxApexAsset;
	}
}

class FApexFieldSamplerActor
{
public:
	ENGINE_API FApexFieldSamplerActor(UFieldSamplerComponent* Owner, FPhysScene* PhysScene, uint32 SceneType, UFieldSamplerAsset* FieldSamplerAsset);

	virtual ENGINE_API ~FApexFieldSamplerActor();

	void DeferredRelease();
	void Release();
	void OnPostFetchResults();

	physx::apex::NxApexActor* GetApexActor()
	{
		return ApexActor;
	}

	virtual void UpdateApexActor();
	virtual void UpdatePosition(const FMatrix&);

public:
	bool		bEnabled;
	int32		Index;

protected:
	virtual void DoCreate();
	virtual void DoRelease();
	virtual void DoUpdateActor() {}
	virtual void DoUpdatePos() {}

	enum
	{
		DeferredAction_Create = 0x01,
		DeferredAction_UpdateActor = 0x02,
		DeferredAction_UpdatePosition = 0x04,
		DeferredAction_Release = 0x08,
	};

	physx::apex::NxApexActor*	ApexActor;

	FPhysScene*					PhysScene;
	UFieldSamplerAsset*			FieldSamplerAsset;
	FMatrix						GlobalPose;
	uint32						DefferredAction;
	UFieldSamplerComponent*		Owner;
	FApexFieldSamplerAsset*		ApexFieldSamplerAsset;
	FRenderCommandFence			ReleaseFence;
	uint32						SceneType;
};

class FApexTurbulenceActor : public FApexFieldSamplerActor
{
public:
	ENGINE_API FApexTurbulenceActor(UFieldSamplerComponent* Owner, FPhysScene* PhysScene, uint32 SceneType, UFieldSamplerAsset* FieldSamplerAsset);

	virtual ENGINE_API ~FApexTurbulenceActor() {}

	virtual void DoUpdateActor();
	virtual void DoUpdatePos();
};

class FApexNoiseActor : public FApexFieldSamplerActor
{
public:
	ENGINE_API FApexNoiseActor(UFieldSamplerComponent* Owner, FPhysScene* PhysScene, uint32 SceneType, UFieldSamplerAsset* FieldSamplerAsset);

	virtual ENGINE_API ~FApexNoiseActor() {}

	virtual void DoUpdateActor();
	virtual void DoUpdatePos();

	float NoiseStrength;
};

class FApexVortexActor : public FApexFieldSamplerActor
{
public:
	ENGINE_API FApexVortexActor(UFieldSamplerComponent* Owner, FPhysScene* PhysScene, uint32 SceneType, UFieldSamplerAsset* FieldSamplerAsset);

	virtual ENGINE_API ~FApexVortexActor() {}

	virtual void DoUpdateActor();
	virtual void DoUpdatePos();

	float RotationalFieldStrength;
	float RadialFieldStrength;
	float LiftFieldStrength;
};

class FApexJetActor : public FApexFieldSamplerActor
{
public:
	ENGINE_API FApexJetActor(UFieldSamplerComponent* Owner, FPhysScene* PhysScene, uint32 SceneType, UFieldSamplerAsset* FieldSamplerAsset);

	virtual ENGINE_API ~FApexJetActor() {}

	virtual void DoUpdateActor();
	virtual void DoUpdatePos();

	float FieldStrength;
};

class FApexVelocitySourceActor : public FApexFieldSamplerActor
{
public:
	ENGINE_API FApexVelocitySourceActor(UFieldSamplerComponent* Owner, FPhysScene* PhysScene, uint32 SceneType, UFieldSamplerAsset* FieldSamplerAsset);

	virtual ENGINE_API ~FApexVelocitySourceActor() {}

	virtual void DoUpdateActor();
	virtual void DoUpdatePos();

	float AverageVelocity;
	float StandardVelocity;
};

class FApexHeatSourceActor : public FApexFieldSamplerActor
{
public:
	ENGINE_API FApexHeatSourceActor(UFieldSamplerComponent* Owner, FPhysScene* PhysScene, uint32 SceneType, UFieldSamplerAsset* FieldSamplerAsset);

	virtual ENGINE_API ~FApexHeatSourceActor() {}

	virtual void DoUpdateActor();
	virtual void DoUpdatePos();

	float AverageTemperature;
	float StandardTemperature;
};
#endif
// NVCHANGE_END: JCAO - 