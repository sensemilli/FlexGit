// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/*=============================================================================
	VortexActor.cpp: AVortexActor methods.
=============================================================================*/


#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"


AVortexActor::AVortexActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UVortexComponent>(TEXT("FieldSamplerComponent0")))
{
	VortexComponent = CastChecked<UVortexComponent>(FieldSamplerComponent);
}

// NVCHANGE_END: JCAO - Add Turbulence actors and components