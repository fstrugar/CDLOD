//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#ifndef COMMON_SH_INCLUDED
#define COMMON_SH_INCLUDED

struct FixedVertexOutput
{
   float4   position          : POSITION;   // vertex position 
   float2   tex0              : TEXCOORD0;
};

struct TerrainVertexOutput
{
   float4   position          : POSITION;
   float4   vertexConsts      : TEXCOORD0;      // .x = morphK, .y = detailmap morphout K, .z = viewspace .z (post-proj .w), .w = distance to camera
   float4   meshGridPos_detUV : TEXCOORD1;      // .xy = meshGridPos (0-1), .zw = detailTexUV
   float4   worldPos          : TEXCOORD2;
};

float4 tex2Dlod_bilinear( const sampler tex, float4 t, const float2 textureSize, const float2 texelSize )
{
	float4 c00 = tex2Dlod( tex, t );
	float4 c10 = tex2Dlod( tex, t + float4( texelSize.x, 0.0, 0.0, 0.0 ));
	float4 c01 = tex2Dlod( tex, t + float4( 0.0, texelSize.y, 0.0, 0.0 ) );
	float4 c11 = tex2Dlod( tex, t + float4( texelSize.x, texelSize.y, 0.0, 0.0 ) );

	float2 f = frac( t.xy * textureSize );
	float4 c0 = lerp( c00, c10, f.x );
	float4 c1 = lerp( c01, c11, f.x );
	return lerp( c0, c1, f.y );
}

#ifdef PIXEL_SHADER

float4                  g_lightColorDiffuse;    // actually diffuse and specular, but nevermind...
float4                  g_lightColorAmbient;

float3 UncompressDXT5_NM(float4 normPacked)
{
   float3 norm = float3( normPacked.w * 2.0 - 1.0, normPacked.x * 2.0 - 1.0, 0 );
   norm.z = sqrt( 1 - norm.x * norm.x - norm.y * norm.y );
   return norm;
}

#endif

#endif