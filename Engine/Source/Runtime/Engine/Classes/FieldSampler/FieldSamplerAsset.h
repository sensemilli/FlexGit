// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset
#pragma once

#include "FieldSamplerAsset.generated.h"

#if WITH_PHYSX
namespace physx
{
#if WITH_APEX
	namespace apex
	{
		class	NxApexAsset;
	}
#endif
}
#endif
#if WITH_APEX
namespace NxParameterized
{
	class Interface;
}
#endif

class FApexFieldSamplerAsset;

UENUM()
enum EFieldSamplerAssetType
{
	EFSAT_NONE,
	EFSAT_ATTRACTOR,
	EFSAT_GRID,
	EFSAT_JET,
	EFSAT_NOISE,
	EFSAT_VORTEX,
	EFSAT_HEAT_SOURCE,
	EFSAT_VELOCITY_SOURCE,
};

UENUM()
enum EGeometryType
{
	EGeometryType_Sphere UMETA(DisplayName="Sphere"),
	EGeometryType_Box UMETA(DisplayName="Box"),
};

UCLASS(abstract, MinimalAPI)
class UFieldSamplerAsset : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	virtual void PostInitProperties() override;
	virtual void FinishDestroy() override;
	virtual void Serialize( FArchive& Ar ) override;
#if WITH_EDITOR	
	void RegisterFSComponent( UFieldSamplerComponent* FSComponent );
	void UnregisterFSComponent( UFieldSamplerComponent* FSComponent );
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent ) override;
#endif
#if WITH_APEX_TURBULENCE
	NxParameterized::Interface* UFieldSamplerAsset::GetDefaultActorDesc();
	FApexFieldSamplerAsset*		ApexFieldSamplerAsset;
#endif
	virtual EFieldSamplerAssetType GetType() { return EFSAT_NONE; }

protected:
	virtual const char* GetApexAssetObjTypeName() { return 0; }
#if WITH_APEX_TURBULENCE
	virtual void SetupApexAssetParams(NxParameterized::Interface*) {}
#endif


private:
	void CreateApexAsset();
	void DestroyApexAsset();
	void DoRelease();
	TArray< UFieldSamplerComponent* > RegisteredFSComponents;
};
// NVCHANGE_END: JCAO - Add Field Sampler Asset