// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/*=============================================================================
	VelocitySourceActor.cpp: AVelocitySourceActor methods.
=============================================================================*/


#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"


AVelocitySourceActor::AVelocitySourceActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UVelocitySourceComponent>(TEXT("FieldSamplerComponent0")))
{
	VelocitySourceComponent = CastChecked<UVelocitySourceComponent>(FieldSamplerComponent);
}

// NVCHANGE_END: JCAO - Add Turbulence actors and components