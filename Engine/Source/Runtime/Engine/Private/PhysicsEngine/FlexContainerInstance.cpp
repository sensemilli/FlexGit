// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "PhysXSupport.h"
#include "PhysicsPublic.h"
#include "FlexContainerInstance.h"

#if WITH_FLEX

bool FFlexContainerInstance::sGlobalDebugDraw = false;

#if STATS

enum EFlexGpuStats
{
	// gpu stats
	STAT_Flex_ContainerGpuTickTime,
	STAT_Flex_Predict,
	STAT_Flex_CreateCellIndices,
	STAT_Flex_SortCellIndices,		
	STAT_Flex_CreateGrid,				
	STAT_Flex_Reorder,					
	STAT_Flex_CollideParticles,
	STAT_Flex_CollideConvexes,			
	STAT_Flex_CollideTriangles,		
	STAT_Flex_CollideFields,			
	STAT_Flex_CalculateDensity,		
	STAT_Flex_SolveDensities,			
	STAT_Flex_SolveVelocities,			
	STAT_Flex_SolveShapes,				
	STAT_Flex_SolveSprings,			
	STAT_Flex_SolveContacts,			
	STAT_Flex_SolveInflatables,		
	STAT_Flex_CalculateAnisotropy,		
	STAT_Flex_UpdateDiffuse,			
	STAT_Flex_UpdateTriangles,
	STAT_Flex_Finalize,				
	STAT_Flex_UpdateBounds,		
};

#endif

// CPU stats, use "stat flex" to enable
DECLARE_CYCLE_STAT(TEXT("Gather Collision Shapes Time (CPU)"), STAT_Flex_GatherCollisionShapes, STATGROUP_Flex);
DECLARE_CYCLE_STAT(TEXT("Update Collision Shapes Time (CPU)"), STAT_Flex_UpdateCollisionShapes, STATGROUP_Flex);
DECLARE_CYCLE_STAT(TEXT("Update Actors Time (CPU)"), STAT_Flex_UpdateActors, STATGROUP_Flex);
DECLARE_CYCLE_STAT(TEXT("Update Data Time (CPU)"), STAT_Flex_DeviceUpdateTime, STATGROUP_Flex);
DECLARE_CYCLE_STAT(TEXT("Solver Tick Time (CPU)"), STAT_Flex_SolverUpdateTime, STATGROUP_Flex);

// Counters
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Container Count"), STAT_Flex_ContainerCount, STATGROUP_Flex);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Instance Count"), STAT_Flex_InstanceCount, STATGROUP_Flex);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Particle Count"), STAT_Flex_ParticleCount, STATGROUP_Flex);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Spring Count"), STAT_Flex_SpringCount, STATGROUP_Flex);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Shape Count"), STAT_Flex_ShapeCount, STATGROUP_Flex);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Static Convex Count"), STAT_Flex_StaticConvexCount, STATGROUP_Flex);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Static Triangle Count"), STAT_Flex_StaticTriangleCount, STATGROUP_Flex);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Force Field Count"), STAT_Flex_ForceFieldCount, STATGROUP_Flex);

// GPU stats, use "stat group enable flexgpu", and "stat flexgpu" to enable via console
// note that the the GPU counters will introduce significant synchronization overhead
DECLARE_CYCLE_STAT(TEXT("Predict"), STAT_Flex_Predict, STATGROUP_FlexGpu);
DECLARE_CYCLE_STAT(TEXT("CreateCellIndices"), STAT_Flex_CreateCellIndices, STATGROUP_FlexGpu);
DECLARE_CYCLE_STAT(TEXT("SortCellIndices"), STAT_Flex_SortCellIndices, STATGROUP_FlexGpu);		
DECLARE_CYCLE_STAT(TEXT("CreateGrid"), STAT_Flex_CreateGrid, STATGROUP_FlexGpu);				
DECLARE_CYCLE_STAT(TEXT("Reorder"), STAT_Flex_Reorder, STATGROUP_FlexGpu);					
DECLARE_CYCLE_STAT(TEXT("Collide Particles"), STAT_Flex_CollideParticles, STATGROUP_FlexGpu);
DECLARE_CYCLE_STAT(TEXT("Collide Convexes"), STAT_Flex_CollideConvexes, STATGROUP_FlexGpu);			
DECLARE_CYCLE_STAT(TEXT("Collide Triangles"), STAT_Flex_CollideTriangles, STATGROUP_FlexGpu);		
DECLARE_CYCLE_STAT(TEXT("Collide Fields"), STAT_Flex_CollideFields, STATGROUP_FlexGpu);			
DECLARE_CYCLE_STAT(TEXT("Calculate Density"), STAT_Flex_CalculateDensity, STATGROUP_FlexGpu);		
DECLARE_CYCLE_STAT(TEXT("Solve Density"), STAT_Flex_SolveDensities, STATGROUP_FlexGpu);			
DECLARE_CYCLE_STAT(TEXT("Solve Velocities"), STAT_Flex_SolveVelocities, STATGROUP_FlexGpu);			
DECLARE_CYCLE_STAT(TEXT("Solve Shapes"), STAT_Flex_SolveShapes, STATGROUP_FlexGpu);				
DECLARE_CYCLE_STAT(TEXT("Solve Springs"), STAT_Flex_SolveSprings, STATGROUP_FlexGpu);			
DECLARE_CYCLE_STAT(TEXT("Solve Contacts"), STAT_Flex_SolveContacts, STATGROUP_FlexGpu);			
DECLARE_CYCLE_STAT(TEXT("Solve Inflatables"), STAT_Flex_SolveInflatables, STATGROUP_FlexGpu);		
DECLARE_CYCLE_STAT(TEXT("Calculate Anisotropy"), STAT_Flex_CalculateAnisotropy, STATGROUP_FlexGpu);		
DECLARE_CYCLE_STAT(TEXT("Update Diffuse"), STAT_Flex_UpdateDiffuse, STATGROUP_FlexGpu);			
DECLARE_CYCLE_STAT(TEXT("Finalize"), STAT_Flex_Finalize, STATGROUP_FlexGpu);				
DECLARE_CYCLE_STAT(TEXT("Update Bounds"), STAT_Flex_UpdateBounds, STATGROUP_FlexGpu);
DECLARE_CYCLE_STAT(TEXT("Update Triangles"), STAT_Flex_UpdateTriangles, STATGROUP_FlexGpu);
DECLARE_CYCLE_STAT(TEXT("Total GPU Kernel Time"), STAT_Flex_ContainerGpuTickTime, STATGROUP_FlexGpu);

void FFlexAllocator::ForAnyElementType::MoveToEmpty(ForAnyElementType& Other)
{
	check(this != &Other);

	if (Data)
	{
		flexFree(Data);
	}

	Data       = Other.Data;
	Other.Data = NULL;
}

/** Destructor. */
FFlexAllocator::ForAnyElementType::~ForAnyElementType()
{
	if(Data)
	{
		flexFree(Data);
	}
}

void FFlexAllocator::ForAnyElementType::ResizeAllocation(int32 PreviousNumElements,int32 NumElements,SIZE_T NumBytesPerElement)
{
	// Avoid calling FMemory::Realloc( NULL, 0 ) as ANSI C mandates returning a valid pointer which is not what we want.
	if( Data || NumElements )
	{
		//checkSlow(((uint64)NumElements*(uint64)ElementTypeInfo.GetSize() < (uint64)INT_MAX));
		void* NewData = flexAlloc(NumElements*NumBytesPerElement);
		FMemory::Memcpy(NewData, Data, PreviousNumElements*NumBytesPerElement);
		flexFree(Data);
		Data = (FScriptContainerElement*)NewData;				
	}
}

namespace 
{
	// helpers to find actor, shape pairs in a TSet
	bool operator == (const PxActorShape& lhs, const PxActorShape& rhs) { return lhs.actor == rhs.actor && lhs.shape == rhs.shape; }
	uint32 GetTypeHash(const PxActorShape& h) { return ::GetTypeHash((void*)(h.actor)) ^ ::GetTypeHash((void*)(h.shape)); }
}

// fwd decl
PxFilterData CreateQueryFilterData(const uint8 MyChannel, const bool bTraceComplex, const FCollisionResponseContainer & InCollisionResponseContainer, const struct FCollisionObjectQueryParams & ObjectParam, const bool bMultitrace);

// send bodies from synchronous PhysX scene to Flex scene
void FFlexContainerInstance::UpdateCollisionData()
{	
	// skip empty containers
	int32 NumActive = flexGetActiveCount(Solver);
	if (NumActive == 0)
		return;

	DEC_DWORD_STAT_BY(STAT_Flex_StaticConvexCount, ConvexPositions.Num());
	DEC_DWORD_STAT_BY(STAT_Flex_StaticTriangleCount, TriMeshIndices.Num()/3);

	ConvexMins.SetNum(0);
	ConvexMaxs.SetNum(0);
	ConvexGeometry.SetNum(0);
	ConvexOffsets.SetNum(0);
	ConvexPositions.SetNum(0);
	ConvexRotations.SetNum(0);
	ConvexFlags.SetNum(0);
	TriMeshVerts.SetNum(0);
	TriMeshIndices.SetNum(0);

	FBox TriMeshBounds;
	TriMeshBounds.Init();

	// used to test if an actor shape pair has already been reported
	TSet<PxActorShape> OverlapSet;
	
	// buffer for overlaps
	TArray<FOverlapResult> Overlaps;
	TArray<PxShape*> Shapes;

	// lock the scene to perform scene queries
	SCENE_LOCK_READ(Owner->GetPhysXScene(PST_Sync));

	// gather shapes from the scene
	for (int32 ActorIndex=0; ActorIndex < Components.Num(); ActorIndex++)
	{
		SCOPE_CYCLE_COUNTER(STAT_Flex_GatherCollisionShapes);

		IFlexContainerClient* Component = Components[ActorIndex];
		if (!Component->IsEnabled())
			continue;

		FBoxSphereBounds ComponentBounds = Component->GetBounds();

		// expand bounds to catch any potential collisions (assume 60fps)
		const FVector Expand = FVector(Template->MaxVelocity / 60.0f + Template->CollisionDistance + Template->CollisionMarginShapes);
		const FVector Center = ComponentBounds.Origin;
		const FVector HalfEdge = ComponentBounds.BoxExtent + Expand;

		// if particles explode, the bound will be very big and cause a hang in the overlap code below or crash
		if(HalfEdge.SizeSquared2D() > Template->MaxContainerBound)
		{
			UE_LOG(LogFlex, Warning, TEXT("Flex container bound grows bigger than %f") , Template->MaxContainerBound);
			continue;
		}

		FCollisionShape Shape;
		Shape.SetBox(HalfEdge);
	
		Overlaps.Reset();
		Owner->OwningWorld->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity, Template->ObjectType, Shape, FCollisionQueryParams(false), FCollisionResponseParams(Template->ResponseToChannels));
		
		for (int32 i=0; i < Overlaps.Num(); ++i)
		{
			const FOverlapResult& hit = Overlaps[i];
			
			const UPrimitiveComponent* PrimComp = hit.Component.Get();
			if (!PrimComp)
				continue;

			//OverlapMultiple returns ECollisionResponse::ECR_Overlap types, which we want to ignore
			ECollisionResponse Response = PrimComp->GetCollisionResponseToChannel(Template->ObjectType);
			if (Response != ECollisionResponse::ECR_Block)
				continue;
			
			FBodyInstance* Body = PrimComp->GetBodyInstance();
			if (!Body)
				continue;

			PxRigidActor* Actor = Body->GetPxRigidActor_AssumesLocked();
			if (!Actor)
				continue;

			Shapes.SetNum(0);

			int32 NumSyncShapes;			
			NumSyncShapes = Body->GetAllShapes_AssumesLocked(Shapes);

			for (int ShapeIndex = 0; ShapeIndex < Shapes.Num(); ++ShapeIndex)
			{
				PxShape* Shape = Shapes[ShapeIndex];

				if (!Actor || !Shape)
					continue;

				// check if we've already processed this actor-shape pair
				bool alreadyProcessed = false;
				OverlapSet.Add(PxActorShape(Actor, Shape), &alreadyProcessed);
				if (alreadyProcessed)
					continue;
		
				const PxTransform& ActorTransform = Actor->getGlobalPose();
				const PxTransform& ShapeTransform = Shape->getLocalPose();
		
				PxFilterData Filter = Shape->getQueryFilterData();

				// only process complex collision shapes if enabled on the container
				if (Template->ComplexCollision)
				{
					if ((Filter.word3 & EPDF_ComplexCollision) == 0)
						continue;
				}
				else
				{
					if ((Filter.word3 & EPDF_SimpleCollision) == 0)
						continue;
				}
		
				switch(Shape->getGeometryType())
				{
					case PxGeometryType::eSPHERE:
					case PxGeometryType::eCAPSULE:
					case PxGeometryType::eBOX:
					{							
						PxTransform WorldTransform = ActorTransform*ShapeTransform;

						ConvexOffsets.Push(ConvexGeometry.Num());

						ConvexPositions.Push(FVector4(WorldTransform.p.x, WorldTransform.p.y, WorldTransform.p.z, 1.0f));
						ConvexRotations.Push(FQuat(WorldTransform.q.x, WorldTransform.q.y, WorldTransform.q.z, WorldTransform.q.w));

						if (Shape->getGeometryType() == PxGeometryType::eCAPSULE)
						{
							PxCapsuleGeometry CapsuleGeometry;
							Shape->getCapsuleGeometry(CapsuleGeometry);

							FlexCollisionCapsule Capsule;
							Capsule.mHalfHeight = CapsuleGeometry.halfHeight;
							Capsule.mRadius = CapsuleGeometry.radius;

							PxBounds3 Bounds = PxBounds3::poseExtent(WorldTransform, PxVec3(Capsule.mHalfHeight + Capsule.mRadius, Capsule.mRadius, Capsule.mRadius));
							ConvexMins.Push(FVector4(Bounds.minimum.x, Bounds.minimum.y, Bounds.minimum.z, 0.0f));
							ConvexMaxs.Push(FVector4(Bounds.maximum.x, Bounds.maximum.y, Bounds.maximum.z, 0.0f));

							ConvexGeometry.Push((FlexCollisionGeometry&)Capsule);

							int32 Flags = flexMakeShapeFlags(FlexCollisionShapeType::eFlexShapeCapsule, Actor->isRigidStatic() == NULL);
							ConvexFlags.Push(Flags);

						}
						else if (Shape->getGeometryType() == PxGeometryType::eSPHERE)
						{
							PxSphereGeometry SphereGeometry;
							Shape->getSphereGeometry(SphereGeometry);

							FlexCollisionSphere Sphere;
							Sphere.mRadius = SphereGeometry.radius;

							PxBounds3 Bounds = PxBounds3::poseExtent(WorldTransform, PxVec3(Sphere.mRadius));
							ConvexMins.Push(FVector4(Bounds.minimum.x, Bounds.minimum.y, Bounds.minimum.z, 0.0f));
							ConvexMaxs.Push(FVector4(Bounds.maximum.x, Bounds.maximum.y, Bounds.maximum.z, 0.0f));

							ConvexGeometry.Push((FlexCollisionGeometry&)Sphere);

							int32 Flags = flexMakeShapeFlags(FlexCollisionShapeType::eFlexShapeSphere, Actor->isRigidStatic() == NULL);
							ConvexFlags.Push(Flags);

						}
						else if (Shape->getGeometryType() == PxGeometryType::eBOX)
						{
							PxBoxGeometry BoxGeometry;
							Shape->getBoxGeometry(BoxGeometry);							

							PxBounds3 Bounds = PxBounds3::poseExtent(WorldTransform, BoxGeometry.halfExtents);
							ConvexMins.Push(FVector4(Bounds.minimum.x, Bounds.minimum.y, Bounds.minimum.z, 0.0f));
							ConvexMaxs.Push(FVector4(Bounds.maximum.x, Bounds.maximum.y, Bounds.maximum.z, 0.0f));

							FlexCollisionPlane planes[6] = 
							{
								{ 1.0f, 0.0f, 0.0f, -BoxGeometry.halfExtents.x },
								{ 0.0f, 1.0f, 0.0f, -BoxGeometry.halfExtents.y },
								{ 0.0f, 0.0f, 1.0f, -BoxGeometry.halfExtents.z },
								{-1.0f, 0.0f, 0.0f, -BoxGeometry.halfExtents.x },
								{ 0.0f, -1.0f, 0.0f, -BoxGeometry.halfExtents.y },
								{ 0.0f, 0.0f, -1.0f, -BoxGeometry.halfExtents.z }
							};

							for (int i=0; i < 6; ++i)
								ConvexGeometry.Push((FlexCollisionGeometry&)planes[i]);

							int32 Flags = flexMakeShapeFlags(FlexCollisionShapeType::eFlexShapeConvexMesh, Actor->isRigidStatic() == NULL);
							ConvexFlags.Push(Flags);
						}

						break;
					}
					case PxGeometryType::eCONVEXMESH:
					{
						PxConvexMeshGeometry ConvexMesh;
						Shape->getConvexMeshGeometry(ConvexMesh);

						PxMat44 MeshTransform = PxMat44(ConvexMesh.scale.toMat33(), PxVec3(0.0f));
						FMatrix WorldTransform = P2UMatrix(PxMat44(ActorTransform*ShapeTransform)*MeshTransform);
								
						if (ConvexMesh.convexMesh)
						{

							int32 NumPolygons = ConvexMesh.convexMesh->getNbPolygons();

							ConvexOffsets.Push(ConvexGeometry.Num());
									
							// to handle non-uniform scale we transform planes to world space, so these transforms can be identity
							ConvexPositions.Push(FVector4(0.0f, 0.0f, 0.0f, 1.0f)); //LOC_MOD
							ConvexRotations.Push(FQuat::Identity);
																											
							// todo: currently using actor bounds, for compounds it would be more efficient to use each shape's bounds instead
							PxBounds3 Bounds = Actor->getWorldBounds();

							ConvexMins.Push(FVector4(Bounds.minimum.x, Bounds.minimum.y, Bounds.minimum.z, 0.0f));
							ConvexMaxs.Push(FVector4(Bounds.maximum.x, Bounds.maximum.y, Bounds.maximum.z, 0.0f));

							for (int32 p=0; p < NumPolygons; ++p)
							{
								PxHullPolygon Poly;
								ConvexMesh.convexMesh->getPolygonData(p, Poly);

								// transform plane to world space
								FPlane WorldPlane = FPlane(Poly.mPlane[0], Poly.mPlane[1], Poly.mPlane[2], -Poly.mPlane[3]).TransformBy(WorldTransform); 
								WorldPlane.W *= -1.0f;

								ConvexGeometry.Push((FlexCollisionGeometry&)WorldPlane);
							}
														
							int32 Flags = flexMakeShapeFlags(FlexCollisionShapeType::eFlexShapeConvexMesh, Actor->isRigidStatic() == NULL);
							ConvexFlags.Push(Flags);
						}

						break;
					}
					case PxGeometryType::eTRIANGLEMESH:
					{
						PxTriangleMeshGeometry TriMesh;							
						Shape->getTriangleMeshGeometry(TriMesh);

						PxMat44 MeshTransform = PxMat44(TriMesh.scale.toMat33(), PxVec3(0.0f));
						FMatrix WorldTransform = P2UMatrix(PxMat44(ActorTransform*ShapeTransform)*MeshTransform);

						int32 IndexOffset = TriMeshVerts.Num();

						if (TriMesh.triangleMesh)
						{
							int32 NumVerts = TriMesh.triangleMesh->getNbVertices();
							int32 NumIndices = TriMesh.triangleMesh->getNbTriangles()*3;

							const PxVec3* Verts = TriMesh.triangleMesh->getVertices();

							for (int32 v=0; v < NumVerts; ++v)
								TriMeshVerts.Push(WorldTransform.TransformPosition(P2UVector(Verts[v])));

							if (TriMesh.triangleMesh->getTriangleMeshFlags() & PxTriangleMeshFlag::eHAS_16BIT_TRIANGLE_INDICES)
							{
								const uint16* Indices = (const uint16*)TriMesh.triangleMesh->getTriangles();

								for (int32 t=0; t < NumIndices; ++t)
									TriMeshIndices.Push(Indices[t] + IndexOffset);
							}
							else
							{
								const uint32* Indices = (const uint32*)TriMesh.triangleMesh->getTriangles();

								for (int32 t=0; t < NumIndices; ++t)
									TriMeshIndices.Push(Indices[t] + IndexOffset);
							}										
						}

						PxBounds3 ActorBounds = Actor->getWorldBounds();
						TriMeshBounds += FBox(P2UVector(ActorBounds.minimum), P2UVector(ActorBounds.maximum));						

						break;
					}
					case PxGeometryType::eHEIGHTFIELD:
					{
						PxHeightFieldGeometry HeightFieldGeom;
						Shape->getHeightFieldGeometry(HeightFieldGeom);

						FMatrix WorldTransform = P2UMatrix(PxMat44(ActorTransform*ShapeTransform));
						FVector Scale = FVector(HeightFieldGeom.rowScale, HeightFieldGeom.heightScale, HeightFieldGeom.columnScale);

						PxHeightField*	HeightField = HeightFieldGeom.heightField;
						const PxU32		NumCols = HeightField->getNbColumns();
						const PxU32		NumRows = HeightField->getNbRows();
						const PxU32		NumVerts = NumRows * NumCols;
						const PxU32     NumFaces = (NumCols - 1) * (NumRows - 1) * 2;

						PxHeightFieldSample* sampleBuffer = new PxHeightFieldSample[NumVerts];
						HeightField->saveCells(sampleBuffer, NumVerts * sizeof(PxHeightFieldSample));

						int32 IndexOffset = TriMeshVerts.Num(); // offset to the eTRIANGLEMESH above for this heightfield.

						for (PxU32 i = 0; i < NumRows; i++)
						{
							for (PxU32 j = 0; j < NumCols; j++)
							{
								FVector vert = FVector(float(i), float(sampleBuffer[j + (i*NumCols)].height), float(j))*Scale;

								TriMeshVerts.Push(WorldTransform.TransformPosition(vert)); //LOC_MOD revisit scale twice here?

								//LOC_MOD remove
								//DrawDebugPoint(Owner->OwningWorld, TriMeshVerts.Last(), 17.0f, FLinearColor::White);
							}
						}

						for (PxU16 i = 0; i < (NumCols - 1); ++i)
						{
							for (PxU16 j = 0; j < (NumRows - 1); ++j)
							{
								PxU8 tessFlag = sampleBuffer[i + j*NumCols].tessFlag();
								PxU16 i0 = j*NumCols + i;
								PxU16 i1 = j*NumCols + i + 1;
								PxU16 i2 = (j + 1) * NumCols + i;
								PxU16 i3 = (j + 1) * NumCols + i + 1;
								// i2---i3
								// |    |
								// |    |
								// i0---i1
								// this is really a corner vertex index, not triangle index
								PxU16 mat0 = HeightField->getTriangleMaterialIndex((j*NumCols + i) * 2);
								PxU16 mat1 = HeightField->getTriangleMaterialIndex((j*NumCols + i) * 2 + 1);
								bool hole0 = (mat0 == PxHeightFieldMaterial::eHOLE);
								bool hole1 = (mat1 == PxHeightFieldMaterial::eHOLE);
							
								TriMeshIndices.Push(IndexOffset + (hole0 ? i0 : i2));
								TriMeshIndices.Push(IndexOffset + i0);
								TriMeshIndices.Push(IndexOffset + (tessFlag ? i3 : i1));

								TriMeshIndices.Push(IndexOffset + (hole1 ? i1 : i3));
								TriMeshIndices.Push(IndexOffset + (tessFlag ? i0 : i2));
								TriMeshIndices.Push(IndexOffset + i1);
							
							}
						}

						PxBounds3 ActorBounds = Actor->getWorldBounds();
						TriMeshBounds += FBox(P2UVector(ActorBounds.minimum), P2UVector(ActorBounds.maximum));

						delete[] sampleBuffer;

						break;
					}
				};
			}
		}
	}

	SCENE_UNLOCK_READ(Owner->GetPhysXScene(PST_Sync));


	float AvgTriEdgeLength = 0.0f;

	for (int i=0; i < TriMeshIndices.Num(); i+=3)
	{
		int a = TriMeshIndices[i+0];
		int b = TriMeshIndices[i+1];
		int c = TriMeshIndices[i+2];

		FVector va = TriMeshVerts[a];
		FVector vb = TriMeshVerts[b];
		FVector vc = TriMeshVerts[c];

		AvgTriEdgeLength += (vb-va).Size();
		AvgTriEdgeLength += (vc-va).Size();
		AvgTriEdgeLength += (vc-vb).Size();
	}

	AvgTriEdgeLength /= TriMeshIndices.Num();

	// push to flex
	{
		SCOPE_CYCLE_COUNTER(STAT_Flex_UpdateCollisionShapes);

		if (ConvexMins.Num())
		{
			flexSetShapes(Solver, &ConvexGeometry[0], ConvexGeometry.Num(), (float*)&ConvexMins[0], (float*)&ConvexMaxs[0], &ConvexOffsets[0], (float*)&ConvexPositions[0], (float*)&ConvexRotations[0], NULL, NULL, &ConvexFlags[0], ConvexFlags.Num(), eFlexMemoryHostAsync);
		}
		else
		{
			flexSetShapes(Solver, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, eFlexMemoryHostAsync);
		}

		// only update static triangles if count changes (expensive operation)
		if (TriMeshIndices.Num())
		{
			flexSetTriangles(Solver, &TriMeshIndices[0], (float*)&TriMeshVerts[0], TriMeshIndices.Num() / 3, TriMeshVerts.Num(), AvgTriEdgeLength, (float*)&TriMeshBounds.Min, (float*)&TriMeshBounds.Max, eFlexMemoryHostAsync);
		}
		else
		{
			flexSetTriangles(Solver, NULL, NULL, 0, 0, 0.0f, NULL, NULL, eFlexMemoryHostAsync);
		}
	}

	INC_DWORD_STAT_BY(STAT_Flex_StaticConvexCount, ConvexPositions.Num());
	INC_DWORD_STAT_BY(STAT_Flex_StaticTriangleCount, TriMeshIndices.Num()/3);
}

FFlexContainerInstance::FFlexContainerInstance(UFlexContainer* InTemplate, FPhysScene* OwnerScene)
	: Bounds(FVector(0.0f), FVector(0.0f), 0.0f)
{
	INC_DWORD_STAT(STAT_Flex_ContainerCount);

	UE_LOG(LogFlex, Display, TEXT("Creating a FLEX system for..."));

	Template = InTemplate;

	Solver = flexCreateSolver(Template->MaxParticles, Template->MaxDiffuseParticles);
	Container = flexExtCreateContainer(Solver, Template->MaxParticles);

	// get pointers to container memory (these do not change)
	flexExtGetParticleData(Container, (float**)&Particles, (float**)&Velocities, (int32**)&Phases, (float**)&Normals);

	if (Template->AnisotropyScale > 0.0f)
	{
		Anisotropy1.SetNum(Template->MaxParticles);
		Anisotropy2.SetNum(Template->MaxParticles);
		Anisotropy3.SetNum(Template->MaxParticles);
	}

	if (Template->PositionSmoothing > 0.0f)
	{
		SmoothPositions.SetNum(Template->MaxParticles);
	}

	if (Template->MaxDiffuseParticles > 0.0f)
	{
		DiffuseParticles.SetNum(Template->MaxParticles);
		DiffuseVelocities.SetNum(Template->MaxParticles);
		DiffuseIndices.SetNum(Template->MaxParticles);
	}

	NumStaticTriangles = 0;
	NumDiffuseParticles = 0;

	GroupCounter = 0;
	LeftOverTime = 0.0f;

	Owner = OwnerScene;

	bWarmup = true;
}

FFlexContainerInstance::~FFlexContainerInstance()
{
	DEC_DWORD_STAT(STAT_Flex_ContainerCount);
	DEC_DWORD_STAT_BY(STAT_Flex_StaticConvexCount, ConvexPositions.Num());
	DEC_DWORD_STAT_BY(STAT_Flex_StaticTriangleCount, TriMeshIndices.Num()/3);

	UE_LOG(LogFlex, Display, TEXT("Destroying a FLEX system for.."));

	if (Container)
		flexExtDestroyContainer(Container);

	if (Solver)
		flexDestroySolver(Solver);	
}

int32 FFlexContainerInstance::CreateParticle(const FVector4& Pos, const FVector& Vel, int32 Phase)
{
	verify(Container);

	int32 index;
	int32 n = flexExtAllocParticles(Container, 1, &index);

	if (n == 0)
	{
		// not enough space in container to allocate
		return -1;
	}
	else
	{
		INC_DWORD_STAT(STAT_Flex_ParticleCount);

		Particles[index] = Pos;
		Velocities[index] = Vel;
		Phases[index] = Phase;

		return index;
	}
}

void FFlexContainerInstance::DestroyParticle(int32 Index)
{
	verify(Container);
	verify(Index >=0 && Index < Template->MaxParticles);

	flexExtFreeParticles(Container, 1, &Index);

	DEC_DWORD_STAT(STAT_Flex_ParticleCount);
}

/** */
void FFlexContainerInstance::CreateParticles(const TArray<FVector>& InPositions, const TArray<float>& InMasses, const TArray<FVector>& InVelocities, int32 InPhase, TArray<int32>& OutIndices)
{
	verify(Container);

	OutIndices.AddZeroed(InPositions.Num());
	int32 n = flexExtAllocParticles(Container, InPositions.Num(), OutIndices.GetData());
	if (n == 0)
	{
		OutIndices.Empty();
	}
	else
	{
		for (int32 i = 0; i < OutIndices.Num(); i++)
		{
			INC_DWORD_STAT(STAT_Flex_ParticleCount);

			Particles[OutIndices[i]] = FVector4(InPositions[i], InMasses[i]);
			Velocities[OutIndices[i]] = InVelocities[i];
			Phases[OutIndices[i]] = InPhase;
		}
	}
}
 
FlexExtInstance* FFlexContainerInstance::CreateInstance(FlexExtAsset* Asset, const FMatrix& Mat, const FVector& Velocity, int32 Phase)
{
	// spawn into the container
	FlexExtInstance* Inst = flexExtCreateInstance(Container, Asset, (const float*)&Mat, Velocity.X, Velocity.Y, Velocity.Z, Phase, 1.0f);

	// creation will fail if instance cannot fit inside container
	if (Inst)
	{
		INC_DWORD_STAT(STAT_Flex_InstanceCount);
		INC_DWORD_STAT_BY(STAT_Flex_ParticleCount, Inst->mAsset->mNumParticles);
		INC_DWORD_STAT_BY(STAT_Flex_SpringCount, Inst->mAsset->mNumSprings);
		INC_DWORD_STAT_BY(STAT_Flex_ShapeCount, Inst->mAsset->mNumShapes);
	}
	else
	{
		// disabled warning text to stop spamming the log
		//debugf(TEXT("Could not create Flex object, not enough space remaining in container."));
	}

	return Inst;
}

void FFlexContainerInstance::DestroyInstance(FlexExtInstance* Inst)
{
	DEC_DWORD_STAT(STAT_Flex_InstanceCount);
	DEC_DWORD_STAT_BY(STAT_Flex_ParticleCount, Inst->mAsset->mNumParticles);
	DEC_DWORD_STAT_BY(STAT_Flex_SpringCount, Inst->mAsset->mNumSprings);
	DEC_DWORD_STAT_BY(STAT_Flex_ShapeCount, Inst->mAsset->mNumShapes);

	flexExtDestroyInstance(Container, Inst);
}

int32 FFlexContainerInstance::GetPhase(const FFlexPhase& Phase)
{
	int Group = Phase.Group;

	// if required then auto-assign a new group
	if (Phase.AutoAssignGroup)
		Group = GroupCounter++;

	int Flags = 0;
	if (Phase.SelfCollide)
		Flags |= eFlexPhaseSelfCollide;

	if (Phase.IgnoreRestCollisions)
		Flags |= eFlexPhaseSelfCollideFilter;

	if (Phase.Fluid)
		Flags |= eFlexPhaseFluid;

	return flexMakePhase(Group, Flags);
}


void FFlexContainerInstance::ComputeSteppingParam(float& Dt, int32& NumSubsteps, float& NewLeftOverTime, float DeltaTime) const
{
	// clamp DeltaTime to a minimum to avoid taking an 
	// excessive number of substeps during frame-rate spikes
	DeltaTime = FMath::Min(DeltaTime, 1.0f/float(Template->MinFrameRate));

	// convert substeps parameter to substeps per-second
	// a value of 2 corresponds to 120 substeps/second
	const float StepsPerSecond = Template->NumSubsteps*60.0f;
	const float SubstepDt = 1.0f/StepsPerSecond;
	const float ElapsedTime = LeftOverTime + DeltaTime;

	if (Template->FixedTimeStep)
	{
		NumSubsteps = int32(ElapsedTime/SubstepDt);		
		Dt = NumSubsteps*SubstepDt;

		// don't carry over more than 1 substep worth of time
		NewLeftOverTime = FMath::Min(ElapsedTime - NumSubsteps*SubstepDt, SubstepDt);
	}
	else
	{
		NumSubsteps = Template->NumSubsteps;
		Dt = DeltaTime;
		
		NewLeftOverTime = 0.0f;
	}
}

void FFlexContainerInstance::UpdateSimData()
{
	SCOPE_CYCLE_COUNTER(STAT_Flex_DeviceUpdateTime);

	//map the surface tension to a comfortable scale
	static float SurfaceTensionFactor = 1e-6f;

	FlexParams params;
	FMemory::Memset(&params, 0, sizeof(params));

	params.mGravity[0] = Template->Gravity.X;
	params.mGravity[1] = Template->Gravity.Y;
	params.mGravity[2] = Template->Gravity.Z;

	params.mWind[0] = Template->Wind.X;
	params.mWind[1] = Template->Wind.Y;
	params.mWind[2] = Template->Wind.Z;

	params.mRadius = Template->Radius;
	params.mViscosity = Template->Viscosity;
	params.mDynamicFriction = Template->ShapeFriction;
	params.mStaticFriction = Template->ShapeFriction;
	params.mParticleFriction = Template->ParticleFriction;
	params.mDrag = Template->Drag;	
	params.mLift = Template->Lift;
	params.mDamping = Template->Damping;
	params.mNumIterations = Template->NumIterations;
	params.mSolidRestDistance = Template->Radius;// (Template->Fluid) ? (Template->Radius * Template->RestDistance) : Template->Radius;
	params.mFluidRestDistance = Template->Radius*Template->RestDistance;
	params.mDissipation = Template->Dissipation;
	params.mParticleCollisionMargin = Template->CollisionMarginParticles;
	params.mShapeCollisionMargin = (Template->CollisionMarginShapes > 0.0f) ? Template->CollisionMarginShapes : Template->CollisionDistance * 0.25f;// FMath::Max(Template->CollisionMarginShapes, FMath::Max(Template->CollisionDistance*0.25f, 1.0f)); // ensure a minimum collision distance for generating contacts against shapes, we need some margin to avoid jittering as contacts activate/deactivate
	params.mCollisionDistance = Template->CollisionDistance;
	params.mPlasticThreshold = Template->PlasticThreshold;
	params.mPlasticCreep = Template->PlasticCreep;
	params.mFluid = Template->Fluid;
	params.mSleepThreshold = Template->SleepThreshold;
	params.mShockPropagation = Template->ShockPropagation;
	params.mRestitution = Template->Restitution;
	params.mSmoothing = Template->PositionSmoothing;
	params.mMaxSpeed = Template->MaxVelocity;
	params.mRelaxationMode = Template->RelaxationMode == EFlexSolverRelaxationMode::Local ? eFlexRelaxationLocal : eFlexRelaxationGlobal;
	params.mRelaxationFactor = Template->RelaxationFactor;
	params.mSolidPressure = Template->SolidPressure;
	params.mAnisotropyScale = Template->AnisotropyScale;
	params.mAnisotropyMin = Template->AnisotropyMin;
	params.mAnisotropyMax = Template->AnisotropyMax;
	params.mAdhesion = Template->Adhesion;
	params.mCohesion = Template->Cohesion;
	params.mSurfaceTension = Template->SurfaceTension * SurfaceTensionFactor;
	params.mVorticityConfinement = Template->VorticityConfinement;
	params.mDiffuseThreshold = 0.0f;
	params.mBuoyancy = 1.0f;
	params.mEnableCCD = Template->EnableCCD;

	params.mDiffuseThreshold = Template->DiffuseThreshold;
	params.mDiffuseBallistic = Template->DiffuseBallistic;
	params.mDiffuseBuoyancy = Template->DiffuseBuoyancy;
	params.mDiffuseDrag = Template->DiffuseDrag;

	params.mPlanes[0][0] = 0.0f;
	params.mPlanes[0][1] = 0.0f;
	params.mPlanes[0][2] = 1.0f;
	params.mPlanes[0][3] = 0.0f;
	params.mNumPlanes = 0;

	int32 ParticleCount = flexGetActiveCount(Solver);
	flexSetVelocities(Solver, (float*)&Velocities[0].X, ParticleCount, eFlexMemoryHost);

	// update params
	flexSetParams(Solver, &params);

	// force fields
	flexExtSetForceFields(Container, ForceFields.GetData(), ForceFields.Num(), eFlexMemoryHostAsync);

	// move particle data to GPU, async
	flexExtPushToDevice(Container);
}

void FFlexContainerInstance::Simulate(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_Flex_SolverUpdateTime);

	// only catpure perf counters if stats are visible (significant perf. cost)
	FlexTimers Timers;
	FMemory::Memset(&Timers, 0, sizeof(Timers));
	FlexTimers* TimersPtr = NULL;

#if STATS
	// only gather GPU stats if enabled as this has a high perf. overhead
	static TStatId GatherEnable = IStatGroupEnableManager::Get().GetHighPerformanceEnableForStat(FName(), STAT_GROUP_TO_FStatGroup(STATGROUP_FlexGpu)::GetGroupName(), STAT_GROUP_TO_FStatGroup(STATGROUP_UObjects)::GetGroupCategory(), STAT_GROUP_TO_FStatGroup(STATGROUP_FlexGpu)::DefaultEnable, true, EStatDataType::ST_int64, TEXT("Flex GPU Stats"), true);

	if (!GatherEnable.IsNone())
		TimersPtr = &Timers;
#endif

	float Dt;
	int32 NumSubsteps;
	ComputeSteppingParam(Dt, NumSubsteps, LeftOverTime, DeltaTime);

	// updates collision shapes in flex
	UpdateCollisionData();
	
	// updates particle data on the device
	UpdateSimData();

	// tick container, note this happens asynchronously with respect
	// to the calling thread, Synchronize() will be called when it has completed
	flexUpdateSolver(Solver, Dt, NumSubsteps, TimersPtr);

	// read back data asynchronously
	flexExtPullFromDevice(Container);

	if (Template->AnisotropyScale > 0.0f)
	{
		flexGetAnisotropy(Solver, (float*)&Anisotropy1[0], (float*)&Anisotropy2[0], (float*)&Anisotropy3[0], FlexMemory::eFlexMemoryHostAsync);
	}

	if (Template->PositionSmoothing > 0.0f)
	{
		flexGetSmoothParticles(Solver, (float*)&SmoothPositions[0], Template->MaxParticles, FlexMemory::eFlexMemoryHostAsync);
	}

	if (Template->MaxDiffuseParticles > 0.0f)
	{
		NumDiffuseParticles = flexGetDiffuseParticles(Solver, (float*)&DiffuseParticles[0], (float*)&DiffuseVelocities[0], (int32*)DiffuseIndices[0], eFlexMemoryDeviceAsync);
	}

#if STATS
	if (TimersPtr)
	{
		float scale = 0.001f / FPlatformTime::GetSecondsPerCycle();

		SET_CYCLE_COUNTER(STAT_Flex_Predict, FMath::TruncToInt(Timers.mPredict*scale));
		SET_CYCLE_COUNTER(STAT_Flex_CreateCellIndices, FMath::TruncToInt(Timers.mCreateCellIndices * scale));
		SET_CYCLE_COUNTER(STAT_Flex_SortCellIndices, FMath::TruncToInt(Timers.mSortCellIndices* scale));
		SET_CYCLE_COUNTER(STAT_Flex_CreateGrid, FMath::TruncToInt(Timers.mCreateGrid * scale));
		SET_CYCLE_COUNTER(STAT_Flex_Reorder, FMath::TruncToInt(Timers.mReorder * scale));
		SET_CYCLE_COUNTER(STAT_Flex_CollideParticles, FMath::TruncToInt(Timers.mCollideParticles* scale));
		SET_CYCLE_COUNTER(STAT_Flex_CollideConvexes, FMath::TruncToInt(Timers.mCollideShapes * scale));
		SET_CYCLE_COUNTER(STAT_Flex_CollideTriangles, FMath::TruncToInt(Timers.mCollideTriangles * scale));
		SET_CYCLE_COUNTER(STAT_Flex_CollideFields, FMath::TruncToInt(Timers.mCollideFields * scale));
		SET_CYCLE_COUNTER(STAT_Flex_CalculateDensity, FMath::TruncToInt(Timers.mCalculateDensity* scale));
		SET_CYCLE_COUNTER(STAT_Flex_SolveDensities, FMath::TruncToInt(Timers.mSolveDensities * scale));
		SET_CYCLE_COUNTER(STAT_Flex_SolveVelocities, FMath::TruncToInt(Timers.mSolveVelocities* scale));
		SET_CYCLE_COUNTER(STAT_Flex_SolveShapes, FMath::TruncToInt(Timers.mSolveShapes * scale));
		SET_CYCLE_COUNTER(STAT_Flex_SolveSprings, FMath::TruncToInt(Timers.mSolveSprings * scale));
		SET_CYCLE_COUNTER(STAT_Flex_SolveContacts, FMath::TruncToInt(Timers.mSolveContacts * scale));
		SET_CYCLE_COUNTER(STAT_Flex_SolveInflatables, FMath::TruncToInt(Timers.mSolveInflatables * scale));
		SET_CYCLE_COUNTER(STAT_Flex_CalculateAnisotropy, FMath::TruncToInt(Timers.mCalculateAnisotropy * scale));
		SET_CYCLE_COUNTER(STAT_Flex_UpdateDiffuse, FMath::TruncToInt(Timers.mUpdateDiffuse * scale));
		SET_CYCLE_COUNTER(STAT_Flex_Finalize, FMath::TruncToInt(Timers.mFinalize * scale));
		SET_CYCLE_COUNTER(STAT_Flex_UpdateBounds, FMath::TruncToInt(Timers.mUpdateBounds * scale));
	}
#endif

	SET_DWORD_STAT(STAT_Flex_ForceFieldCount, ForceFields.Num());

	//reset force fields
	ForceFields.SetNum(0);
}

void FFlexContainerInstance::Synchronize()
{
	// output any debug information
	DebugDraw();
	
	// get container bounds
	FVector Lower, Upper;
	flexExtGetBoundsData(Container, (float*)&Lower, (float*)&Upper);
	Bounds = FBoxSphereBounds(FBox(Lower, Upper));

	flexExtUpdateInstances(Container);

	{
		SCOPE_CYCLE_COUNTER(STAT_Flex_UpdateActors);
		
		// process components
		for (int32 i=0; i < Components.Num(); ++i)
			Components[i]->Synchronize();
	}
}

void FFlexContainerInstance::Register(IFlexContainerClient* Comp)
{
	Components.Push(Comp);
}

void FFlexContainerInstance::Unregister(IFlexContainerClient* Comp)
{
	Components.RemoveAt(Components.Find(Comp));
}


void FFlexContainerInstance::DebugDraw()
{
	if (Template->DebugDraw || sGlobalDebugDraw)
	{
		// draw instance bounds
		for (int32 i=0; i < Components.Num(); ++i)
		{
			IFlexContainerClient* Component = Components[i];
			if (!Component->IsEnabled())
				continue;

			FBoxSphereBounds ComponentBounds = Component->GetBounds();

			DrawDebugBox(Owner->OwningWorld, ComponentBounds.Origin, ComponentBounds.BoxExtent, FColor(0, 255, 0));
		}

		//draw container bounds
		DrawDebugBox(Owner->OwningWorld, Bounds.Origin, Bounds.BoxExtent, FColor(255, 255, 255));

		// draw particles
		const FColor Colors[8] = 
		{
			FLinearColor(0.0f, 0.5f, 1.0f).ToFColor(false),
			FLinearColor(0.797f, 0.354f, 0.000f).ToFColor(false),
			FLinearColor(0.092f, 0.465f, 0.820f).ToFColor(false),
			FLinearColor(0.000f, 0.349f, 0.173f).ToFColor(false),
			FLinearColor(0.875f, 0.782f, 0.051f).ToFColor(false),
			FLinearColor(0.000f, 0.170f, 0.453f).ToFColor(false),
			FLinearColor(0.673f, 0.111f, 0.000f).ToFColor(false),
			FLinearColor(0.612f, 0.194f, 0.394f).ToFColor(false)
		};

		TArray<int32> ActiveIndices;
		ActiveIndices.SetNum(Template->MaxParticles);

		int32 NumActive = flexExtGetActiveList(Container, &ActiveIndices[0]);
			
		// draw particles colored by phase
		for (int32 i = 0; i < NumActive; ++i)
			DrawDebugPoint(Owner->OwningWorld, Particles[ActiveIndices[i]], 10.0f, Colors[Phases[ActiveIndices[i]]%8]);

		// visualize contacts against the environment
		const int maxContactsPerParticle = 4;

		TArray<FPlane> ContactPlanes;
		ContactPlanes.SetNum(Template->MaxParticles*maxContactsPerParticle);
			
		TArray<int32> ContactIndices;
		ContactIndices.SetNum(Template->MaxParticles);

		TArray<uint8> ContactCounts;
		ContactCounts.SetNum(Template->MaxParticles);
		
		flexGetContacts(Solver, (float*)&ContactPlanes[0], &ContactIndices[0], &ContactCounts[0], eFlexMemoryHost);

		for (int i=0 ; i < NumActive; ++i)
		{
			const int ContactIndex = ContactIndices[ActiveIndices[i]];
			const unsigned char Count = ContactCounts[ContactIndex];

			const float Scale = 10.0f;

			for (int c=0; c < Count; ++c)
			{
				FPlane Plane = ContactPlanes[ContactIndex*maxContactsPerParticle + c];

				DrawDebugLine(Owner->OwningWorld, Particles[ActiveIndices[i]],Particles[ActiveIndices[i]] + FVector(Plane.X, Plane.Y, Plane.Z)*Scale, FColor::Green);
			}
		}
	}
}

void FFlexContainerInstance::AddRadialForce(FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff)
{
	ForceFields.AddUninitialized(1);
	FlexExtForceField& Force = ForceFields.Top();

	(FVector&)Force.mPosition = Origin;
	Force.mRadius = Radius;
	Force.mStrength = Strength;
	Force.mLinearFalloff = (Falloff != RIF_Constant);
	Force.mMode = eFlexExtModeForce;
}

void FFlexContainerInstance::AddRadialImpulse(FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff, bool bVelChange)
{
	ForceFields.AddUninitialized(1);
	FlexExtForceField& Force = ForceFields.Top();

	(FVector&)Force.mPosition = Origin;
	Force.mRadius = Radius;
	Force.mStrength = Strength;
	Force.mLinearFalloff = (Falloff != RIF_Constant);
	Force.mMode = bVelChange ? eFlexExtModeVelocityChange : eFlexExtModeImpulse;
}

#endif //WITH_FLEX

