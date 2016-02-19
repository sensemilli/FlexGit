// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/*=============================================================================
	HeatSourceActor.cpp: AHeatSourceActor methods.
=============================================================================*/


#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"


AHeatSourceActor::AHeatSourceActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UHeatSourceComponent>(TEXT("FieldSamplerComponent0")))
{
	HeatSourceComponent = CastChecked<UHeatSourceComponent>(FieldSamplerComponent);
}

// NVCHANGE_END: JCAO - Add Turbulence actors and components