// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset

//=============================================================================
// VortexAssetFactory
//=============================================================================
#pragma once
#include "VortexAssetFactory.generated.h"

UCLASS(hidecategories=Object)
class UVortexAssetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()


	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	// Begin UFactory Interface	
};
// NVCHANGE_END: JCAO - Add Field Sampler Asset


