// NVCHANGE_BEGIN: Add VXGI

#pragma once
#include "MaterialExpressionVxgiVoxelization.generated.h"

UCLASS(collapsecategories, hidecategories=Object)
class UMaterialExpressionVxgiVoxelization : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual bool NeedsRealtimePreview() override { return true; }
	// End UMaterialExpression Interface
#if WITH_EDITOR
	virtual uint32 GetOutputType(int32 OutputIndex) { return MCT_Float; }
#endif
};

// NVCHANGE_END: Add VXGI
