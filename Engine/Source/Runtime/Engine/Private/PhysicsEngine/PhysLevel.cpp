// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "PhysicsPublic.h"
#include "ParticleDefinitions.h"
#include "PrecomputedLightVolume.h"

#if WITH_PHYSX
	#include "PhysXSupport.h"
// NVCHANGE_BEGIN: JCAO - Replace vector fields with APEX turbulence velocity fields
	#include "PhysXRender.h"
// NVCHANGE_END: JCAO - Replace vector fields with APEX turbulence velocity fields
#endif

#if WITH_BOX2D
	#include "../PhysicsEngine2D/Box2DIntegration.h"
#endif
#include "PhysicsEngine/PhysicsSettings.h"

#ifndef APEX_STATICALLY_LINKED
	#define APEX_STATICALLY_LINKED	0
#endif

#if WITH_FLEX
// fwd declare error func
void FlexErrorFunc(FlexErrorSeverity level, const char* msg, const char* file, int line);
#endif

/** Physics stats */

DEFINE_STAT(STAT_TotalPhysicsTime);
DEFINE_STAT(STAT_PhysicsKickOffDynamicsTime);
DEFINE_STAT(STAT_PhysicsFetchDynamicsTime);
DEFINE_STAT(STAT_PhysicsEventTime);
DEFINE_STAT(STAT_SetBodyTransform);

DEFINE_STAT(STAT_NumBroadphaseAdds);
DEFINE_STAT(STAT_NumBroadphaseRemoves);
DEFINE_STAT(STAT_NumActiveConstraints);
DEFINE_STAT(STAT_NumActiveSimulatedBodies);
DEFINE_STAT(STAT_NumActiveKinematicBodies);
DEFINE_STAT(STAT_NumMobileBodies);
DEFINE_STAT(STAT_NumStaticBodies);
DEFINE_STAT(STAT_NumShapes);

DEFINE_STAT(STAT_NumBroadphaseAddsAsync);
DEFINE_STAT(STAT_NumBroadphaseRemovesAsync);
DEFINE_STAT(STAT_NumActiveConstraintsAsync);
DEFINE_STAT(STAT_NumActiveSimulatedBodiesAsync);
DEFINE_STAT(STAT_NumActiveKinematicBodiesAsync);
DEFINE_STAT(STAT_NumMobileBodiesAsync);
DEFINE_STAT(STAT_NumStaticBodiesAsync);
DEFINE_STAT(STAT_NumShapesAsync);


FPhysCommandHandler * GPhysCommandHandler = NULL;

// CVars
static TAutoConsoleVariable<float> CVarToleranceScaleLength(
	TEXT("p.ToleranceScale_Length"),
	100.f,
	TEXT("The approximate size of objects in the simulation. Default: 100"),
	ECVF_ReadOnly);

static TAutoConsoleVariable<float> CVarTolerenceScaleMass(
	TEXT("p.ToleranceScale_Mass"),
	100.f,
	TEXT("The approximate mass of a length * length * length block. Default: 100"),
	ECVF_ReadOnly);

static TAutoConsoleVariable<float> CVarToleranceScaleSpeed(
	TEXT("p.ToleranceScale_Speed"),
	1000.f,
	TEXT("The typical magnitude of velocities of objects in simulation. Default: 1000"),
	ECVF_ReadOnly);

static TAutoConsoleVariable<int32> CVarAPEXMaxDestructibleDynamicChunkIslandCount(
	TEXT("p.APEXMaxDestructibleDynamicChunkIslandCount"),
	2000,
	TEXT("APEX Max Destructilbe Dynamic Chunk Island Count."),
	ECVF_Default);


static TAutoConsoleVariable<int32> CVarAPEXMaxDestructibleDynamicChunkCount(
	TEXT("p.APEXMaxDestructibleDynamicChunkCount"),
	2000,
	TEXT("APEX Max Destructible dynamic Chunk Count."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarAPEXSortDynamicChunksByBenefit(
	TEXT("p.bAPEXSortDynamicChunksByBenefit"),
	1,
	TEXT("True if APEX should sort dynamic chunks by benefit."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarUseUnifiedHeightfield(
	TEXT("p.bUseUnifiedHeightfield"),
	1,
	TEXT("Whether to use the PhysX unified heightfield. This feature of PhysX makes landscape collision consistent with triangle meshes but the thickness parameter is not supported for unified heightfields. 1 enables and 0 disables. Default: 1"),
	ECVF_ReadOnly);



//////////////////////////////////////////////////////////////////////////
// UWORLD
//////////////////////////////////////////////////////////////////////////

void UWorld::SetupPhysicsTickFunctions(float DeltaSeconds)
{
	StartPhysicsTickFunction.bCanEverTick = true;
	StartPhysicsTickFunction.Target = this;
	
	EndPhysicsTickFunction.bCanEverTick = true;
	EndPhysicsTickFunction.Target = this;
	
	StartClothTickFunction.bCanEverTick = true;
	StartClothTickFunction.Target = this;
	
	EndClothTickFunction.bCanEverTick = true;
	EndClothTickFunction.Target = this;
	
	
	// see if we need to update tick registration
	bool bNeedToUpdateTickRegistration = (bShouldSimulatePhysics != StartPhysicsTickFunction.IsTickFunctionRegistered())
		|| (bShouldSimulatePhysics != EndPhysicsTickFunction.IsTickFunctionRegistered())
		|| (bShouldSimulatePhysics != StartClothTickFunction.IsTickFunctionRegistered())
		|| (bShouldSimulatePhysics != EndClothTickFunction.IsTickFunctionRegistered());

	if (bNeedToUpdateTickRegistration && PersistentLevel)
	{
		if (bShouldSimulatePhysics && !StartPhysicsTickFunction.IsTickFunctionRegistered())
		{
			StartPhysicsTickFunction.TickGroup = TG_StartPhysics;
			StartPhysicsTickFunction.RegisterTickFunction(PersistentLevel);
		}
		else if (!bShouldSimulatePhysics && StartPhysicsTickFunction.IsTickFunctionRegistered())
		{
			StartPhysicsTickFunction.UnRegisterTickFunction();
		}

		if (bShouldSimulatePhysics && !EndPhysicsTickFunction.IsTickFunctionRegistered())
		{
			EndPhysicsTickFunction.TickGroup = TG_EndPhysics;
			EndPhysicsTickFunction.RegisterTickFunction(PersistentLevel);
			EndPhysicsTickFunction.AddPrerequisite(this, StartPhysicsTickFunction);
		}
		else if (!bShouldSimulatePhysics && EndPhysicsTickFunction.IsTickFunctionRegistered())
		{
			EndPhysicsTickFunction.RemovePrerequisite(this, StartPhysicsTickFunction);
			EndPhysicsTickFunction.UnRegisterTickFunction();
		}

		//cloth
		if (bShouldSimulatePhysics && !StartClothTickFunction.IsTickFunctionRegistered())
		{
			StartClothTickFunction.TickGroup = TG_StartCloth;
			StartClothTickFunction.RegisterTickFunction(PersistentLevel);
		}
		else if (!bShouldSimulatePhysics && StartClothTickFunction.IsTickFunctionRegistered())
		{
			StartClothTickFunction.UnRegisterTickFunction();
		}

		if (bShouldSimulatePhysics && !EndClothTickFunction.IsTickFunctionRegistered())
		{
			EndClothTickFunction.TickGroup = TG_EndCloth;
			EndClothTickFunction.RegisterTickFunction(PersistentLevel);
			EndClothTickFunction.AddPrerequisite(this, StartClothTickFunction);
		}
		else if (!bShouldSimulatePhysics && EndClothTickFunction.IsTickFunctionRegistered())
		{
			EndClothTickFunction.RemovePrerequisite(this, StartClothTickFunction);
			EndClothTickFunction.UnRegisterTickFunction();
		}
	}

	FPhysScene* PhysScene = GetPhysicsScene();
	if (PhysicsScene == NULL)
	{
		return;
	}

#if WITH_PHYSX
	
	// When ticking the main scene, clean up any physics engine resources (once a frame)
	DeferredPhysResourceCleanup();
#endif

	// Update gravity in case it changed
	FVector DefaultGravity( 0.f, 0.f, GetGravityZ() );

	static const auto CVar_MaxPhysicsDeltaTime = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("p.MaxPhysicsDeltaTime"));
	PhysScene->SetUpForFrame(&DefaultGravity, DeltaSeconds, UPhysicsSettings::Get()->MaxPhysicsDeltaTime);
}

void UWorld::StartPhysicsSim()
{
	FPhysScene* PhysScene = GetPhysicsScene();
	if (PhysScene == NULL)
	{
		return;
	}

	PhysScene->StartFrame();
}

void UWorld::FinishPhysicsSim()
{
	FPhysScene* PhysScene = GetPhysicsScene();
	if (PhysScene == NULL)
	{
		return;
	}

	PhysScene->EndFrame(LineBatcher);
}

void UWorld::StartClothSim()
{
	FPhysScene* PhysScene = GetPhysicsScene();
	if (PhysScene == NULL)
	{
		return;
	}

	PhysScene->StartCloth();
}

// the physics tick functions

void FStartPhysicsTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	QUICK_SCOPE_CYCLE_COUNTER(FStartPhysicsTickFunction_ExecuteTick);
	check(Target);
	Target->StartPhysicsSim();
}

FString FStartPhysicsTickFunction::DiagnosticMessage()
{
	return TEXT("FStartPhysicsTickFunction");
}

void FEndPhysicsTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	QUICK_SCOPE_CYCLE_COUNTER(FEndPhysicsTickFunction_ExecuteTick);

	check(Target);
	FPhysScene* PhysScene = Target->GetPhysicsScene();
	if (PhysScene == NULL)
	{
		return;
	}
	FGraphEventRef PhysicsComplete = PhysScene->GetCompletionEvent();
	if (PhysicsComplete.GetReference() && !PhysicsComplete->IsComplete())
	{
		// don't release the next tick group until the physics has completed and we have run FinishPhysicsSim
		DECLARE_CYCLE_STAT(TEXT("FSimpleDelegateGraphTask.FinishPhysicsSim"),
			STAT_FSimpleDelegateGraphTask_FinishPhysicsSim,
			STATGROUP_TaskGraphTasks);

		MyCompletionGraphEvent->DontCompleteUntil(
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
				FSimpleDelegateGraphTask::FDelegate::CreateUObject(Target, &UWorld::FinishPhysicsSim),
				GET_STATID(STAT_FSimpleDelegateGraphTask_FinishPhysicsSim), PhysicsComplete, ENamedThreads::GameThread
			)
		);
	}
	else
	{
		// it was already done, so let just do it.
		Target->FinishPhysicsSim();
	}

#if PHYSX_MEMORY_VALIDATION
	static int32 Frequency = 0;
	if (Frequency++ > 10)
	{
		Frequency = 0;
		GPhysXAllocator->ValidateHeaders();
	}
#endif
}

FString FEndPhysicsTickFunction::DiagnosticMessage()
{
	return TEXT("FEndPhysicsTickFunction");
}

void FStartClothSimulationFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	QUICK_SCOPE_CYCLE_COUNTER(FStartClothSimulationFunction_ExecuteTick);

	check(Target);
	Target->StartClothSim();
}

FString FStartClothSimulationFunction::DiagnosticMessage()
{
	return TEXT("FStartClothSimulationFunction");
}

void FEndClothSimulationFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	QUICK_SCOPE_CYCLE_COUNTER(FEndClothSimulationFunction_ExecuteTick);

	//We currently have nothing to do in this tick group, but we still want to wait on cloth simulation so that PostPhysics is ensured this is done
	check(Target);
	FPhysScene* PhysScene = Target->GetPhysicsScene();
	if (PhysScene == NULL)
	{
		return;
	}
	PhysScene->WaitClothScene();
}

FString FEndClothSimulationFunction::DiagnosticMessage()
{
	return TEXT("FStartClothSimulationFunction");
}

// NVCHANGE_BEGIN: JCAO - Add PhysXLevel and blueprint node
bool NvIsPhysXHighSupported()
{
#if WITH_CUDA_CONTEXT
	const uint32 MB = 1024 * 1024;

	bool Ret = false;

	if (GPhysXSDK == NULL)
		return false;

	// SLI and PhysX high are not compatible due to cuda interop not working with SLI
	if (GNumActiveGPUsForRendering > 1)
		return false;

	PxCudaContextManagerDesc CtxMgrDesc;
	PxCudaContextManager* CudaCtxMgr = PxCreateCudaContextManager(GPhysXSDK->getFoundation(), CtxMgrDesc, GPhysXSDK->getProfileZoneManager());
	if (CudaCtxMgr == NULL)
		return false;

	// Fermi and 1 GB of GPU memory for shared GFX and PhysX
	// Fermi and 768 MB of GPU memory for dedicated PhysX
	if (CudaCtxMgr->contextIsValid() /*&&
									 CudaCtxMgr->supportsArchSM20()*/)
	{
		//if (CudaCtxMgr->usingDedicatedPhysXGPU() &&
		//	CudaCtxMgr->getDeviceTotalMemBytes() >= 768 * MB)
		//{
		//	Ret = true;
		//}
		//else if (CudaCtxMgr->getDeviceTotalMemBytes() >= 1024 * MB)
		//{
		Ret = true;
		//}
	}

	CudaCtxMgr->release();
	return Ret;
#else
	return false;
#endif
}
// NVCHANGE_END: JCAO - Add PhysXLevel and blueprint node

// NVCHANGE_BEGIN: JCAO - Create a global cuda context used for all the physx scene
#if WITH_CUDA_CONTEXT
#if !USE_MULTIPLE_GPU_DISPATCHER_FOR_EACH_SCENE
PxCudaContextManager* GetCudaContextManager()
{
	if (GCudaContextManager == NULL)
	{
		physx::PxCudaContextManagerDesc CtxMgrDesc;

		if (GDynamicRHI && !GUsingNullRHI && GDynamicRHI)
		{
			CtxMgrDesc.graphicsDevice = GDynamicRHI->RHIGetNativeDevice();

#if WITH_CUDA_INTEROP
			CtxMgrDesc.interopMode = physx::PxCudaInteropMode::D3D11_INTEROP;
#endif // WITH_CUDA_INTEROP

			GCudaContextManager = PxCreateCudaContextManager(GPhysXSDK->getFoundation(), CtxMgrDesc, GPhysXSDK->getProfileZoneManager());

			if (GCudaContextManager && !GCudaContextManager->contextIsValid())
			{
				TerminateCudaContextManager();
			}

			if (GCudaContextManager && GCudaContextManager->supportsArchSM30())
			{
				GCudaContextManager->setUsingConcurrentStreams(false);
			}
		}
	}

	return GCudaContextManager;
}

void TerminateCudaContextManager()
{
	if (GCudaContextManager != NULL)
	{
		GCudaContextManager->release();
		GCudaContextManager = NULL;
	}
}
#endif // USE_MULTIPLE_GPU_DISPATCHER_FOR_EACH_SCENE
#endif //WITH_CUDA_CONTEXT
// NVCHANGE_END: JCAO - Create a global cuda context used for all the physx scene

void PvdConnect(FString Host);

//////// GAME-LEVEL RIGID BODY PHYSICS STUFF ///////
void InitGamePhys()
{
#if WITH_BOX2D
	FPhysicsIntegration2D::InitializePhysics();
#endif

#if WITH_PHYSX
	// Do nothing if SDK already exists
	if(GPhysXFoundation != NULL)
	{
		return;
	}

	// Make sure 
	LoadPhysXModules();

	// Create Foundation
	GPhysXAllocator = new FPhysXAllocator();
	FPhysXErrorCallback* ErrorCallback = new FPhysXErrorCallback();

	GPhysXFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, *GPhysXAllocator, *ErrorCallback);
	check(GPhysXFoundation);

#if PHYSX_MEMORY_STATS
	// Want names of PhysX allocations
	GPhysXFoundation->setReportAllocationNames(true);
#endif

	// Create profile manager
	GPhysXProfileZoneManager = &PxProfileZoneManager::createProfileZoneManager(GPhysXFoundation);
	check(GPhysXProfileZoneManager);

	// Create Physics
	PxTolerancesScale PScale;
	PScale.length = CVarToleranceScaleLength.GetValueOnGameThread();
	PScale.mass = CVarTolerenceScaleMass.GetValueOnGameThread();
	PScale.speed = CVarToleranceScaleSpeed.GetValueOnGameThread();

	GPhysXSDK = PxCreatePhysics(PX_PHYSICS_VERSION, *GPhysXFoundation, PScale, false, GPhysXProfileZoneManager);
	check(GPhysXSDK);

	FPhysxSharedData::Initialize();

	GPhysCommandHandler = new FPhysCommandHandler();

	FCoreUObjectDelegates::PreGarbageCollect.AddRaw(GPhysCommandHandler, &FPhysCommandHandler::Flush);

	// Init Extensions
	PxInitExtensions(*GPhysXSDK);
#if WITH_VEHICLE
	PxInitVehicleSDK(*GPhysXSDK);
#endif

	if (CVarUseUnifiedHeightfield.GetValueOnGameThread())
	{
		//Turn on PhysX 3.3 unified height field collision detection. 
		//This approach shares the collision detection code between meshes and height fields such that height fields behave identically to the equivalent terrain created as a mesh. 
		//This approach facilitates mixing the use of height fields and meshes in the application with no tangible difference in collision behavior between the two approaches except that 
		//heightfield thickness is not supported for unified heightfields.
		PxRegisterUnifiedHeightFields(*GPhysXSDK);
	}
	else
	{
		PxRegisterHeightFields(*GPhysXSDK);
	}

	if( FParse::Param( FCommandLine::Get(), TEXT( "PVD" ) ) )
	{
		PvdConnect(TEXT("localhost"));
	}


#if WITH_PHYSICS_COOKING || WITH_RUNTIME_PHYSICS_COOKING
	// Create Cooking
	PxCookingParams PCookingParams(PScale);
	PCookingParams.meshWeldTolerance = 0.1f; // Weld to 1mm precision
	PCookingParams.meshPreprocessParams = PxMeshPreprocessingFlags(PxMeshPreprocessingFlag::eWELD_VERTICES | PxMeshPreprocessingFlag::eREMOVE_UNREFERENCED_VERTICES | PxMeshPreprocessingFlag::eREMOVE_DUPLICATED_TRIANGLES);
	PCookingParams.targetPlatform = PxPlatform::ePC;
	//PCookingParams.meshCookingHint = PxMeshCookingHint::eCOOKING_PERFORMANCE;
	//PCookingParams.meshSizePerformanceTradeOff = 0.0f;
	GPhysXCooking = PxCreateCooking(PX_PHYSICS_VERSION, *GPhysXFoundation, PCookingParams);
	check(GPhysXCooking);
#endif

#if WITH_APEX
	// Build the descriptor for the APEX SDK
	NxApexSDKDesc ApexDesc;
	ApexDesc.physXSDK				= GPhysXSDK;	// Pointer to the PhysXSDK
	ApexDesc.cooking				= GPhysXCooking;	// Pointer to the cooking library
	// NVCHANGE_BEGIN: JCAO - Replace vector fields with APEX turbulence velocity fields
	ApexDesc.renderResourceManager = &GApexRenderResourceManager;
	// NVCHANGE_END: JCAO - Replace vector fields with APEX turbulence velocity fields
	ApexDesc.resourceCallback		= &GApexResourceCallback;	// The resource callback is how APEX asks the application to find assets when it needs them

	// Create the APEX SDK
	NxApexCreateError ErrorCode;
	GApexSDK = NxCreateApexSDK(ApexDesc, &ErrorCode);
	check(ErrorCode == APEX_CE_NO_ERROR);
	check(GApexSDK);

#if APEX_STATICALLY_LINKED
	// We need to instantiate the module if we have statically linked them
	// Otherwise all createModule functions will fail
	instantiateModuleDestructible();

#if WITH_APEX_CLOTHING
	instantiateModuleClothing();
#endif

#if WITH_APEX_LEGACY
	instantiateModuleLegacy();
#endif
	// NVCHANGE_BEGIN : JCAO - Add Turbulence Module
#if WITH_APEX_TURBULENCE
	instantiateModuleTurbulenceFS();
	instantiateModuleParticles();
#endif //WITH_APEX_TURBULENCE
	// NVCHANGE_END : JCAO - Add Turbulence Module
#endif

	// 1 legacy module for all in APEX 1.3
	// Load the only 1 legacy module
#if WITH_APEX_LEGACY
	GApexModuleLegacy = GApexSDK->createModule("Legacy");
	check(GApexModuleLegacy);
#endif // WITH_APEX_LEGACY

	// Load APEX Destruction module
	GApexModuleDestructible = static_cast<NxModuleDestructible*>(GApexSDK->createModule("Destructible"));
	check(GApexModuleDestructible);

	// Set Destructible module parameters
	NxParameterized::Interface* ModuleParams = GApexModuleDestructible->getDefaultModuleDesc();
	// ModuleParams contains the default module descriptor, which may be modified here before calling the module init function
	GApexModuleDestructible->init(*ModuleParams);
	// Disabling dynamic LOD
	GApexModuleDestructible->setLODEnabled(false);
	// Set chunk report for fracture effect callbacks
	GApexModuleDestructible->setChunkReport(&GApexChunkReport);

	
	GApexModuleDestructible->setMaxDynamicChunkIslandCount((physx::PxU32)FMath::Max(CVarAPEXMaxDestructibleDynamicChunkIslandCount.GetValueOnGameThread(), 0));
	GApexModuleDestructible->setMaxChunkCount((physx::PxU32)FMath::Max(CVarAPEXMaxDestructibleDynamicChunkCount.GetValueOnGameThread(), 0));
	GApexModuleDestructible->setSortByBenefit(CVarAPEXSortDynamicChunksByBenefit.GetValueOnGameThread() != 0);

	GApexModuleDestructible->scheduleChunkStateEventCallback(NxDestructibleCallbackSchedule::FetchResults);

	// APEX 1.3 to preserve 1.2 behavior
	GApexModuleDestructible->setUseLegacyDamageRadiusSpread(true); 
	GApexModuleDestructible->setUseLegacyChunkBoundsTesting(true);

#if WITH_APEX_CLOTHING
	// Load APEX Clothing module
	GApexModuleClothing = static_cast<NxModuleClothing*>(GApexSDK->createModule("Clothing"));
	check(GApexModuleClothing);
	// Set Clothing module parameters
	ModuleParams = GApexModuleClothing->getDefaultModuleDesc();

	// Can be tuned for switching between more memory and more spikes.
	NxParameterized::setParamU32(*ModuleParams, "maxUnusedPhysXResources", 5);

	// If true, let fetch results tasks run longer than the fetchResults call. 
	// Setting to true could not ensure same finish timing with Physx simulation phase
	NxParameterized::setParamBool(*ModuleParams, "asyncFetchResults", false);

	// ModuleParams contains the default module descriptor, which may be modified here before calling the module init function
	GApexModuleClothing->init(*ModuleParams);
#endif	//WITH_APEX_CLOTHING

	// NVCHANGE_BEGIN : JCAO - Add Turbulence Module
#if WITH_APEX_TURBULENCE
	GApexModuleTurbulenceFS = static_cast<NxModuleTurbulenceFS*>(GApexSDK->createModule("TurbulenceFS", &ErrorCode));
//	check(GApexModuleTurbulenceFS);
	//GApexModuleTurbulenceFS->enableOutputVelocityField();
	//GApexModuleTurbulenceFS->setLODEnabled(false);
	// NVCHANGE_BEGIN: JCAO - Add custom filter shader for turbulence interacting with the kinematic rigid body
	//GApexModuleTurbulenceFS->setCustomFilterShader(TurbulenceFSFilterShader, NULL, 0);
	// NVCHANGE_END: JCAO - Add custom filter shader for turbulence interacting with the kinematic rigid body

	GApexModuleParticles = static_cast<NxModuleParticles*>(GApexSDK->createModule("Particles"));
//	check(GApexModuleParticles);

//	GApexModuleBasicFS = static_cast<NxModuleBasicFS*>(GApexModuleParticles->getModule("BasicFS"));
	//check(GApexModuleBasicFS);
	//GApexModuleBasicFS->setLODEnabled(false);

//	GApexModuleFieldSampler = static_cast<NxModuleFieldSampler*>(GApexModuleParticles->getModule("FieldSampler"));
	//check(GApexModuleFieldSampler);
	//GApexModuleFieldSampler->setLODEnabled(false);
#endif // WITH_APEX_TURBULENCE
	// NVCHANGE_END : JCAO - Add Turbulence Module

#endif // #if WITH_APEX

#if WITH_FLEX
	if (flexInit(FLEX_VERSION, FlexErrorFunc) == eFlexErrorNone)
	{
		GFlexIsInitialized = true;
	}
#endif // WITH_FLEX

#endif // WITH_PHYSX
}

void TermGamePhys()
{

#if WITH_FLEX
	if (GFlexIsInitialized)
	{
		flexShutdown();
		GFlexIsInitialized = false;
	}
#endif // WITH_FLEX

#if WITH_BOX2D
	FPhysicsIntegration2D::ShutdownPhysics();
#endif

#if WITH_PHYSX
	// NVCHANGE_BEGIN: JCAO - Fix the crash caused by missing release apex scene
	// Force flushing the deferred commands before releasing APEX SDK
	if (GPhysCommandHandler != NULL)
	{
		GPhysCommandHandler->Flush();
	}
	// NVCHANGE_END: JCAO - Fix the crash caused by missing release apex scene
	// NVCHANGE_BEGIN: JCAO - Create a global cuda context used for all the physx scene
#if WITH_CUDA_CONTEXT
#if !USE_MULTIPLE_GPU_DISPATCHER_FOR_EACH_SCENE
	TerminateCudaContextManager();
#endif
#endif //WITH_CUDA_CONTEXT
	// NVCHANGE_END: JCAO - Create a global cuda context used for all the physx scene

	FPhysxSharedData::Terminate();

	// Do nothing if they were never initialized
	if(GPhysXFoundation == NULL)
	{
		return;
	}

	if (GPhysCommandHandler != NULL)
	{
		GPhysCommandHandler->Flush();	//finish off any remaining commands
		delete GPhysCommandHandler;
		GPhysCommandHandler = NULL;
	}

#if WITH_APEX
#if WITH_APEX_LEGACY
	if(GApexModuleLegacy != NULL)
	{
		GApexModuleLegacy->release();
		GApexModuleLegacy = NULL;
	}
#endif // WITH_APEX_LEGACY
	if(GApexSDK != NULL)
	{
		GApexSDK->release(); 
		GApexSDK = NULL;
	}
#endif	// #if WITH_APEX

#if WITH_PHYSICS_COOKING || WITH_RUNTIME_PHYSICS_COOKING
	if(GPhysXCooking != NULL)
	{
		GPhysXCooking->release(); 
		GPhysXCooking = NULL;
	}
#endif

	PxCloseExtensions();
	PxCloseVehicleSDK();

	if(GPhysXSDK != NULL)
	{
		GPhysXSDK->release();
		GPhysXSDK = NULL;
	}

	// @todo delete FPhysXAllocator
	// @todo delete FPhysXOutputStream

	UnloadPhysXModules();
#endif
}

/** 
*	Perform any cleanup of physics engine resources. 
*	This is deferred because when closing down the game, you want to make sure you are not destroying a mesh after the physics SDK has been shut down.
*/
void DeferredPhysResourceCleanup()
{
#if WITH_PHYSX
	// Release all tri meshes and reset array
	for(int32 MeshIdx=0; MeshIdx<GPhysXPendingKillTriMesh.Num(); MeshIdx++)
	{
		PxTriangleMesh* PTriMesh = GPhysXPendingKillTriMesh[MeshIdx];
		check(PTriMesh);
		PTriMesh->release();
		GPhysXPendingKillTriMesh[MeshIdx] = NULL;
	}
	GPhysXPendingKillTriMesh.Reset();

	// Release all convex meshes and reset array
	for(int32 MeshIdx=0; MeshIdx<GPhysXPendingKillConvex.Num(); MeshIdx++)
	{
		PxConvexMesh* PConvexMesh = GPhysXPendingKillConvex[MeshIdx];
		check(PConvexMesh);
		PConvexMesh->release();
		GPhysXPendingKillConvex[MeshIdx] = NULL;
	}
	GPhysXPendingKillConvex.Reset();

	// Release all heightfields and reset array
	for(int32 HfIdx=0; HfIdx<GPhysXPendingKillHeightfield.Num(); HfIdx++)
	{
		PxHeightField* PHeightfield = GPhysXPendingKillHeightfield[HfIdx];
		check(PHeightfield);
		PHeightfield->release();
		GPhysXPendingKillHeightfield[HfIdx] = NULL;
	}
	GPhysXPendingKillHeightfield.Reset();

	// Release all materials and reset array
	for(int32 MeshIdx=0; MeshIdx<GPhysXPendingKillMaterial.Num(); MeshIdx++)
	{
		PxMaterial* PMaterial = GPhysXPendingKillMaterial[MeshIdx];
		check(PMaterial);
		PMaterial->release();
		GPhysXPendingKillMaterial[MeshIdx] = NULL;
	}
	GPhysXPendingKillMaterial.Reset();
#endif
}

