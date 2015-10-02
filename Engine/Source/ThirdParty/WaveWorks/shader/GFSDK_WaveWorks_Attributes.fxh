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
#ifndef _GFSDK_WAVEWORKS_ATTRIBUTES_FX
#define _GFSDK_WAVEWORKS_ATTRIBUTES_FX
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
	#define GFSDK_WAVEWORKS_BEGIN_ATTR_VS_CBUFFER(Label)
	#define GFSDK_WAVEWORKS_END_ATTR_VS_CBUFFER
	#define GFSDK_WAVEWORKS_BEGIN_ATTR_PS_CBUFFER(Label) 
	#define GFSDK_WAVEWORKS_END_ATTR_PS_CBUFFER
#endif
#if defined( GFSDK_WAVEWORKS_USE_TESSELLATION )
	#define GFSDK_WAVEWORKS_BEGIN_ATTR_DISPLACEMENT_CBUFFER(Label) GFSDK_WAVEWORKS_BEGIN_ATTR_DS_CBUFFER(Label)
	#define GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_CONSTANT(Type,Label,Regoff) GFSDK_WAVEWORKS_DECLARE_ATTR_DS_CONSTANT(Type,Label,Regoff) 
	#define GFSDK_WAVEWORKS_END_ATTR_DISPLACEMENT_CBUFFER GFSDK_WAVEWORKS_END_ATTR_DS_CBUFFER
	#define GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER(SampLabel,TexLabel,Regoff) GFSDK_WAVEWORKS_DECLARE_ATTR_DS_SAMPLER(SampLabel,TexLabel,Regoff)
	#define GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER_TEXTUREARRAY(SampLabel,TexLabel,Regoff) GFSDK_WAVEWORKS_DECLARE_ATTR_DS_SAMPLER_TEXTUREARRAY(SampLabel,TexLabel,Regoff)
#else
	#define GFSDK_WAVEWORKS_BEGIN_ATTR_DISPLACEMENT_CBUFFER(Label) GFSDK_WAVEWORKS_BEGIN_ATTR_VS_CBUFFER(Label)
	#define GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_CONSTANT(Type,Label,Regoff)  GFSDK_WAVEWORKS_DECLARE_ATTR_VS_CONSTANT(Type,Label,Regoff) 
	#define GFSDK_WAVEWORKS_END_ATTR_DISPLACEMENT_CBUFFER GFSDK_WAVEWORKS_END_ATTR_VS_CBUFFER
	#define GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER(SampLabel,TexLabel,Regoff) GFSDK_WAVEWORKS_DECLARE_ATTR_VS_SAMPLER(SampLabel,TexLabel,Regoff)
	#define GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER_TEXTUREARRAY(SampLabel,TexLabel,Regoff) GFSDK_WAVEWORKS_DECLARE_ATTR_VS_SAMPLER_TEXTUREARRAY(SampLabel,TexLabel,Regoff)
#endif
GFSDK_WAVEWORKS_BEGIN_ATTR_DISPLACEMENT_CBUFFER(nv_waveworks_attr0)
GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_CONSTANT(float3, nv_waveworks_attr1, 0)
#if defined( GFSDK_WAVEWORKS_GL )
	GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_CONSTANT(float, nv_waveworks_attr2, 1)
#else
	GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_CONSTANT(float, nv_waveworks_attr3, 1)
#endif
GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_CONSTANT(float4, nv_waveworks_attr4, 2)
GFSDK_WAVEWORKS_END_ATTR_DISPLACEMENT_CBUFFER
GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER(nv_waveworks_attr5, nv_waveworks_attr6, 0)
GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER(nv_waveworks_attr7, nv_waveworks_attr8, 1)
GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER(nv_waveworks_attr9, nv_waveworks_attr10, 2)
GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER(nv_waveworks_attr11, nv_waveworks_attr12, 3)
#if defined( GFSDK_WAVEWORKS_GL )
	GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER_TEXTUREARRAY(nv_waveworks_attr13, nv_waveworks_attr14, 4)
#endif
GFSDK_WAVEWORKS_BEGIN_ATTR_PS_CBUFFER(nv_waveworks_attr15)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nv_waveworks_attr16, 0)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nv_waveworks_attr17, 1)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nv_waveworks_attr18, 2)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nv_waveworks_attr19, 3)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nv_waveworks_attr20, 4)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nv_waveworks_attr21, 5)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nv_waveworks_attr22, 6)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nv_waveworks_attr23, 7)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nv_waveworks_attr24, 8)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nv_waveworks_attr25, 9)
GFSDK_WAVEWORKS_END_ATTR_PS_CBUFFER
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER(nv_waveworks_attr26, nv_waveworks_attr27, 0)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER(nv_waveworks_attr28, nv_waveworks_attr29, 1)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER(nv_waveworks_attr30, nv_waveworks_attr31, 2)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER(nv_waveworks_attr32, nv_waveworks_attr33, 3)
#if defined( GFSDK_WAVEWORKS_GL )
	GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER_TEXTUREARRAY(nv_waveworks_attr34, nv_waveworks_attr35, 4)
#endif
struct GFSDK_WAVEWORKS_INTERPOLATED_VERTEX_OUTPUT
{
    float4 nv_waveworks_attr36			SEMANTIC(TEXCOORD0);
    float4 nv_waveworks_attr37			SEMANTIC(TEXCOORD1);
    float4 nv_waveworks_attr38	SEMANTIC(TEXCOORD2);
    float3 nv_waveworks_attr39						SEMANTIC(TEXCOORD3);
};
struct GFSDK_WAVEWORKS_VERTEX_OUTPUT
{
    GFSDK_WAVEWORKS_INTERPOLATED_VERTEX_OUTPUT interp;
    float3 pos_world;
    float3 pos_world_undisplaced;
    float3 world_displacement;
};
GFSDK_WAVEWORKS_VERTEX_OUTPUT GFSDK_WaveWorks_GetDisplacedVertex(GFSDK_WAVEWORKS_VERTEX_INPUT In)
{
	float3 nv_waveworks_attr40 = GFSDK_WaveWorks_GetUndisplacedVertexWorldPosition(In);
	float  nv_waveworks_attr41 = length(nv_waveworks_attr1 - nv_waveworks_attr40);
	float2 nv_waveworks_attr42 = nv_waveworks_attr40.xy * nv_waveworks_attr4.x;
	float2 nv_waveworks_attr43 = nv_waveworks_attr40.xy * nv_waveworks_attr4.y;
	float2 nv_waveworks_attr44 = nv_waveworks_attr40.xy * nv_waveworks_attr4.z;
	float2 nv_waveworks_attr45 = nv_waveworks_attr40.xy * nv_waveworks_attr4.w;
	float4 nv_waveworks_attr46;
	float4 nv_waveworks_attr47 = 1.0/nv_waveworks_attr4.xyzw;
	nv_waveworks_attr46.x = 1.0;
	nv_waveworks_attr46.yzw = saturate(0.25*(nv_waveworks_attr47.yzw*24.0-nv_waveworks_attr41)/nv_waveworks_attr47.yzw);
	nv_waveworks_attr46.yzw *= nv_waveworks_attr46.yzw;
	#if defined(GFSDK_WAVEWORKS_GL)
		float3 nv_waveworks_attr48;
		if(nv_waveworks_attr2 > 0)
		{
			nv_waveworks_attr48 =  nv_waveworks_attr46.x * SampleTex2Dlod(nv_waveworks_attr14, nv_waveworks_attr13, vec3(nv_waveworks_attr42, 0.0), 0).xyz;
			nv_waveworks_attr48 += nv_waveworks_attr46.y==0? float3(0,0,0) : nv_waveworks_attr46.y * SampleTex2Dlod(nv_waveworks_attr14, nv_waveworks_attr13, vec3(nv_waveworks_attr43, 1.0), 0).xyz;
			nv_waveworks_attr48 += nv_waveworks_attr46.z==0? float3(0,0,0) : nv_waveworks_attr46.z * SampleTex2Dlod(nv_waveworks_attr14, nv_waveworks_attr13, vec3(nv_waveworks_attr44, 2.0), 0).xyz;
			nv_waveworks_attr48 += nv_waveworks_attr46.w==0? float3(0,0,0) : nv_waveworks_attr46.w * SampleTex2Dlod(nv_waveworks_attr14, nv_waveworks_attr13, vec3(nv_waveworks_attr45, 3.0), 0).xyz;
		}
		else
		{
			nv_waveworks_attr48 =  nv_waveworks_attr46.x * SampleTex2Dlod(nv_waveworks_attr6, nv_waveworks_attr5, nv_waveworks_attr42, 0).xyz;
			nv_waveworks_attr48 += nv_waveworks_attr46.y==0? float3(0,0,0) : nv_waveworks_attr46.y * SampleTex2Dlod(nv_waveworks_attr8, nv_waveworks_attr7, nv_waveworks_attr43, 0).xyz;
			nv_waveworks_attr48 += nv_waveworks_attr46.z==0? float3(0,0,0) : nv_waveworks_attr46.z * SampleTex2Dlod(nv_waveworks_attr10, nv_waveworks_attr9, nv_waveworks_attr44, 0).xyz;
			nv_waveworks_attr48 += nv_waveworks_attr46.w==0? float3(0,0,0) : nv_waveworks_attr46.w * SampleTex2Dlod(nv_waveworks_attr12, nv_waveworks_attr11, nv_waveworks_attr45, 0).xyz;
		}
	#else
		float3 nv_waveworks_attr48 =  nv_waveworks_attr46.x * SampleTex2Dlod(nv_waveworks_attr6, nv_waveworks_attr5, nv_waveworks_attr42, 0).xyz;
			   nv_waveworks_attr48 += nv_waveworks_attr46.y==0? float3(0,0,0) : nv_waveworks_attr46.y * SampleTex2Dlod(nv_waveworks_attr8, nv_waveworks_attr7, nv_waveworks_attr43, 0).xyz;
			   nv_waveworks_attr48 += nv_waveworks_attr46.z==0? float3(0,0,0) : nv_waveworks_attr46.z * SampleTex2Dlod(nv_waveworks_attr10, nv_waveworks_attr9, nv_waveworks_attr44, 0).xyz;
			   nv_waveworks_attr48 += nv_waveworks_attr46.w==0? float3(0,0,0) : nv_waveworks_attr46.w * SampleTex2Dlod(nv_waveworks_attr12, nv_waveworks_attr11, nv_waveworks_attr45, 0).xyz;
	#endif
	float3 nv_waveworks_attr49 = nv_waveworks_attr40 + nv_waveworks_attr48;
	GFSDK_WAVEWORKS_VERTEX_OUTPUT Output;
	Output.interp.nv_waveworks_attr39 = nv_waveworks_attr1 - nv_waveworks_attr49;
	Output.interp.nv_waveworks_attr36.xy = nv_waveworks_attr42;
	Output.interp.nv_waveworks_attr36.zw = nv_waveworks_attr43;
	Output.interp.nv_waveworks_attr37.xy = nv_waveworks_attr44;
	Output.interp.nv_waveworks_attr37.zw = nv_waveworks_attr45;
	Output.interp.nv_waveworks_attr38 = nv_waveworks_attr46;
	Output.pos_world = nv_waveworks_attr49;
	Output.pos_world_undisplaced = nv_waveworks_attr40;
	Output.world_displacement = nv_waveworks_attr48;
	return Output; 
}
GFSDK_WAVEWORKS_VERTEX_OUTPUT GFSDK_WaveWorks_GetDisplacedVertexAfterTessellation(float4 In0, float4 In1, float4 In2, float3 BarycentricCoords)
{
	float3 nv_waveworks_attr50 =	In0.xyz * BarycentricCoords.x + 
											In1.xyz * BarycentricCoords.y + 
											In2.xyz * BarycentricCoords.z;
	float3 nv_waveworks_attr40 = nv_waveworks_attr50;
	float4 nv_waveworks_attr46;
	float nv_waveworks_attr41 = length(nv_waveworks_attr1 - nv_waveworks_attr40);
	float4 nv_waveworks_attr47 = 1.0/nv_waveworks_attr4.xyzw;
	nv_waveworks_attr46.x = 1.0;
	nv_waveworks_attr46.yzw = saturate(0.25*(nv_waveworks_attr47.yzw*24.0-nv_waveworks_attr41)/nv_waveworks_attr47.yzw);
	nv_waveworks_attr46.yzw *= nv_waveworks_attr46.yzw;
	float2 nv_waveworks_attr42 = nv_waveworks_attr40.xy * nv_waveworks_attr4.x;
	float2 nv_waveworks_attr43 = nv_waveworks_attr40.xy * nv_waveworks_attr4.y;
	float2 nv_waveworks_attr44 = nv_waveworks_attr40.xy * nv_waveworks_attr4.z;
	float2 nv_waveworks_attr45 = nv_waveworks_attr40.xy * nv_waveworks_attr4.w;
	#if defined(GFSDK_WAVEWORKS_GL)
		float3 nv_waveworks_attr48;
		if(nv_waveworks_attr2 > 0)
		{
			nv_waveworks_attr48 =  nv_waveworks_attr46.x * SampleTex2Dlod(nv_waveworks_attr14, nv_waveworks_attr13, vec3(nv_waveworks_attr42, 0.0), 0).xyz;
			nv_waveworks_attr48 += nv_waveworks_attr46.y==0? float3(0,0,0) : nv_waveworks_attr46.y * SampleTex2Dlod(nv_waveworks_attr14, nv_waveworks_attr13, vec3(nv_waveworks_attr43, 1.0), 0).xyz;
			nv_waveworks_attr48 += nv_waveworks_attr46.z==0? float3(0,0,0) : nv_waveworks_attr46.z * SampleTex2Dlod(nv_waveworks_attr14, nv_waveworks_attr13, vec3(nv_waveworks_attr44, 2.0), 0).xyz;
			nv_waveworks_attr48 += nv_waveworks_attr46.w==0? float3(0,0,0) : nv_waveworks_attr46.w * SampleTex2Dlod(nv_waveworks_attr14, nv_waveworks_attr13, vec3(nv_waveworks_attr45, 3.0), 0).xyz;
		}
		else
		{
			nv_waveworks_attr48 =  nv_waveworks_attr46.x * SampleTex2Dlod(nv_waveworks_attr6, nv_waveworks_attr5, nv_waveworks_attr42, 0).xyz;
			nv_waveworks_attr48 += nv_waveworks_attr46.y==0? float3(0,0,0) : nv_waveworks_attr46.y * SampleTex2Dlod(nv_waveworks_attr8, nv_waveworks_attr7, nv_waveworks_attr43, 0).xyz;
			nv_waveworks_attr48 += nv_waveworks_attr46.z==0? float3(0,0,0) : nv_waveworks_attr46.z * SampleTex2Dlod(nv_waveworks_attr10, nv_waveworks_attr9, nv_waveworks_attr44, 0).xyz;
			nv_waveworks_attr48 += nv_waveworks_attr46.w==0? float3(0,0,0) : nv_waveworks_attr46.w * SampleTex2Dlod(nv_waveworks_attr12, nv_waveworks_attr11, nv_waveworks_attr45, 0).xyz;
		}
	#else
		float3 nv_waveworks_attr48 =  nv_waveworks_attr46.x * SampleTex2Dlod(nv_waveworks_attr6, nv_waveworks_attr5, nv_waveworks_attr42, 0).xyz;
			   nv_waveworks_attr48 += nv_waveworks_attr46.y==0? float3(0,0,0) : nv_waveworks_attr46.y * SampleTex2Dlod(nv_waveworks_attr8, nv_waveworks_attr7, nv_waveworks_attr43, 0).xyz;
			   nv_waveworks_attr48 += nv_waveworks_attr46.z==0? float3(0,0,0) : nv_waveworks_attr46.z * SampleTex2Dlod(nv_waveworks_attr10, nv_waveworks_attr9, nv_waveworks_attr44, 0).xyz;
			   nv_waveworks_attr48 += nv_waveworks_attr46.w==0? float3(0,0,0) : nv_waveworks_attr46.w * SampleTex2Dlod(nv_waveworks_attr12, nv_waveworks_attr11, nv_waveworks_attr45, 0).xyz;
	#endif
	float3 nv_waveworks_attr49 = nv_waveworks_attr40 + nv_waveworks_attr48;
	GFSDK_WAVEWORKS_VERTEX_OUTPUT Output;
	Output.interp.nv_waveworks_attr39 = nv_waveworks_attr1 - nv_waveworks_attr49;
	Output.interp.nv_waveworks_attr36.xy = nv_waveworks_attr42;
	Output.interp.nv_waveworks_attr36.zw = nv_waveworks_attr43;
	Output.interp.nv_waveworks_attr37.xy = nv_waveworks_attr44;
	Output.interp.nv_waveworks_attr37.zw = nv_waveworks_attr45;
	Output.interp.nv_waveworks_attr38 = nv_waveworks_attr46;
	Output.pos_world = nv_waveworks_attr49;
	Output.pos_world_undisplaced = nv_waveworks_attr40;
	Output.world_displacement = nv_waveworks_attr48;
	return Output; 
}
struct GFSDK_WAVEWORKS_SURFACE_ATTRIBUTES
{
	float3 normal;
	float3 eye_dir;
	float foam_surface_folding;
	float foam_turbulent_energy;
	float foam_wave_hats;
};
GFSDK_WAVEWORKS_SURFACE_ATTRIBUTES GFSDK_WaveWorks_GetSurfaceAttributes(GFSDK_WAVEWORKS_INTERPOLATED_VERTEX_OUTPUT In)
{
	float3 nv_waveworks_attr51 = normalize(In.nv_waveworks_attr39);
	float4 nv_waveworks_attr52;
	float4 nv_waveworks_attr53;
	float4 nv_waveworks_attr54;
	float4 nv_waveworks_attr55;
	#if defined(GFSDK_WAVEWORKS_GL)
		float3 nv_waveworks_attr48;
		if(nv_waveworks_attr2 > 0)
		{
			nv_waveworks_attr52 = SampleTex2D(nv_waveworks_attr35, nv_waveworks_attr34, vec3(In.nv_waveworks_attr36.xy, 0.0));
			nv_waveworks_attr53 = SampleTex2D(nv_waveworks_attr35, nv_waveworks_attr34, vec3(In.nv_waveworks_attr36.zw, 1.0));
			nv_waveworks_attr54 = SampleTex2D(nv_waveworks_attr35, nv_waveworks_attr34, vec3(In.nv_waveworks_attr37.xy, 2.0));
			nv_waveworks_attr55 = SampleTex2D(nv_waveworks_attr35, nv_waveworks_attr34, vec3(In.nv_waveworks_attr37.zw, 3.0));
		}
		else
		{
			nv_waveworks_attr52 = SampleTex2D(nv_waveworks_attr27, nv_waveworks_attr26, In.nv_waveworks_attr36.xy);
			nv_waveworks_attr53 = SampleTex2D(nv_waveworks_attr29, nv_waveworks_attr28, In.nv_waveworks_attr36.zw);
			nv_waveworks_attr54 = SampleTex2D(nv_waveworks_attr31, nv_waveworks_attr30, In.nv_waveworks_attr37.xy);
			nv_waveworks_attr55 = SampleTex2D(nv_waveworks_attr33, nv_waveworks_attr32, In.nv_waveworks_attr37.zw);
		}
	#else
		nv_waveworks_attr52 = SampleTex2D(nv_waveworks_attr27, nv_waveworks_attr26, In.nv_waveworks_attr36.xy);
		nv_waveworks_attr53 = SampleTex2D(nv_waveworks_attr29, nv_waveworks_attr28, In.nv_waveworks_attr36.zw);
		nv_waveworks_attr54 = SampleTex2D(nv_waveworks_attr31, nv_waveworks_attr30, In.nv_waveworks_attr37.xy);
		nv_waveworks_attr55 = SampleTex2D(nv_waveworks_attr33, nv_waveworks_attr32, In.nv_waveworks_attr37.zw);
	#endif
	float2 nv_waveworks_attr56;
	nv_waveworks_attr56.xy = nv_waveworks_attr52.xy*In.nv_waveworks_attr38.x + 
				   nv_waveworks_attr53.xy*In.nv_waveworks_attr38.y*nv_waveworks_attr18 + 
				   nv_waveworks_attr54.xy*In.nv_waveworks_attr38.z*nv_waveworks_attr21 + 
				   nv_waveworks_attr55.xy*In.nv_waveworks_attr38.w*nv_waveworks_attr24;
	float nv_waveworks_attr57 = 0.25; 
	float nv_waveworks_attr58 = 
					  100.0*nv_waveworks_attr52.w *  
					  lerp(nv_waveworks_attr57, nv_waveworks_attr53.w, In.nv_waveworks_attr38.y)*
					  lerp(nv_waveworks_attr57, nv_waveworks_attr54.w, In.nv_waveworks_attr38.z)*
					  lerp(nv_waveworks_attr57, nv_waveworks_attr55.w, In.nv_waveworks_attr38.w);
	float nv_waveworks_attr59 = 
    				   max(-100,
					  (1.0-nv_waveworks_attr52.z) +
					  (1.0-nv_waveworks_attr53.z) +
					  (1.0-nv_waveworks_attr54.z) +
					  (1.0-nv_waveworks_attr55.z));
	float3 nv_waveworks_attr60 = normalize(float3(nv_waveworks_attr56, nv_waveworks_attr16));
	float nv_waveworks_attr61 = 0.5;		
	float nv_waveworks_attr62 =  
      				   10.0*(-0.55 + 
					  (1.0-nv_waveworks_attr52.z) +
					  nv_waveworks_attr61*(1.0-nv_waveworks_attr53.z) +
					  nv_waveworks_attr61*nv_waveworks_attr61*(1.0-nv_waveworks_attr54.z) +
					  nv_waveworks_attr61*nv_waveworks_attr61*nv_waveworks_attr61*(1.0-nv_waveworks_attr55.z));
	GFSDK_WAVEWORKS_SURFACE_ATTRIBUTES Output;
	Output.normal = nv_waveworks_attr60;
	Output.eye_dir = nv_waveworks_attr51;
	Output.foam_surface_folding = nv_waveworks_attr59;
	Output.foam_turbulent_energy = log(1.0 + nv_waveworks_attr58);
	Output.foam_wave_hats = nv_waveworks_attr62;
	return Output;
}
#endif /* _GFSDK_WAVEWORKS_ATTRIBUTES_FX */
