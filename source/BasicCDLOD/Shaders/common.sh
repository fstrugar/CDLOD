//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

struct FixedVertexOutput
{
   float4   position          : POSITION;   // vertex position 
   float2   tex0              : TEXCOORD0;
};

struct TerrainVertexOutput
{
   float4   position       : POSITION;
   float4   globalUV_detUV : TEXCOORD0;      // .xy = globalUV, .zw = detailTexUV
   float4   lightDir       : TEXCOORD1;      // .xyz
   float4   eyeDir         : TEXCOORD2;		// .xyz = eyeDir, .w = eyeDist
   float4   morphInfo      : TEXCOORD3;      // .x = morphK, .y = specMorphK
#if USE_SHADOWMAP
   float4   shadowPosition : TEXCOORD4;      // pos in shadow proj space
   float4   shadowNoise    : TEXCOORD5;      // .xy - noise UVs, .zw - offsetted depth & w
   float4   shadowL2Position : TEXCOORD6;    // pos in shadow L2 proj space
#endif
};

struct ShadowVertexOutput
{
   float4   position       : POSITION;
   float2   depth          : TEXCOORD0;
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
float4                  g_fogColor;

float3 UncompressDXT5_NM(float4 normPacked)
{
   float3 norm = float3( normPacked.w * 2.0 - 1.0, normPacked.x * 2.0 - 1.0, 0 );
   norm.z = sqrt( 1 - norm.x * norm.x - norm.y * norm.y );
   return norm;
}

float CalculateDiffuseStrength( float3 normal, float3 lightDir )
{
   return saturate( -dot( normal, lightDir ) );
}

float CalculateSpecularStrength( float3 normal, float3 lightDir, float3 eyeDir )
{
   float3 diff    = saturate( dot(normal, -lightDir) );
   float3 reflect = normalize( 2 * diff * normal + lightDir ); 
   
   return saturate( dot( reflect, eyeDir ) );
}

float CalculateDirectionalLight( float3 normal, float3 lightDir, float3 eyeDir, float specularPow, float specularMul )
{
   float3 light0 = normalize( lightDir );

   return CalculateDiffuseStrength( normal, light0 ) + specularMul * pow( CalculateSpecularStrength( normal, light0, eyeDir ), specularPow );
}

#endif

