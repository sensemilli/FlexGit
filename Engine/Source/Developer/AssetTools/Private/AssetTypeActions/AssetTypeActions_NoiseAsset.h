// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset
#pragma once

class FAssetTypeActions_NoiseAsset : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_NoiseAsset", "Noise Asset"); }
	virtual FColor GetTypeColor() const override { return FColor(255,192,128); }
	virtual UClass* GetSupportedClass() const override{ return UNoiseAsset::StaticClass(); }
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Physics; }
};

// NVCHANGE_END: JCAO - Add Field Sampler Asset