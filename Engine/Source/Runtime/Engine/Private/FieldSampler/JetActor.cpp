// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/*=============================================================================
	JetActor.cpp: AJetActor methods.
=============================================================================*/


#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"


AJetActor::AJetActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UJetComponent>(TEXT("FieldSamplerComponent0")))
{
	JetComponent = CastChecked<UJetComponent>(FieldSamplerComponent);
}

// NVCHANGE_END: JCAO - Add Turbulence actors and components