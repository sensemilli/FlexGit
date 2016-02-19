// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset

#include "ActorFactoryAttractorActor.generated.h"

UCLASS(MinimalAPI, config=Editor, collapsecategories, hidecategories=Object)
class UActorFactoryAttractorActor : public UActorFactory
{
	GENERATED_UCLASS_BODY()

	// Begin UActorFactory Interface
	virtual void PostSpawnActor( UObject* Asset, AActor* NewActor ) override;
	virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) override;
	// End UActorFactory Interface
};

// NVCHANGE_END: JCAO - Add Field Sampler Asset



