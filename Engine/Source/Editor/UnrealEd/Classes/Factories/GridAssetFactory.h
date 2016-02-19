// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset

//=============================================================================
// GridAssetFactory
//=============================================================================
#pragma once
#include "GridAssetFactory.generated.h"

UCLASS(hidecategories=Object)
class UGridAssetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()


	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	// Begin UFactory Interface	
};

// NVCHANGE_END: JCAO - Add Field Sampler Asset



