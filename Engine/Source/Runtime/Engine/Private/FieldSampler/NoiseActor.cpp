// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/*=============================================================================
	NoiseActor.cpp: ANoiseActor methods.
=============================================================================*/


#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"


ANoiseActor::ANoiseActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UNoiseComponent>(TEXT("FieldSamplerComponent0")))
{
	NoiseComponent = CastChecked<UNoiseComponent>(FieldSamplerComponent);
}

// NVCHANGE_END: JCAO - Add Turbulence actors and components