#include "AssetToolsPrivatePCH.h"
#include "Hair.h"
#include "AssetTypeActions_Hair.h"

UClass* FAssetTypeActions_Hair::GetSupportedClass() const
{
	return UHair::StaticClass();
}

void FAssetTypeActions_Hair::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const
{
	for (auto& Asset : TypeAssets)
	{
		const auto Hair = CastChecked<UHair>(Asset);
		Hair->AssetImportData->ExtractFilenames(OutSourceFilePaths);
	}
}
