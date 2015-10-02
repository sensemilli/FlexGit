#pragma once

#include "Hair.generated.h"

enum GFSDK_HairAssetID;

UCLASS()
class ENGINE_API UHair : public UObject
{
	GENERATED_UCLASS_BODY()

	// Begin UObject interface
	virtual void PostInitProperties() override;
	// End UObject interface
	
	UPROPERTY()
	TArray<uint8> AssetData;

	~UHair();

	GFSDK_HairAssetID AssetId = AssetIdNull;
	static const GFSDK_HairAssetID AssetIdNull;

	/** Importing data and options used for this asset */
	UPROPERTY(EditAnywhere, Instanced, Category = ImportSettings)
	class UAssetImportData* AssetImportData;
};
