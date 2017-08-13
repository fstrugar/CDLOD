//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#include "common.sh"

float4            g_quadOffset;           // .z holds the z center of the AABB
float4            g_quadScale;            // .z holds the current LOD level
float2            g_quadWorldMax;         // .xy max used to clamp triangles outside of world range

float4            g_terrainScale;
float4            g_terrainOffset;

float4            g_morphConsts;

float4x4          g_viewProjection;

#if USE_SHADOWMAP
float4x4          g_shadowViewProjection;
float4x4          g_shadowView;
float4x4          g_shadowProj;
float4            g_shadowNoiseConsts;    // .xy - noise UV scale, .z - noise depth scale

float4x4          g_shadowL2View;
float4x4          g_shadowL2Proj;
#endif

float4            g_cameraPos;

float4            g_diffuseLightDir;

float2            g_samplerWorldToTextureScale;
float4            g_heightmapTextureInfo;

sampler           g_terrainHMVertexTexture;
//sampler           g_terrainNMVertexTexture;


float3            g_gridDim;                        // .x = gridDim, .y = gridDimHalf, .z = oneOverGridDimHalf


sampler           g_detailHMVertexTexture;
float4            g_detailConsts;                   // .w = detailMeshLODLevelsAffected
uniform bool      g_useDetailMap;

//float4            g_shadowConsts                   : register(c20); // for f's sake... hlsl bug?


// returns baseVertexPos where: xy are true values, z is g_quadOffset.z which is z center of the AABB
float4 getBaseVertexPos( float4 inPos )
{
   float4 ret = inPos * g_quadScale + g_quadOffset;
   ret.xy = min( ret.xy, g_quadWorldMax );
   return ret;
}

// morphs vertex xy from from high to low detailed mesh position
float2 morphVertex( float4 inPos, float2 vertex, float morphLerpK )
{
   float2 fracPart = (frac( inPos.xy * float2(g_gridDim.y, g_gridDim.y) ) * float2(g_gridDim.z, g_gridDim.z) ) * g_quadScale.xy;
   return vertex.xy - fracPart * morphLerpK;
}

// calc big texture tex coords
float2 calcGlobalUV( float2 vertex )
{
   float2 globalUV = (vertex.xy - g_terrainOffset.xy) / g_terrainScale.xy;  // this can be combined into one inPos * a + b
   globalUV *= g_samplerWorldToTextureScale;
   globalUV += g_heightmapTextureInfo.zw * 0.5;
   return globalUV;
}

float sampleHeightmap( uniform sampler heightmapSampler, float2 uv, float mipLevel, bool useFilter )
{
#if BILINEAR_VERTEX_FILTER_SUPPORTED
   
   return tex2Dlod( heightmapSampler, float4( uv.x, uv.y, 0, mipLevel ) ).x;

#else

const float2 textureSize = g_heightmapTextureInfo.xy;
const float2 texelSize   = g_heightmapTextureInfo.zw; 

   uv = uv.xy * textureSize - float2(0.5, 0.5);
   float2 uvf = floor( uv.xy );
   float2 f = uv - uvf;
   uv = (uvf + float2(0.5, 0.5)) * texelSize;

   float t00 = tex2Dlod( heightmapSampler, float4( uv.x, uv.y, 0, mipLevel ) ).x;
   float t10 = tex2Dlod( heightmapSampler, float4( uv.x + texelSize.x, uv.y, 0, mipLevel ) ).x;

   float tA = lerp( t00, t10, f.x );

   float t01 = tex2Dlod( heightmapSampler, float4( uv.x, uv.y + texelSize.y, 0, mipLevel ) ).x;
   float t11 = tex2Dlod( heightmapSampler, float4( uv.x + texelSize.x, uv.y + texelSize.y, 0, mipLevel ) ).x;

   float tB = lerp( t01, t11, f.x );

   return lerp( tA, tB, f.y );

#endif
}


void ProcessCDLODVertex( float4 inPos /*: POSITION*/, out float4 outUnmorphedWorldPos, out float4 outWorldPos, out float2 outGlobalUV, out float2 outDetailUV, out float2 outMorphK, out float outEyeDist )
{
   float4 vertex     = getBaseVertexPos( inPos );

   const float LODLevel = g_quadScale.z;

   // could use mipmaps for performance reasons but that will need some additional shader logic for morphing between levels, code 
   // changes and making sure that hardware supports it, so I'll leave that for some other time
   // pseudocode would be something like:
   //  - first approx-sampling:      mipLevel = LODLevel - log2(RenderGridResolutionMult) (+1?);
   //  - morphed precise sampling:   mipLevel = LODLevel - log2(RenderGridResolutionMult) + morphLerpK;
   float mipLevel = 0;  

   ////////////////////////////////////////////////////////////////////////////   
   // pre-sample height to be able to precisely calculate morphing value
   //  - we could probably live without this step if quad tree granularity is high (LeafQuadTreeNodeSize
   //    is small, like in the range of 4-16) by calculating the approximate height from 4 quad edge heights
   //    provided in shader consts; tweaking morph values; and possibly fixing other problems. But since that's
   //    only an optimisation, and this version is more robust, we'll leave it in for now.
   float2 preGlobalUV = calcGlobalUV( vertex.xy );
   vertex.z = sampleHeightmap( g_terrainHMVertexTexture, preGlobalUV.xy, mipLevel, false ) * g_terrainScale.z + g_terrainOffset.z;
   //vertex.z = tex2Dlod( g_terrainHMVertexTexture, float4( preGlobalUV.x, preGlobalUV.y, 0, mipLevel ) ).x * g_terrainScale.z + g_terrainOffset.z;

   outUnmorphedWorldPos = vertex;
   outUnmorphedWorldPos.w = 1;
   
   float eyeDist     = distance( vertex, g_cameraPos );
   float morphLerpK  = 1.0f - clamp( g_morphConsts.z - eyeDist * g_morphConsts.w, 0.0, 1.0 );   
   
   vertex.xy         = morphVertex( inPos, vertex.xy, morphLerpK );
 
   float2 globalUV   = calcGlobalUV( vertex.xy );

   float4 vertexSamplerUV = float4( globalUV.x, globalUV.y, 0, mipLevel );

   ////////////////////////////////////////////////////////////////////////////   
   // sample height and calculate it
   vertex.z = sampleHeightmap( g_terrainHMVertexTexture, vertexSamplerUV, mipLevel, morphLerpK > 0 ) * g_terrainScale.z + g_terrainOffset.z;
   //vertex.z = tex2Dlod( g_terrainHMVertexTexture, vertexSamplerUV ).x * g_terrainScale.z + g_terrainOffset.z;
   vertex.w = 1.0;   // this could also be set simply by having g_quadOffset.w = 1 and g_quadScale.w = 0....   
   
   float detailMorphLerpK = 0.0;
#if USE_DETAIL_HEIGHTMAP
   float2 detailUV   = float2( vertex.x / g_detailConsts.x, vertex.y / g_detailConsts.y );
   if( g_useDetailMap )
   {
      const float detailLODLevelsAffected = g_detailConsts.w;
      detailMorphLerpK = 1 - saturate( LODLevel + 2.0 - detailLODLevelsAffected ) * morphLerpK;

      //vertex.xyz += normal * ((tex2Dlod( g_detailHMVertexTexture, float4( detailUV.x, detailUV.y, 0, 0 ) ) - 0.5) * g_detailConsts.z);
      vertex.z += detailMorphLerpK * (sampleHeightmap( g_detailHMVertexTexture, detailUV.xy, 0, true ) - 0.5) * g_detailConsts.z;
      //vertex.z += detailMorphLerpK * (tex2Dlod( g_detailHMVertexTexture, float4( detailUV.x, detailUV.y, 0, 0 ) ) - 0.5) * g_detailConsts.z;
   }
#else
   float2 detailUV   = float2( 0, 0 );
#endif

   outWorldPos       = vertex;
   outGlobalUV       = globalUV;
   outDetailUV       = detailUV;
   outMorphK         = float2( morphLerpK, detailMorphLerpK );
   outEyeDist        = eyeDist;
}

TerrainVertexOutput terrainSimple( float4 inPos : POSITION )
{
   TerrainVertexOutput output;
   
   float4 unmorphedWorldPos;
   float4 worldPos;
   float2 globalUV;
   float2 detailUV;
   float2 morphK;
   float eyeDist;
   ProcessCDLODVertex( inPos, unmorphedWorldPos, worldPos, globalUV, detailUV, morphK, eyeDist );

   ////////////////////////////////////////////////////////////////////////////   
   float3 eyeDir     = normalize( (float3)g_cameraPos - (float3)worldPos );
   float3 lightDir   = g_diffuseLightDir;
   //

   //output.worldPos         = float4( worldPos, distance( worldPos, g_cameraPos.xyz ) );
   output.position         = mul( worldPos, g_viewProjection );
   output.globalUV_detUV   = float4( globalUV.x, globalUV.y, detailUV.x, detailUV.y );
   output.eyeDir           = float4( eyeDir, eyeDist );
   output.lightDir         = float4( lightDir, 0.0 );
   output.morphInfo        = float4( morphK.xy, 0.0, 0.0 );

#if USE_SHADOWMAP
   
   ////////////////////////////////////////////////////////////////////////////
   // calc coords for sampling noise map
   //TODO: have quadXY-based noiseUV generation to avoid fp inaccuracies on bigger terrains
   output.shadowNoise.xy = globalUV * g_shadowNoiseConsts.xy;
   output.shadowNoise.zw = float2( 0.0, 0.0 );
   ////////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////
   // calc shadow map stuff
   float4 shadowPosition   = mul( worldPos, g_shadowView );
   output.shadowPosition   = mul( shadowPosition, g_shadowProj );
   output.shadowPosition.xy = 0.5 * output.shadowPosition.xy + float2( 0.5, 0.5 ) * output.shadowPosition.w;
   output.shadowPosition.y = output.shadowPosition.w * 1.0f - output.shadowPosition.y;
   //
   //shadowPosition.z -= g_shadowNoiseConsts.z * 0.5;
   shadowPosition = mul( shadowPosition, g_shadowProj );
   output.shadowNoise.zw = shadowPosition.zw;
   ////////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////
   // calc L2 shadow map stuff
   shadowPosition   = mul( worldPos, g_shadowL2View );
   output.shadowL2Position = mul( shadowPosition, g_shadowL2Proj );
   output.shadowL2Position.xy = 0.5 * output.shadowL2Position.xy + float2( 0.5, 0.5 ) * output.shadowL2Position.w;
   output.shadowL2Position.y = output.shadowL2Position.w * 1.0f - output.shadowL2Position.y;
   ////////////////////////////////////////////////////////////////////////////

#endif

   return output;    
}

ShadowVertexOutput terrainShadow( float4 inPos : POSITION )
{
   ShadowVertexOutput output;
#if USE_SHADOWMAP
   
   float4 unmorphedWorldPos;
   float4 worldPos;
   float2 globalUV;
   float2 detailUV;
   float2 morphK;
   float eyeDist;
   ProcessCDLODVertex( inPos, unmorphedWorldPos, worldPos, globalUV, detailUV, morphK, eyeDist );

   float3 eyeDir     = normalize( (float3)g_cameraPos - (float3)worldPos );

   float4 vertex = mul( worldPos, g_shadowViewProjection );

   output.depth.xy            = vertex.zw;
   output.position            = vertex;

#else

   output.depth.xy            = float2( 0.0, 0.0 );
   output.position            = float4( 0.0, 0.0, 0.0, 0.0 );

#endif
   return output;   
}
