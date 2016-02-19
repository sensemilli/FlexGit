// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/*=============================================================================
	GridActor.cpp: AGridActor methods.
=============================================================================*/


#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"


AGridActor::AGridActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UGridComponent>(TEXT("FieldSamplerComponent0")))
{
	GridComponent = CastChecked<UGridComponent>(FieldSamplerComponent);
}

// NVCHANGE_END: JCAO - Add Turbulence actors and components
