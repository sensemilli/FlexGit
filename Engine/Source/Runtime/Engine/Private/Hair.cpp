#include "EnginePrivate.h"
#include "HairSceneProxy.h"
#include "Hair.h"
#include "EditorFramework/AssetImportData.h"

const GFSDK_HairAssetID UHair::AssetIdNull = GFSDK_HairAssetID_NULL;

UHair::UHair(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
}

void UHair::PostInitProperties()
{
#if WITH_EDITORONLY_DATA
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
#endif
	Super::PostInitProperties();
}

UHair::~UHair()
{
	FHairSceneProxy::ReleaseHair_GameThread(AssetId);
}