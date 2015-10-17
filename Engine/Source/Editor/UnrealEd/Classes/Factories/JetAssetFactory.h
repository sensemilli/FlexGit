// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset

//=============================================================================
// JetAssetFactory
//=============================================================================
#pragma once
#include "JetAssetFactory.generated.h"

UCLASS(hidecategories=Object)
class UJetAssetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()


	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	// Begin UFactory Interface	
};

// NVCHANGE_END: JCAO - Add Field Sampler Asset



