

#include "EnginePrivate.h"

ARenderToTextureFlowMap::ARenderToTextureFlowMap(const class FObjectInitializer& PCIP)
	: Super(PCIP), BlendTimeScale(0.1f), SurfaceWidth(512), SurfaceHeight(512), ParameterBaseName(TEXT("Texture"))
{
	PrimaryActorTick.bCanEverTick = true;
}

void ARenderToTextureFlowMap::BeginPlay()
{
	TargetInstance = UMaterialInstanceDynamic::Create(TargetMaterial, this);
	ResetInstance = UMaterialInstanceDynamic::Create(ResetMaterial, this);

	Super::BeginPlay();

	RenderTargets.Empty(6);
	for (int Index = 0; Index < 6; ++Index)
	{
		BindResetInstance(Index);
		RenderTargets.Add(UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(this, UCanvasRenderTarget2D::StaticClass(), SurfaceWidth, SurfaceHeight));
		RenderTargets[Index]->AddressX = RenderTargets[Index]->AddressY = TA_Clamp;
		RenderTargets[Index]->OverrideFormat = PF_G32R32F;
		RenderTargets[Index]->OnCanvasRenderTargetUpdate.AddDynamic(this, &ARenderToTextureFlowMap::OnReceiveUpdate);
		RenderTargets[Index]->UpdateResource(); // initialize render target
	}

	SourceInstances.Empty(3);
	for (int Index = 0; Index < 3; ++Index)
	{
		SourceInstances.Add(UMaterialInstanceDynamic::Create(SourceMaterial, this));
	}

	BlendTime = 0.0f;
	FlipFlop = 0;
}

void ARenderToTextureFlowMap::Tick(float DeltaTime)
{
	float NewBlendTime = BlendTime + DeltaTime * BlendTimeScale;

	for (int Index = FlipFlop; Index < 6; Index += 2)
	{
		float IndexTime = (Index / 2 + 1) / 3.0f;
		if (BlendTime <= IndexTime && IndexTime < NewBlendTime)
		{
			BindResetInstance(Index);
		}
		else
		{
			SourceInstances[Index / 2]->SetTextureParameterValue(TEXT("Texture"), RenderTargets[Index ^ 1]);
			SourceInstances[Index / 2]->SetScalarParameterValue(TEXT("DeltaTime"), DeltaTime);
			RenderSource = SourceInstances[Index / 2];
		}
		RenderTargets[Index]->UpdateResource();

		FString ParameterName = FString::Printf(*(ParameterBaseName + TEXT("%u")), Index / 2);
		TargetInstance->SetTextureParameterValue(FName(*ParameterName), RenderTargets[Index]);
	}

	BlendTime = modff(NewBlendTime, &NewBlendTime);
	TargetInstance->SetScalarParameterValue(TEXT("BlendTime"), BlendTime);

	FlipFlop = !FlipFlop;
}

void ARenderToTextureFlowMap::BindResetInstance(int Index)
{
	FLinearColor Offset = FLinearColor(ForceInitToZero);
	Offset.Component(Index / 2) = 0.5f;
	ResetInstance->SetVectorParameterValue(TEXT("Offset"), Offset);
	RenderSource = ResetInstance;
}

void ARenderToTextureFlowMap::BindSourceInstance(int Index)
{
}

void ARenderToTextureFlowMap::OnReceiveUpdate(UCanvas* Canvas, int32 Width, int32 Height)
{
	RenderMaterial(RenderSource, Canvas, Width, Height);
}

void ARenderToTextureFlowMap::RenderMaterial(UMaterialInterface* Material, UCanvas* Canvas, int32 Width, int32 Height)
{
	if (Material && Canvas)
	{
		FCanvasTileItem TileItem(FVector2D(0, 0), Material->GetRenderProxy(false), FVector2D(Width, Height));
		Canvas->DrawItem(TileItem);
	}
}
