// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset

#include "ActorFactoryHeatSourceActor.generated.h"

UCLASS(MinimalAPI, config=Editor, collapsecategories, hidecategories=Object)
class UActorFactoryHeatSourceActor : public UActorFactory
{
	GENERATED_UCLASS_BODY()

	// Begin UActorFactory Interface
	virtual void PostSpawnActor( UObject* Asset, AActor* NewActor ) override;
	virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) override;
	// End UActorFactory Interface
};
// NVCHANGE_END: JCAO - Add Field Sampler Asset


