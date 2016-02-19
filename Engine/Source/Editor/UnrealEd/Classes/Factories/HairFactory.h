#pragma once

#include "HairFactory.generated.h"

UCLASS()
class UHairFactory : public UFactory, public FReimportHandler
{
	GENERATED_UCLASS_BODY()

	bool FactoryCanImport(const FString& Filename)override;
	FText GetDisplayName() const override;
	UObject* FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn) override;
	bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	EReimportResult::Type Reimport(UObject* Obj) override;
};


