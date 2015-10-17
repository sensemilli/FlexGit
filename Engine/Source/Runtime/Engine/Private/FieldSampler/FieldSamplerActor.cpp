// NVCHANGE_BEGIN: JCAO - Add Turbulence actors and components

/*=============================================================================
	FieldSamplerActor.cpp: AFieldSamplerActor methods.
=============================================================================*/


#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"


AFieldSamplerActor::AFieldSamplerActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FieldSamplerComponent = ObjectInitializer.CreateAbstractDefaultSubobject<UFieldSamplerComponent>(this, TEXT("FieldSamplerComponent0"));
	RootComponent = FieldSamplerComponent;

#if WITH_EDITORONLY_DATA
	SpriteComponent = ObjectInitializer.CreateEditorOnlyDefaultSubobject<UBillboardComponent>(this, TEXT("Sprite"));

	if (!IsRunningCommandlet() && (SpriteComponent != nullptr))
	{
		// Structure to hold one-time initialization
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> EffectsTextureObject;
			FName ID_Effects;
			FText NAME_Effects;
			FConstructorStatics()
				: EffectsTextureObject(TEXT("/Engine/EditorResources/S_VectorFieldVol"))
				, ID_Effects(TEXT("Effects"))
				, NAME_Effects(NSLOCTEXT("SpriteCategory", "Effects", "Effects"))
			{
			}
		};
		static FConstructorStatics ConstructorStatics;

		SpriteComponent->Sprite = ConstructorStatics.EffectsTextureObject.Get();
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_Effects;
		SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Effects;
		SpriteComponent->bAbsoluteScale = true;
		SpriteComponent->AttachParent = FieldSamplerComponent;
		SpriteComponent->bReceivesDecals = false;
	}
#endif // WITH_EDITORONLY_DATA
}

// NVCHANGE_END: JCAO - Add Turbulence actors and components
