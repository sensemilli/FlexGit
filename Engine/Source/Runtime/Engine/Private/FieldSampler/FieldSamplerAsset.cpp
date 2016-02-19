// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset

#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"
#include "ApexFieldSamplerAsset.h"

UFieldSamplerAsset::UFieldSamplerAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UFieldSamplerAsset::PostInitProperties()
{
	Super::PostInitProperties();

	CreateApexAsset();
}

void UFieldSamplerAsset::CreateApexAsset()
{
#if WITH_APEX_TURBULENCE
#if WITH_EDITOR
	if (ApexFieldSamplerAsset == NULL)
	{
		const char* AssetObjTypeName = GetApexAssetObjTypeName();
		if (AssetObjTypeName) 
		{
			physx::apex::NxApexAssetAuthoring *Authoring = GApexSDK->createAssetAuthoring(AssetObjTypeName);
			check(Authoring);
			if (Authoring)
			{
				NxParameterized::Interface* ApexAssetParams = Authoring->releaseAndReturnNxParameterizedInterface();
				SetupApexAssetParams(ApexAssetParams);
				ApexFieldSamplerAsset = new FApexFieldSamplerAsset(ApexAssetParams);
				ApexFieldSamplerAsset->IncreaseRefCount();
			}
		}
	}
#endif
#endif
}

void UFieldSamplerAsset::DoRelease()
{
#if WITH_APEX_TURBULENCE
	if(ApexFieldSamplerAsset)
	{
		ApexFieldSamplerAsset->DecreaseRefCount();
		ApexFieldSamplerAsset = NULL;
	}
#endif
}

void UFieldSamplerAsset::DestroyApexAsset()
{
#if WITH_APEX_TURBULENCE
	DoRelease();
#endif
}

void UFieldSamplerAsset::FinishDestroy()
{
	DestroyApexAsset();

	Super::FinishDestroy();
}

#if WITH_APEX_TURBULENCE
NxParameterized::Interface* UFieldSamplerAsset::GetDefaultActorDesc()
{
	NxParameterized::Interface* Params = NULL;

	if (ApexFieldSamplerAsset)
	{
		Params = ApexFieldSamplerAsset->GetApexAsset()->getDefaultActorDesc();
	}

	return Params;
}
#endif // WITH_APEX_TURBULENCE

void UFieldSamplerAsset::Serialize(FArchive& Ar)
{
	Super::Serialize( Ar );

	if( Ar.IsLoading() )
	{
		// Deserializing the name of the NxApexAsset
		TArray<uint8> NameBuffer;
		uint32 NameBufferSize;
		Ar << NameBufferSize;
		NameBuffer.AddUninitialized( NameBufferSize );
		Ar.Serialize( NameBuffer.GetData(), NameBufferSize );

		// Buffer for the NxApexAsset
		TArray<uint8> Buffer;
		uint32 Size;
		Ar << Size;
		if( Size > 0 )
		{
			// Size is non-zero, so a binary blob follows
			Buffer.AddUninitialized( Size );
			Ar.Serialize( Buffer.GetData(), Size );
#if WITH_APEX_TURBULENCE
			// Wrap this blob with the APEX read stream class
			physx::PxFileBuf* Stream = GApexSDK->createMemoryReadStream( Buffer.GetData(), Size );
			// Create an NxParameterized serializer
			NxParameterized::Serializer* Serializer = GApexSDK->createSerializer(NxParameterized::Serializer::NST_BINARY);
			// Deserialize into a DeserializedData buffer
			NxParameterized::Serializer::DeserializedData DeserializedData;
			Serializer->deserialize( *Stream, DeserializedData );
			// remove the apex asset first
			DestroyApexAsset();
			NxApexAsset* Asset = NULL;
			if( DeserializedData.size() > 0 )
			{
				// The DeserializedData has something in it, so create an APEX asset from it
				Asset = GApexSDK->createAsset( DeserializedData[0], NULL );//(const char*)NameBuffer.GetData() );	// BRG - don't require it to have a given name
				// check if the asset is a field sampler asset.
				FString AssetTypeName = Asset->getObjTypeName();
				FString ModuleAssetTypeName = GetApexAssetObjTypeName();
				if (Asset && AssetTypeName != ModuleAssetTypeName)
				{
					Asset->release();
					Asset = NULL;
				}
			}

			DoRelease();
			ApexFieldSamplerAsset = new FApexFieldSamplerAsset(Asset);
			ApexFieldSamplerAsset->IncreaseRefCount();

			// Release our temporary objects
			Serializer->release();
			GApexSDK->releaseMemoryReadStream( *Stream );
#endif // WITH_APEX_TURBULENCE
		}
	}
	else if ( Ar.IsSaving() )
	{
		const char* Name = "NO_APEX";
#if WITH_APEX_TURBULENCE
		Name = NULL;
		// if asset is null, try create once.
		CreateApexAsset();

		if (ApexFieldSamplerAsset != NULL)
		{
			Name = ApexFieldSamplerAsset->GetApexAsset()->getName();
		}
		if( Name == NULL )
		{
			Name = "";
		}
#endif // WITH_APEX_TURBULENCE
		// Serialize the name
		uint32 NameBufferSize = FCStringAnsi::Strlen( Name )+1;
		Ar << NameBufferSize;
		Ar.Serialize( (void*)Name, NameBufferSize );
#if WITH_APEX_TURBULENCE
		// Create an APEX write stream
		physx::PxFileBuf* Stream = GApexSDK->createMemoryWriteStream();
		// Create an NxParameterized serializer
		NxParameterized::Serializer* Serializer = GApexSDK->createSerializer(NxParameterized::Serializer::NST_BINARY);
		uint32 Size = 0;
		TArray<uint8> Buffer;
		// Get the NxParameterized data for asset
		if( ApexFieldSamplerAsset != NULL )
		{
			const NxParameterized::Interface* AssetParameterized = ApexFieldSamplerAsset->GetApexAsset()->getAssetNxParameterized();
			if( AssetParameterized != NULL )
			{
				// Serialize the data into the stream
				Serializer->serialize( *Stream, &AssetParameterized, 1 );
				// Read the stream data into our buffer for UE serialzation
				Size = Stream->getFileLength();
				Buffer.AddUninitialized( Size );
				Stream->read( &Buffer[0], Size );
			}
		}
		Ar << Size;
		if ( Size > 0 )
		{
			Ar.Serialize( Buffer.GetData(), Size );
		}
		// Release our temporary objects
		Serializer->release();
		Stream->release();
#else // WITH_APEX_TURBULENCE
		uint32 size=0;
		Ar << size;
#endif // WITH_APEX_TURBULENCE
	}
}

#if WITH_EDITOR
void UFieldSamplerAsset::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// notify all the components that asset was changed
	TArray<bool> RecreatePhysics;
	RecreatePhysics.AddZeroed(RegisteredFSComponents.Num());

	for( int32 i=0; i<RegisteredFSComponents.Num(); i++ )
	{
		// check if physics state was created
		if (RegisteredFSComponents[i]->IsPhysicsStateCreated())
		{
			RegisteredFSComponents[i]->DestroyPhysicsState();
			RecreatePhysics[i] = true;
		}
	}

	DestroyApexAsset();
	CreateApexAsset();

	// Recreate the apex actors

	for( int32 i=0; i<RegisteredFSComponents.Num(); i++ )
	{
		// if the component has the physics state before, then recreate the physics state.
		if (RecreatePhysics[i])
		{
			RegisteredFSComponents[i]->CreatePhysicsState();
		}
	}
}

void UFieldSamplerAsset::RegisterFSComponent( UFieldSamplerComponent* FSComponent )
{
	if(!RegisteredFSComponents.Contains(FSComponent))
	{
		RegisteredFSComponents.Add(FSComponent);
	}
}

void UFieldSamplerAsset::UnregisterFSComponent( UFieldSamplerComponent* FSComponent )
{
	RegisteredFSComponents.Remove(FSComponent);
}
#endif // WITH_EDITOR

// NVCHANGE_END: JCAO - Add Field Sampler Asset