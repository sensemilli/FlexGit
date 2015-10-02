// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TextureResource.h"

/*=============================================================================
	WaveWorks.h: WaveWorks related classes.
=============================================================================*/

/**
 * FWaveWorksResource type for WaveWorks resources.
 */
class FWaveWorksResource : public FRenderResource, public FDeferredUpdateResource
{
public:

	/**
	 * Constructor
	 * @param InOwner - WaveWorks object to create a resource for
	 */
	FWaveWorksResource(const class UWaveWorks* InOwner)
		: Owner(InOwner)
	{
	}

	// FRenderResource interface.

	/**
	 * Initializes the dynamic RHI resource and/or RHI render target used by this resource.
	 * Called when the resource is initialized, or when reseting all RHI resources.
	 * Resources that need to initialize after a D3D device reset must implement this function.
	 * This is only called by the rendering thread.
	 */
	virtual void InitDynamicRHI();

	/**
	 * Releases the dynamic RHI resource and/or RHI render target resources used by this resource.
	 * Called when the resource is released, or when reseting all RHI resources.
	 * Resources that need to release before a D3D device reset must implement this function.
	 * This is only called by the rendering thread.
	 */
	virtual void ReleaseDynamicRHI();

	// FDeferredClearResource interface

	/**
	 * Generates the next frame of the WaveWorks simuation
	 */
	virtual void UpdateDeferredResource(FRHICommandListImmediate &, bool bClearRenderTarget = true);

	/**
	* @return WaveWorksRHI for rendering
	*/
	FWaveWorksRHIRef GetWaveWorksRHI() { return WaveWorksRHI; }

private:
	/** The UWaveWorks which this resource represents. */
	const class UWaveWorks* Owner;
	/** WaveWorks resource used for rendering with and resolving to */
	FWaveWorksRHIRef WaveWorksRHI;
};
