//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#define PIXEL_SHADER
#include "common.sh"

float4      g_gridToSplatMaterialTextureConsts;

sampler     g_splatMatTex0;
sampler     g_splatMatTex1;
sampler     g_splatMatTex2;
sampler     g_splatMatTex3;
sampler     g_splatMatTex4;
sampler     g_splatMatNormTex0;
sampler     g_splatMatNormTex1;
sampler     g_splatMatNormTex2;
sampler     g_splatMatNormTex3;
sampler     g_splatMatNormTex4;

float       g_splatAlphaModAdd[4];
float       g_splatAlphaModMul[4];

float       g_splatSpecularPow[5];
float       g_splatSpecularMul[5];

void calcSplatMaterialUV( float2 gridPos, out float2 outSplatMaterialUV )
{
   outSplatMaterialUV   = gridPos.xy * g_gridToSplatMaterialTextureConsts.xy + g_gridToSplatMaterialTextureConsts.zw;
}

float ModAlpha( int modIndex, float inSplatMapAlpha, float inSplatTextureAlpha )
{
#ifdef DEBUG_DISABLE_SPLAT_ALPHA_MODIFIER
   return inSplatMapAlpha;
#else
   return saturate( (inSplatMapAlpha * inSplatTextureAlpha + g_splatAlphaModAdd[modIndex]) * g_splatAlphaModMul[modIndex] );
#endif
}

void ApplySplatting( float2 gridPos, float4 splatMap, float3 normal, out float3 outAlbedo, out float3 outNormal, 
                     out float outSpecPow, out float outSpecMul )
{
   float2 splatMatUV;
   calcSplatMaterialUV( gridPos, splatMatUV );

   float3 finalSplatColor;
   float3 finalSplatNormal;

   // Hack to add more detail to splatmap
   float4 alphaMod = sin( normal.xy * 20 ).xyxy * 0.2 + 1.0;
   alphaMod.w = 1.0;

   // Calculate splatting
   {
      float2 tileTexCoords = splatMatUV;
      //
   //#ifndef DEBUG_DISABLE_DETAIL_MAPS
   //   tileTexCoords.x += detailUVMod.x * c_detailMapTextureCoordModStrength;
   //   tileTexCoords.y += detailUVMod.y * c_detailMapTextureCoordModStrength;
   //#endif
      //
      float4 splat         = float4( 0, 0, 0, 0 );
      float4 splatNormalMap = float4( 0, 0, 0, 0 );
      //
      float4 splatColors[5];
      float4 splatNormalMaps[5]; // x and w will hold normal; .y and .z hold specularPow and specularMul
      //
      splatColors[0] = tex2D( g_splatMatTex0, tileTexCoords );
      splatColors[1] = tex2D( g_splatMatTex1, tileTexCoords );
      splatColors[2] = tex2D( g_splatMatTex2, tileTexCoords );
      splatColors[3] = tex2D( g_splatMatTex3, tileTexCoords );
      splatColors[4] = tex2D( g_splatMatTex4, tileTexCoords );
      //
      splatNormalMaps[0] = tex2D( g_splatMatNormTex0, tileTexCoords );
      splatNormalMaps[1] = tex2D( g_splatMatNormTex1, tileTexCoords );
      splatNormalMaps[2] = tex2D( g_splatMatNormTex2, tileTexCoords );
      splatNormalMaps[3] = tex2D( g_splatMatNormTex3, tileTexCoords );
      splatNormalMaps[4] = tex2D( g_splatMatNormTex4, tileTexCoords );

      // y and z are unused, so reuse them for easier/faster lerping later
      for( int i = 0; i < 5; i++ )
      {
         splatNormalMaps[i].y = g_splatSpecularPow[i];
         splatNormalMaps[i].z = g_splatSpecularMul[i];
      }

      float4 alphas = float4( splatColors[1].a, splatColors[2].a, splatColors[3].a, splatColors[4].a );
      alphas = saturate( alphas * alphaMod );

      splatMap.x = ModAlpha( 0, splatMap.x, alphas.x );
      splatMap.y = ModAlpha( 1, splatMap.y, alphas.y );
      splatMap.z = ModAlpha( 2, splatMap.z, alphas.z );
      splatMap.w = ModAlpha( 3, splatMap.w, alphas.w );
         
      float blend[5];      
      float4 oneMinusSplatMap = float4( 1, 1, 1, 1 ) - splatMap;
         
      blend[4] = splatMap.w;
         
      float acc = oneMinusSplatMap.w;
      blend[3] = splatMap.z * acc;
         
      acc *= oneMinusSplatMap.z;
      blend[2] = splatMap.y * acc;

      acc *= oneMinusSplatMap.y;
      blend[1] = splatMap.x * acc;

      acc *= oneMinusSplatMap.x;
      blend[0] = 1 * acc;
         
      //float assertOne = blend0 + blend1 + blend2 + blend3 + blend4;
     //blend[0] = 1.0f;

      for( int j = 0; j < 5; j++ )
      {
         splat          += blend[j] * splatColors[j];
         splatNormalMap += blend[j] * splatNormalMaps[j];
      }

      finalSplatColor = splat.xyz;
      finalSplatNormal = UncompressDXT5_NM( splatNormalMap );
      outSpecPow = splatNormalMap.y;
      outSpecMul = splatNormalMap.z;
   }


   // Apply normal map
   float3 tangentX       = cross( float3( 0, 1, 0 ), normal );
   float3 tangentY       = cross( normal, tangentX );

   float3x3 worldToTangentSpace;
   worldToTangentSpace[0] = tangentX;
   worldToTangentSpace[1] = tangentY;
   worldToTangentSpace[2] = normal;
   finalSplatNormal = mul( finalSplatNormal, worldToTangentSpace );

   // Return output
   outAlbedo = finalSplatColor;
   outNormal = finalSplatNormal;

   //outAlbedo.xyz = alphaMod.xyz;
}