// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset

//=============================================================================
// AttractorAssetFactory
//=============================================================================
#pragma once
#include "AttractorAssetFactory.generated.h"

UCLASS(hidecategories=Object)
class UAttractorAssetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()


	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	// Begin UFactory Interface	
};

// NVCHANGE_END: JCAO - Add Field Sampler Asset



