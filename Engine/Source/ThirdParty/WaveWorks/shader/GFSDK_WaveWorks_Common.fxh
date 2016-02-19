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
#ifndef _GFSDK_WAVEWORKS_COMMON_FX
#define _GFSDK_WAVEWORKS_COMMON_FX
/*
 *
 *
 */
#if defined(GFSDK_WAVEWORKS_SM4) || defined(GFSDK_WAVEWORKS_SM5)
	#define SampleTex2D(nv_waveworks_comm0,nv_waveworks_comm1,nv_waveworks_comm2) nv_waveworks_comm0.Sample(nv_waveworks_comm1,nv_waveworks_comm2)
	#define SampleTex2Dlod(nv_waveworks_comm0,nv_waveworks_comm1,nv_waveworks_comm2,nv_waveworks_comm3) nv_waveworks_comm0.SampleLevel(nv_waveworks_comm1,nv_waveworks_comm2,nv_waveworks_comm3)
	#define BEGIN_CBUFFER(name,slot) cbuffer name : register(b##slot) {
	#define END_CBUFFER };
	#define SEMANTIC(x) : x
#elif defined(GFSDK_WAVEWORKS_SM3)
	#define SampleTex2D(nv_waveworks_comm0,nv_waveworks_comm1,nv_waveworks_comm2) tex2D(nv_waveworks_comm1,nv_waveworks_comm2)
	#define SampleTex2Dlod(nv_waveworks_comm0,nv_waveworks_comm1,nv_waveworks_comm2,nv_waveworks_comm3) tex2Dlod(nv_waveworks_comm1,float4(nv_waveworks_comm2,0,nv_waveworks_comm3))
	#define BEGIN_CBUFFER(name,slot)
	#define END_CBUFFER
	#define SV_Target COLOR
	#define SV_Position POSITION
	#define SEMANTIC(x) : x
#elif defined(GFSDK_WAVEWORKS_GNM)
	#define SampleTex2D(nv_waveworks_comm0,nv_waveworks_comm1,nv_waveworks_comm2) nv_waveworks_comm0.Sample(nv_waveworks_comm1,nv_waveworks_comm2)
	#define SampleTex2Dlod(nv_waveworks_comm0,nv_waveworks_comm1,nv_waveworks_comm2,nv_waveworks_comm3) nv_waveworks_comm0.SampleLOD(nv_waveworks_comm1,nv_waveworks_comm2,nv_waveworks_comm3)
	#define BEGIN_CBUFFER(name,slot) ConstantBuffer name : register(b##slot) {
	#define END_CBUFFER };
	#define SV_Target S_TARGET_OUTPUT
	#define SV_Position S_POSITION
	#define SEMANTIC(x) : x
#elif defined(GFSDK_WAVEWORKS_GL)
	#define SampleTex2D(nv_waveworks_comm0,nv_waveworks_comm1,nv_waveworks_comm2) texture(nv_waveworks_comm1,nv_waveworks_comm2)
	#define SampleTex2Dlod(nv_waveworks_comm0,nv_waveworks_comm1,nv_waveworks_comm2,nv_waveworks_comm3) textureLod(nv_waveworks_comm1,nv_waveworks_comm2,nv_waveworks_comm3)
	#define BEGIN_CBUFFER(name,slot)
	#define END_CBUFFER
	#define SEMANTIC(x)
	#define float2 vec2
	#define float3 vec3
	#define float4 vec4
	#define float4x3 mat3x4
	vec3 mul(vec4 v, mat3x4 m) { return v * m; }
	#define lerp mix
	#define saturate(x) clamp(x,0.0,1.0)
#else
	#error Shader model not defined (expected GFSDK_WAVEWORKS_SM3, GFSDK_WAVEWORKS_SM4, GFSDK_WAVEWORKS_SM5, GFSDK_WAVEWORKS_GNM or GFSDK_WAVEWORKS_GL)
#endif
/*
 *
 *
 */
#endif /* _GFSDK_WAVEWORKS_COMMON_FX */
