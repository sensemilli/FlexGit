// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_FLEX

#include "flex.h"
#include "flexExt.h"

// UE types
class UFlexContainer;
class UFlexComponent;
class UFlexAsset;
struct FFlexPhase;
struct IFlexContainerClient;

#if STATS

DECLARE_STATS_GROUP(TEXT("Flex"), STATGROUP_Flex, STATCAT_Advanced);
DECLARE_STATS_GROUP_VERBOSE(TEXT("FlexGpu"), STATGROUP_FlexGpu, STATCAT_Advanced);

enum EFlexStats
{
	// UFlexComponentStats stats
	STAT_Flex_RenderMeshTime,
	STAT_Flex_UpdateBoundsCpu,
	STAT_Flex_ActiveParticleCount,
	STAT_Flex_ActiveMeshActorCount,
	
	// Container stats
	STAT_Flex_DeviceUpdateTime,
	STAT_Flex_SolverUpdateTime,
	STAT_Flex_WaitTime,
	STAT_Flex_GatherCollisionShapes,
	STAT_Flex_UpdateCollisionShapes,
	STAT_Flex_UpdateActors,
	STAT_Flex_ContainerCount,
	STAT_Flex_InstanceCount,
	STAT_Flex_ParticleCount,
	STAT_Flex_SpringCount,
	STAT_Flex_ShapeCount,
	STAT_Flex_StaticConvexCount,
	STAT_Flex_StaticTriangleCount,
	STAT_Flex_ForceFieldCount,
};

#endif

/** Allocation policy for Flex data, allows us to use pinned host memory with TArrays */ 
class FFlexAllocator
{
public:

	enum { NeedsElementType = false };
	enum { RequireRangeCheck = false };

	class ForAnyElementType
	{
	public:
		/** Default constructor. */
		ForAnyElementType()
			: Data(NULL)
		{}

		/**
		 * Moves the state of another allocator into this one.
		 * Assumes that the allocator is currently empty, i.e. memory may be allocated but any existing elements have already been destructed (if necessary).
		 * @param Other - The allocator to move the state from.  This allocator should be left in a valid empty state.
		 */
		void MoveToEmpty(ForAnyElementType& Other);

		/** Destructor. */
		~ForAnyElementType();

		// FContainerAllocatorInterface
		FORCEINLINE FScriptContainerElement* GetAllocation() const
		{
			return Data;
		}
		void ResizeAllocation(int32 PreviousNumElements,int32 NumElements,SIZE_T NumBytesPerElement);

		int32 CalculateSlack(int32 NumElements,int32 NumAllocatedElements,SIZE_T NumBytesPerElement) const
		{
			if(NumElements < NumAllocatedElements)
				return NumAllocatedElements;
			else if(NumElements > 0)
				return NumElements*3/2;
			else
				return 0;

			//return DefaultCalculateSlack(NumElements,NumAllocatedElements,NumBytesPerElement);
		}

		SIZE_T GetAllocatedSize(int32 NumAllocatedElements, SIZE_T NumBytesPerElement) const
		{
			return NumAllocatedElements * NumBytesPerElement;
		}

	private:
		ForAnyElementType(const ForAnyElementType&);
		ForAnyElementType& operator=(const ForAnyElementType&);

		/** A pointer to the container's elements. */
		FScriptContainerElement* Data;
	};
	
	template<typename ElementType>
	class ForElementType : public ForAnyElementType
	{
	public:

		/** Default constructor. */
		ForElementType()
		{}

		FORCEINLINE ElementType* GetAllocation() const
		{
			return (ElementType*)ForAnyElementType::GetAllocation();
		}
	};
};

template <>
struct TAllocatorTraits<FFlexAllocator>
{
	enum { SupportsMove = true };
};

// one container per-phys scene
struct FFlexContainerInstance
{
	FFlexContainerInstance(UFlexContainer* Template, FPhysScene* Owner);
	virtual ~FFlexContainerInstance();

	int32 CreateParticle(const FVector4& pos, const FVector& vel, int32 phase);
	void DestroyParticle(int32 index);

	// spawns a new instance of an asset into the container
	FlexExtInstance* CreateInstance(FlexExtAsset* Asset, const FMatrix& Mat, const FVector& Velocity, int32 Phase);
	void DestroyInstance(FlexExtInstance* Asset);

	// convert a phase to the solver format, will allocate a new group if requested
	int32 GetPhase(const FFlexPhase& Phase);

	// kicks off the simulation update and all compute kernels
	void Simulate(float DeltaTime);	
	// starts synchronization phase, should be called after GPU work has finished e.g.: after a flexWaitFence() from PhysScene
	void Synchronize();

	// gather and send collision data to the solver
	void UpdateCollisionData();
	// send particle and parameter data to the solver
	void UpdateSimData();

	// register component to receive callbacks 
	void Register(IFlexContainerClient* Comp);
	void Unregister(IFlexContainerClient* Comp);

	// add a radial force for one frame 
	void AddRadialForce(FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff);
	// add a radial impulse for one frame 
	void AddRadialImpulse(FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff, bool bVelChange);

	// helper methods
	void ComputeSteppingParam(float& Dt, int32& NumSubsteps, float& NewLeftOverTime, float DeltaTime) const;
	void DebugDraw();

	FlexExtContainer* Container;
	FlexSolver* Solver;
	
	// pointers into the container's memory
	FVector4* Particles;
	FVector* Velocities;
	FVector4* Normals;
	int* Phases;

	// copy of particle data
	TArray<FVector4, FFlexAllocator> Anisotropy1;
	TArray<FVector4, FFlexAllocator> Anisotropy2;
	TArray<FVector4, FFlexAllocator> Anisotropy3;
	TArray<FVector4, FFlexAllocator> SmoothPositions;

	// cached by PhysScene
	int32 NumStaticTriangles;

	FPhysScene* Owner;
	FBoxSphereBounds Bounds;

	TArray<IFlexContainerClient*> Components;
	TWeakObjectPtr<UFlexContainer> Template;

	// incrementing group counter used to auto-assign unique groups to rigids
	int GroupCounter;

	TArray<FVector4, FFlexAllocator> ConvexMins;
	TArray<FVector4, FFlexAllocator> ConvexMaxs;
	TArray<FlexCollisionGeometry, FFlexAllocator> ConvexGeometry;
	TArray<int32, FFlexAllocator> ConvexOffsets;
	TArray<int32, FFlexAllocator> ConvexFlags;
	TArray<FVector4, FFlexAllocator> ConvexPositions;
	TArray<FQuat, FFlexAllocator> ConvexRotations;

	float LeftOverTime;
	static bool sGlobalDebugDraw;

	TArray<FVector, FFlexAllocator> TriMeshVerts;
	TArray<int32, FFlexAllocator> TriMeshIndices;

	TArray<FlexExtForceField, FFlexAllocator> ForceFields;
};

#endif //WITH_FLEX