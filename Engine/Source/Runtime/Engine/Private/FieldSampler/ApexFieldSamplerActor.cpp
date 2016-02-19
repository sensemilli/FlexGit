// NVCHANGE_BEGIN: JCAO - 

#include "EnginePrivate.h"
#include "ApexFieldSamplerActor.h"
#include "ApexFieldSamplerAsset.h"
#include "PhysicsPublic.h"
#include "../PhysicsEngine/PhysXSupport.h"


#if WITH_APEX_TURBULENCE

FApexFieldSamplerActor::FApexFieldSamplerActor(UFieldSamplerComponent* InOwner, FPhysScene* InPhysScene, uint32 InSceneType, UFieldSamplerAsset* InFieldSamplerAsset)
	: Index(INDEX_NONE)
	, bEnabled(false)
	, Owner(InOwner)
	, PhysScene(InPhysScene)
	, FieldSamplerAsset(InFieldSamplerAsset)
	, ApexFieldSamplerAsset(InFieldSamplerAsset->ApexFieldSamplerAsset)
	, DefferredAction(0)
	, ApexActor(0)
	, SceneType(InSceneType)
{
	if (PhysScene)
	{
		Index = PhysScene->ApexFieldSamplerActorsList[SceneType].AddUninitialized().Index;
		PhysScene->ApexFieldSamplerActorsList[SceneType][ Index ] = this;

		DefferredAction |= DeferredAction_Create;
	}
}

FApexFieldSamplerActor::~FApexFieldSamplerActor()
{
}

void FApexFieldSamplerActor::DeferredRelease()
{
	// Owner and FieldSamplerAsset can't be trust anymore when starting release
	Owner = NULL;
	FieldSamplerAsset = NULL;

	if (static_cast<FApexTurbulenceActor*>(this) != NULL)
	{
		ReleaseFence.BeginFence();
	}
	DefferredAction |= DeferredAction_Release;
}

void FApexFieldSamplerActor::Release()
{
	// Owner and FieldSamplerAsset can't be trust anymore when starting release
	Owner = NULL;
	FieldSamplerAsset = NULL;
	DoRelease();
}

void FApexFieldSamplerActor::OnPostFetchResults()
{
	if (DefferredAction & DeferredAction_Release)
	{
		bool bReadyForRelease = static_cast<FApexTurbulenceActor*>(this) != NULL ? ReleaseFence.IsFenceComplete() : 1;

		if(bReadyForRelease)
		{
			DoRelease();
		}
		return;
	}

	if (DefferredAction & DeferredAction_Create)
	{
		DoCreate();
		DefferredAction &= ~DeferredAction_Create;
	}
	if (DefferredAction & DeferredAction_UpdateActor)
	{
		DoUpdateActor();
		DefferredAction &= ~DeferredAction_UpdateActor;
	}
	if (DefferredAction & DeferredAction_UpdatePosition)
	{
		DoUpdatePos();
		DefferredAction &= ~DeferredAction_UpdatePosition;
	}
}


void FApexFieldSamplerActor::UpdateApexActor()
{
	DefferredAction |= DeferredAction_UpdateActor;
}

void FApexFieldSamplerActor::UpdatePosition(const FMatrix& InGlobalPose)
{
	GlobalPose = InGlobalPose;
	DefferredAction |= DeferredAction_UpdatePosition;
}

void FApexFieldSamplerActor::DoCreate()
{
	if (SceneType == PST_Async && !PhysScene->HasAsyncScene())
	{
		return;
	}
	
	NxApexScene* ApexScene = PhysScene->GetApexScene(SceneType);
	check(ApexScene);
	check(FieldSamplerAsset);
	NxParameterized::Interface* ActorParams = FieldSamplerAsset->GetDefaultActorDesc();
	if (ActorParams)
	{
		PxFilterData PQueryFilterData;
		if(Owner)
		{
			Owner->SetupApexActorParams(ActorParams, PQueryFilterData);
		}
		else
		{
			physx::PxMat44 Pose = U2PMatrix(FMatrix::Identity);
			verify(NxParameterized::setParamMat34(*ActorParams, "initialPose", Pose));
			verify(NxParameterized::setParamString(*ActorParams, "fieldSamplerFilterDataName", FIELD_SAMPLER_FILTER_DATA_NAME));
			verify(NxParameterized::setParamString(*ActorParams, "fieldBoundaryFilterDataName", FIELD_BOUNDARY_FILTER_DATA_NAME));
			
			PQueryFilterData.word0 = 0;
			PQueryFilterData.word1 = 0xFFFF;
			PQueryFilterData.word2 = 0;
			PQueryFilterData.word3 |=  (0x1 << 24);
		}
		GApexSDK->getNamedResourceProvider()->setResource(APEX_COLLISION_GROUP_128_NAME_SPACE, COLLISION_FILTER_DATA_NAME, (void*)&PQueryFilterData);
		GApexSDK->getNamedResourceProvider()->setResource(APEX_COLLISION_GROUP_128_NAME_SPACE, FIELD_SAMPLER_FILTER_DATA_NAME, (void*)&PQueryFilterData);
		GApexSDK->getNamedResourceProvider()->setResource(APEX_COLLISION_GROUP_128_NAME_SPACE, FIELD_BOUNDARY_FILTER_DATA_NAME, (void*)&PQueryFilterData);

		ApexActor = FieldSamplerAsset->ApexFieldSamplerAsset->GetApexAsset()->createApexActor(*ActorParams, *ApexScene);
		check(ApexActor);
		FieldSamplerAsset->ApexFieldSamplerAsset->IncreaseRefCount();
		if(Owner)
		{
			Owner->OnDeferredCreated();
		}
	}
}

void FApexFieldSamplerActor::DoRelease()
{
	if (ApexActor)
	{
		ApexActor->release();
		ApexActor = NULL;
		ApexFieldSamplerAsset->DecreaseRefCount();
	}

	PhysScene->ApexFieldSamplerActorsList[SceneType].RemoveAt( Index );
	delete this;
}


FApexTurbulenceActor::FApexTurbulenceActor(UFieldSamplerComponent* InOwner, FPhysScene* InPhysScene, uint32 InSceneType, UFieldSamplerAsset* InFieldSamplerAsset)
	: FApexFieldSamplerActor(InOwner, InPhysScene, InSceneType, InFieldSamplerAsset)
{
}

void FApexTurbulenceActor::DoUpdateActor()
{
	physx::apex::NxTurbulenceFSActor* ApexFSActor = static_cast<physx::apex::NxTurbulenceFSActor*>(ApexActor);

	UGridAsset* GridAsset = Cast<UGridAsset>(FieldSamplerAsset);

	if (ApexFSActor && GridAsset)
	{
		ApexFSActor->setEnabled(bEnabled);

		ApexFSActor->setAngularVelocityMultiplierAndClamp(GridAsset->AngularVelocityMultiplier, GridAsset->AngularVelocityClamp);
		ApexFSActor->setLinearVelocityMultiplierAndClamp(GridAsset->LinearVelocityMultiplier, GridAsset->LinearVelocityClamp);

		ApexFSActor->setExternalVelocity(U2PVector(GridAsset->ExternalVelocity));

		ApexFSActor->setFieldVelocityMultiplier(GridAsset->FieldVelocityMultiplier);
		ApexFSActor->setFieldVelocityWeight(GridAsset->FieldVelocityWeight);

		float DefaultThermalConductivity = 0.0f;
		ApexFSActor->setHeatBasedParameters(GridAsset->TemperatureBasedForceMultiplier, GridAsset->AmbientTemperature, U2PVector(GridAsset->HeatForceDirection), DefaultThermalConductivity);

		ApexFSActor->setNoiseParameters(GridAsset->NoiseStrength, U2PVector(GridAsset->NoiseSpacePeriod), GridAsset->NoiseTimePeriod, GridAsset->NoiseOctaves);

		ApexFSActor->setVelocityFieldCleaningTime(GridAsset->VelocityFieldFadeTime);
		ApexFSActor->setVelocityFieldCleaningDelay(GridAsset->VelocityFieldFadeDelay);
		ApexFSActor->setVelocityFieldCleaningIntensity(GridAsset->VelocityFieldFadeIntensity);
	}
}

void FApexTurbulenceActor::DoUpdatePos()
{
	physx::apex::NxTurbulenceFSActor* ApexFSActor = static_cast<physx::apex::NxTurbulenceFSActor*>(ApexActor);
	if (ApexFSActor != NULL)
	{
		physx::PxMat44 Pose = U2PMatrix(GlobalPose);
		ApexFSActor->setPose(Pose);
	}
}

FApexJetActor::FApexJetActor(UFieldSamplerComponent* InOwner, FPhysScene* InPhysScene, uint32 InSceneType, UFieldSamplerAsset* InFieldSamplerAsset)
	: FApexFieldSamplerActor(InOwner, InPhysScene, InSceneType, InFieldSamplerAsset)
{
}

void FApexJetActor::DoUpdateActor()
{
	physx::apex::NxJetFSActor* ApexFSActor = static_cast<physx::apex::NxJetFSActor*>(ApexActor);
	UJetAsset* JetAsset = Cast<UJetAsset>(FieldSamplerAsset);
	if (ApexFSActor && JetAsset)
	{
		ApexFSActor->setEnabled(bEnabled);

		ApexFSActor->setFieldStrength(FieldStrength);
	}
}

void FApexJetActor::DoUpdatePos()
{
	physx::apex::NxJetFSActor* ApexFSActor = static_cast<physx::apex::NxJetFSActor*>(ApexActor);
	if (ApexFSActor != NULL)
	{
		physx::PxMat44 Pose = U2PMatrix(GlobalPose);
		ApexFSActor->setCurrentPose(Pose);
	}
}

FApexNoiseActor::FApexNoiseActor(UFieldSamplerComponent* InOwner, FPhysScene* InPhysScene, uint32 InSceneType, UFieldSamplerAsset* InFieldSamplerAsset)
	: FApexFieldSamplerActor(InOwner, InPhysScene, InSceneType, InFieldSamplerAsset)
{
}

void FApexNoiseActor::DoUpdateActor()
{
	physx::apex::NxNoiseFSActor* ApexFSActor = static_cast<physx::apex::NxNoiseFSActor*>(ApexActor);
	UNoiseAsset* NoiseAsset = Cast<UNoiseAsset>(FieldSamplerAsset);
	if (ApexFSActor && NoiseAsset)
	{
		ApexFSActor->setEnabled(bEnabled);
		ApexFSActor->setNoiseStrength(NoiseStrength);
	}
}

void FApexNoiseActor::DoUpdatePos()
{
	physx::apex::NxNoiseFSActor* ApexFSActor = static_cast<physx::apex::NxNoiseFSActor*>(ApexActor);
	if (ApexFSActor != NULL)
	{
		physx::PxMat44 Pose = U2PMatrix(GlobalPose);
		ApexFSActor->setCurrentPose(Pose);
	}
}

FApexVortexActor::FApexVortexActor(UFieldSamplerComponent* InOwner, FPhysScene* InPhysScene, uint32 InSceneType, UFieldSamplerAsset* InFieldSamplerAsset)
	: FApexFieldSamplerActor(InOwner, InPhysScene, InSceneType, InFieldSamplerAsset)
{
}

void FApexVortexActor::DoUpdateActor()
{
	physx::apex::NxVortexFSActor* ApexFSActor = static_cast<physx::apex::NxVortexFSActor*>(ApexActor);
	UVortexAsset* VortexAsset = Cast<UVortexAsset>(FieldSamplerAsset);
	if (ApexFSActor && VortexAsset)
	{
		ApexFSActor->setEnabled(bEnabled);

		ApexFSActor->setRotationalStrength(RotationalFieldStrength);
		ApexFSActor->setRadialStrength(RadialFieldStrength);
		ApexFSActor->setLiftStrength(LiftFieldStrength);

		ApexFSActor->setHeight(VortexAsset->CapsuleFieldHeight);
		ApexFSActor->setBottomRadius(VortexAsset->CapsuleFieldBottomRadius);
		ApexFSActor->setTopRadius(VortexAsset->CapsuleFieldTopRadius);
	}
}

void FApexVortexActor::DoUpdatePos()
{
	physx::apex::NxVortexFSActor* ApexFSActor = static_cast<physx::apex::NxVortexFSActor*>(ApexActor);
	if (ApexFSActor != NULL)
	{
		physx::PxMat44 Pose = U2PMatrix(GlobalPose);
		ApexFSActor->setCurrentPose(Pose);
	}
}


FApexHeatSourceActor::FApexHeatSourceActor(UFieldSamplerComponent* InOwner, FPhysScene* InPhysScene, uint32 InSceneType, UFieldSamplerAsset* InFieldSamplerAsset)
	: FApexFieldSamplerActor(InOwner, InPhysScene, InSceneType, InFieldSamplerAsset)
{
}

void FApexHeatSourceActor::DoUpdateActor()
{
	physx::apex::NxHeatSourceActor* ApexFSActor = static_cast<physx::apex::NxHeatSourceActor*>(ApexActor);
	UHeatSourceAsset* HeatSourceAsset = Cast<UHeatSourceAsset>(FieldSamplerAsset);
	if (ApexFSActor && HeatSourceAsset)
	{
		ApexFSActor->setEnabled(bEnabled);
		ApexFSActor->setTemperature(AverageTemperature, StandardTemperature);
	}
}

void FApexHeatSourceActor::DoUpdatePos()
{
	physx::apex::NxHeatSourceActor* ApexFSActor = static_cast<physx::apex::NxHeatSourceActor*>(ApexActor);
	if (ApexFSActor != NULL)
	{
		physx::PxMat44 Pose = U2PMatrix(GlobalPose);
		ApexFSActor->setPose(Pose);
	}
}

FApexVelocitySourceActor::FApexVelocitySourceActor(UFieldSamplerComponent* InOwner, FPhysScene* InPhysScene, uint32 InSceneType, UFieldSamplerAsset* InFieldSamplerAsset)
	: FApexFieldSamplerActor(InOwner, InPhysScene, InSceneType, InFieldSamplerAsset)
{
}

void FApexVelocitySourceActor::DoUpdateActor()
{
	physx::apex::NxVelocitySourceActor* ApexFSActor = static_cast<physx::apex::NxVelocitySourceActor*>(ApexActor);
	UVelocitySourceAsset* VelocitySourceAsset = Cast<UVelocitySourceAsset>(FieldSamplerAsset);
	if (ApexFSActor && VelocitySourceAsset)
	{
		ApexFSActor->setEnabled(bEnabled);
		ApexFSActor->setVelocity(AverageVelocity, StandardVelocity);
	}
}

void FApexVelocitySourceActor::DoUpdatePos()
{
	physx::apex::NxVelocitySourceActor* ApexFSActor = static_cast<physx::apex::NxVelocitySourceActor*>(ApexActor);
	if (ApexFSActor != NULL)
	{
		physx::PxMat44 Pose = U2PMatrix(GlobalPose);
		ApexFSActor->setPose(Pose);
	}
}
#endif // WITH_APEX_TURBULENCE
// NVCHANGE_END: JCAO - 