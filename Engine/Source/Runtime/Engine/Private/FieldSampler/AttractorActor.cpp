// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/*=============================================================================
	AttractorActor.cpp: AAttractorActor methods.
=============================================================================*/


#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"


AAttractorActor::AAttractorActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UAttractorComponent>(TEXT("FieldSamplerComponent0")))
{
	AttractorComponent = CastChecked<UAttractorComponent>(FieldSamplerComponent);
}

// NVCHANGE_END: JCAO - Add Turbulence actors and components