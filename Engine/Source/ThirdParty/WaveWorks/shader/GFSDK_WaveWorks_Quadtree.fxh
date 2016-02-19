/*
 * This code contains NVIDIA Confidential Information and is disclosed 
 * under the Mutual Non-Disclosure Agreement. 
 * 
 * Notice 
 * ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
 * NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
 * THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
 * MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
 * 
 * NVIDIA Corporation assumes no responsibility for the consequences of use of such 
 * information or for any infringement of patents or other rights of third parties that may 
 * result from its use. No license is granted by implication or otherwise under any patent 
 * or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
 * expressly authorized by NVIDIA.  Details are subject to change without notice. 
 * This code supersedes and replaces all information previously supplied. 
 * NVIDIA Corporation products are not authorized for use as critical 
 * components in life support devices or systems without express written approval of 
 * NVIDIA Corporation. 
 * 
 * Copyright © 2008- 2013 NVIDIA Corporation. All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property and proprietary
 * rights in and to this software and related documentation and any modifications thereto.
 * Any use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation is
 * strictly prohibited.
 */
#ifndef _GFSDK_WAVEWORKS_QUADPATCH_FX
#define _GFSDK_WAVEWORKS_QUADPATCH_FX
/*
 *
 *
 */
#include "GFSDK_WaveWorks_Common.fxh"
/*
 *
 *
 */
#if defined(GFSDK_WAVEWORKS_SM3) || defined(GFSDK_WAVEWORKS_GL)
	#define GFSDK_WAVEWORKS_BEGIN_GEOM_VS_CBUFFER(Label)
	#define GFSDK_WAVEWORKS_END_GEOM_VS_CBUFFER
#endif
#if defined( GFSDK_WAVEWORKS_USE_TESSELLATION )
	GFSDK_WAVEWORKS_BEGIN_GEOM_HS_CBUFFER(nv_waveworks_quad0)
	GFSDK_WAVEWORKS_DECLARE_GEOM_HS_CONSTANT(float4, nv_waveworks_quad1, 0)
	GFSDK_WAVEWORKS_DECLARE_GEOM_HS_CONSTANT(float4, nv_waveworks_quad2, 1)
	GFSDK_WAVEWORKS_END_GEOM_HS_CBUFFER
#endif
GFSDK_WAVEWORKS_BEGIN_GEOM_VS_CBUFFER(nv_waveworks_quad3)
GFSDK_WAVEWORKS_DECLARE_GEOM_VS_CONSTANT(float4x3, nv_waveworks_quad4, 0)
GFSDK_WAVEWORKS_DECLARE_GEOM_VS_CONSTANT(float4, nv_waveworks_quad5, 3)
GFSDK_WAVEWORKS_DECLARE_GEOM_VS_CONSTANT(float4, nv_waveworks_quad6, 4)
GFSDK_WAVEWORKS_END_GEOM_VS_CBUFFER
struct GFSDK_WAVEWORKS_VERTEX_INPUT
{
	float4 nv_waveworks_quad7 SEMANTIC(POSITION);
};
float3 GFSDK_WaveWorks_GetUndisplacedVertexWorldPosition(GFSDK_WAVEWORKS_VERTEX_INPUT In)
{
	float2 nv_waveworks_quad8 = In.nv_waveworks_quad7.xy;
#if !defined(GFSDK_WAVEWORKS_USE_TESSELLATION)
	float nv_waveworks_quad9 = 0.5f;
	float nv_waveworks_quad10 = nv_waveworks_quad6.w;
	float2 nv_waveworks_quad11 = nv_waveworks_quad8;
	float2 nv_waveworks_quad12 = nv_waveworks_quad11;
	float nv_waveworks_quad13 = 0.f;
	for(int nv_waveworks_quad14 = 0; nv_waveworks_quad14 != 4; ++nv_waveworks_quad14) {
		float2 nv_waveworks_quad15;
		float2 nv_waveworks_quad16 = modf(nv_waveworks_quad9*nv_waveworks_quad11.xy,nv_waveworks_quad15);
		if(0.5f == nv_waveworks_quad16.x && 0.5f == nv_waveworks_quad16.y) nv_waveworks_quad12.xy = nv_waveworks_quad11.xy - nv_waveworks_quad10;
		else if(0.5f == nv_waveworks_quad16.x) nv_waveworks_quad12.x = nv_waveworks_quad11.x + nv_waveworks_quad10;
		else if(0.5f == nv_waveworks_quad16.y) nv_waveworks_quad12.y = nv_waveworks_quad11.y + nv_waveworks_quad10;
		float3 nv_waveworks_quad17 = mul(float4(nv_waveworks_quad11,0.f,1.f), nv_waveworks_quad4) - nv_waveworks_quad5.xyz;
		float nv_waveworks_quad18 = length(nv_waveworks_quad17);
		float nv_waveworks_quad19 = log2(nv_waveworks_quad18 * nv_waveworks_quad6.x) + 1.f;
		nv_waveworks_quad13 = saturate(nv_waveworks_quad19 - float(nv_waveworks_quad14));
		if(nv_waveworks_quad13 < 1.f) {
			break;
		} else {
			nv_waveworks_quad11 = nv_waveworks_quad12;
			nv_waveworks_quad9 *= 0.5f;
			nv_waveworks_quad10 *= -2.f;
		}
	}
	nv_waveworks_quad8.xy = lerp(nv_waveworks_quad11, nv_waveworks_quad12, nv_waveworks_quad13);
#endif 
	return mul(float4(nv_waveworks_quad8,In.nv_waveworks_quad7.zw), nv_waveworks_quad4);
}
#if defined( GFSDK_WAVEWORKS_USE_TESSELLATION )
float GFSDK_WaveWorks_GetEdgeTessellationFactor(float4 vertex1, float4 vertex2)
{
	float4 nv_waveworks_quad20 = 0.5*(vertex1 + vertex2);
	float nv_waveworks_quad21 = length (vertex1 - vertex2);
	float nv_waveworks_quad22 = length(nv_waveworks_quad1.xyz - nv_waveworks_quad20.xyz);
	return max(1.0,nv_waveworks_quad2.x * nv_waveworks_quad21 / nv_waveworks_quad22);
}
float GFSDK_WaveWorks_GetVertexTargetTessellatedEdgeLength(float3 vertex)
{
	float nv_waveworks_quad23 = length(nv_waveworks_quad1.xyz - vertex.xyz);
	return nv_waveworks_quad23 / nv_waveworks_quad2.x;
}
#endif
/*
 *
 *
 */
#endif /* _GFSDK_WAVEWORKS_QUADPATCH_FX */
