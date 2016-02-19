// NVCHANGE_BEGIN: JCAO - Turbulence FS

/*==============================================================================
	TurbulenceFS.cpp: TurbulenceFS instance
==============================================================================*/

#include "EnginePrivate.h"
#include "TurbulenceFS.h"

void FTurbulenceFSInstance::UpdateTransforms(const FMatrix& LocalToWorld)
{
	const FVector VolumeOffset = LocalBounds.Min;
	const FVector VolumeScale = LocalBounds.Max - LocalBounds.Min;
	VolumeToWorldNoScale = LocalToWorld.GetMatrixWithoutScale().RemoveTranslation();
	VolumeToWorld = FScaleMatrix(VolumeScale) * FTranslationMatrix(VolumeOffset)
		* FTranslationMatrix(LocalToWorld.GetOrigin());
	WorldToVolume = VolumeToWorld.Inverse();

#if VERIFY_UV
	FVector Pos = LocalToWorld.GetOrigin();
	FVector4 CenterUV = WorldToVolume.TransformFVector4(FVector4(Pos, 1.0f));
	FVector4 UV_Bound1 = WorldToVolume.TransformFVector4(FVector4(Pos - VolumeScale * 0.5f, 1.0f));
	FVector4 UV_Bound2 = WorldToVolume.TransformFVector4(FVector4(Pos + VolumeScale * 0.5f, 1.0f));
	FVector4 UV_X1 = WorldToVolume.TransformFVector4(FVector4(Pos.X - VolumeScale.X * 0.5f, Pos.Y, Pos.Z, 1.0f));
	FVector4 UV_X2 = WorldToVolume.TransformFVector4(FVector4(Pos.X + VolumeScale.X * 0.5f, Pos.Y, Pos.Z, 1.0f));
	FVector4 UV_Y1 = WorldToVolume.TransformFVector4(FVector4(Pos.X , Pos.Y - VolumeScale.Y * 0.5f, Pos.Z, 1.0f));
	FVector4 UV_Y2 = WorldToVolume.TransformFVector4(FVector4(Pos.X , Pos.Y + VolumeScale.Y * 0.5f, Pos.Z, 1.0f));
	FVector4 UV_Z1 = WorldToVolume.TransformFVector4(FVector4(Pos.X, Pos.Y, Pos.Z - VolumeScale.Z * 0.5f, 1.0f));
	FVector4 UV_Z2 = WorldToVolume.TransformFVector4(FVector4(Pos.X, Pos.Y, Pos.Z + VolumeScale.Z * 0.5f, 1.0f));

	FVector4 RandomPos = FVector4( 
		Pos.X - VolumeScale.X * 0.5f + FMath::FRand() * VolumeScale.X, 
		Pos.Y - VolumeScale.Y * 0.5f + FMath::FRand() * VolumeScale.Y,
		Pos.Z - VolumeScale.Z * 0.5f + FMath::FRand() * VolumeScale.Z,
		1.0f);
	FVector4 UV_Rand = WorldToVolume.TransformFVector4(RandomPos);
#endif
}

void FAttractorFSInstance::UpdateTransforms(const FMatrix& LocalToWorld)
{
	Origin = LocalToWorld.GetOrigin();
}
// NVCHANGE_END: JCAO - Turbulence FS