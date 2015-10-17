// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset

//=============================================================================
// HeatSourceAssetFactory
//=============================================================================
#pragma once
#include "HeatSourceAssetFactory.generated.h"

UCLASS(hidecategories=Object)
class UHeatSourceAssetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()


	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	// Begin UFactory Interface	
};
// NVCHANGE_END: JCAO - Add Field Sampler Asset


