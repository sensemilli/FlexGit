// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset

//=============================================================================
// VelocitySourceAssetFactory
//=============================================================================
#pragma once
#include "VelocitySourceAssetFactory.generated.h"

UCLASS(hidecategories=Object)
class UVelocitySourceAssetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()


	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	// Begin UFactory Interface	
};
// NVCHANGE_END: JCAO - Add Field Sampler Asset


