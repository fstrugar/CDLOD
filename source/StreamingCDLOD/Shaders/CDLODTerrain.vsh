//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#include "common.sh"

float4            g_quadOffset;           // .z holds the z center of the AABB
float4            g_quadScale;            // .z holds the current LOD level
float2            g_quadWorldMax;         // .xy max used to clamp triangles outside of world range

float4            g_terrainScale;
float4            g_terrainOffset;

float4            g_morphConsts;

float4x4          g_viewProjection;

float4            g_cameraPos;

// Heightmap texture (one block in the streaming storage)
sampler           g_heightmapVertexTexture;
float4            g_heightmapTextureInfo;          // .xy = texture width & height; .zw = 1 / texture width & height
float4            g_gridToHMConsts;                // .xy = mul; .zw = add

float3            g_gridDim;                        // .x = gridDim, .y = gridDimHalf, .z = oneOverGridDimHalf

sampler           g_detailHMVertexTexture;
float4            g_gridToDMConsts;
float4            g_detailConsts;                   // .x = 0, .y = 0, .z = detailHMSizeZ, .w = detailMeshLODLevelsAffected
uniform bool      g_useDetailMap;

float4            g_nodeCorners;

// returns baseVertexPos where: xy are true values, z is g_quadOffset.z which is z center of the AABB
float3 getBaseVertexPos( float3 inPos )
{
   float3 ret = inPos * g_quadScale.xyz + g_quadOffset.xyz;
   ret.xy = min( ret.xy, g_quadWorldMax );
   return ret;
}

// morphs input vertex uv from high to low detailed mesh position
float2 morphVertex( float2 inPos, float morphLerpK )
{
   float2 fracPart = frac( inPos.xy * float2(g_gridDim.y, g_gridDim.y) ) * float2(g_gridDim.z, g_gridDim.z);
   return inPos - fracPart * morphLerpK;
}

// calc texture coords used to sample heightmap texture
float2 calcHeightmapUV( float2 vertex )
{
   return vertex.xy * g_gridToHMConsts.xy + g_gridToHMConsts.zw;
}

// calc texture coords used to sample detailmap texture
float2 calcDetailmapUV( float2 vertex )
{
   return vertex.xy * g_gridToDMConsts.xy + g_gridToDMConsts.zw;
}

float sampleHeightmap( uniform sampler heightmapSampler, float2 uv, float mipLevel, bool useFilter )
{
   uv += g_heightmapTextureInfo.zw * 0.5;

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

float sampleApproxCornermap( float2 uv )
{
   float t00 = g_nodeCorners.x;
   float t10 = g_nodeCorners.y;

   float tA = lerp( t00, t10, uv.x );

   float t01 = g_nodeCorners.z;
   float t11 = g_nodeCorners.w;

   float tB = lerp( t01, t11, uv.x );

   return lerp( tA, tB, uv.y );
}

void ProcessCDLODVertex( float3 inPos /*: POSITION*/, out float2 outMeshGridPos, out float3 outUnmorphedWorldPos, out float3 outWorldPos, out float2 outDetailUV, out float2 outMorphK )
{
   float3 vertex     = getBaseVertexPos( inPos );

   const float LODLevel = g_quadScale.z;

   // could use mipmaps for performance reasons but that will need some additional shader logic for morphing between levels, code 
   // changes and making sure that hardware supports it, so I'll leave that for some other time
   // pseudocode would be something like:
   //  - first approx-sampling:      mipLevel = LODLevel - log2(RenderGridResolutionMult) (+1?);
   //  - morphed precise sampling:   mipLevel = LODLevel - log2(RenderGridResolutionMult) + morphLerpK;
   float mipLevel = 0;  

   ////////////////////////////////////////////////////////////////////////////////////////////////
   // #define SAMPLE_HEIGHTMAP_FOR_MORPH_CALC
   // Use SAMPLE_HEIGHTMAP_FOR_MORPH_CALC to  pre-sample height to be able to precisely calculate
   // morphing value if morphing precision is of high importance.
   // Does not significantly affect performance if graphics hardware supports bilinear filter on vertex textures.
   // If not used, morph transition will not be perfectly 3D-distance based except on the quadtree node edges.
   // That means that the LOD transition will always be correct, but might not match perfectly if used in combination
   // with other LOD systems.
   #if defined( SAMPLE_HEIGHTMAP_FOR_MORPH_CALC )
   {
      float2 heightmapUV = calcHeightmapUV( inPos.xy );
      vertex.z = sampleHeightmap( g_heightmapVertexTexture, heightmapUV.xy, mipLevel, false ) * g_terrainScale.z + g_terrainOffset.z;
   }
   #else
   {
      vertex.z = sampleApproxCornermap( inPos.xy ) * g_terrainScale.z + g_terrainOffset.z;
   }
   #endif
   ////////////////////////////////////////////////////////////////////////////////////////////////

   outUnmorphedWorldPos = vertex;
   
   float eyeDist     = distance( vertex, g_cameraPos.xyz );
   float morphLerpK  = 1.0f - clamp( g_morphConsts.z - eyeDist * g_morphConsts.w, 0.0, 1.0 );

   const float2 inPosMorphed = morphVertex( inPos.xy, morphLerpK );
   vertex.xy = getBaseVertexPos( float3( inPosMorphed.xy, 0 ) ).xy;
 
   float2 heightmapUV = calcHeightmapUV( inPosMorphed.xy );

   float4 vertexSamplerUV = float4( heightmapUV.xy, 0, mipLevel );

   ////////////////////////////////////////////////////////////////////////////   
   // sample height and calculate it
   vertex.z = sampleHeightmap( g_heightmapVertexTexture, vertexSamplerUV.xy, mipLevel, morphLerpK > 0 ) * g_terrainScale.z + g_terrainOffset.z;
   
   float detailMorphLerpK = 0.0;
   float2 detailUV   = float2( 0.0, 0.0 );
#if USE_DETAIL_HEIGHTMAP
   if( g_useDetailMap )
   {
      detailUV = calcDetailmapUV( inPosMorphed.xy );
      const float detailLODLevelsAffected = g_detailConsts.w;
      detailMorphLerpK = 1 - saturate( LODLevel + 2.0 - detailLODLevelsAffected ) * morphLerpK;

      vertex.z += detailMorphLerpK * (sampleHeightmap( g_detailHMVertexTexture, detailUV.xy, 0, true ) - 0.5) * g_detailConsts.z;
   }
#endif

   outMeshGridPos    = inPosMorphed;
   outWorldPos       = vertex;
   outDetailUV       = detailUV;
   outMorphK         = float2( morphLerpK, detailMorphLerpK );
}

TerrainVertexOutput terrainSimple( float3 inPos : POSITION )
{
   TerrainVertexOutput output;
   
   float2 meshGridPos;
   float3 unmorphedWorldPos;
   float3 worldPos;
   float2 detailUV;
   float2 morphK;
   ProcessCDLODVertex( inPos, meshGridPos, unmorphedWorldPos, worldPos, detailUV, morphK );

   ////////////////////////////////////////////////////////////////////////////   
   float3 eyeDir     = (float3)g_cameraPos - (float3)worldPos;
   float distToCamera = length( eyeDir );
   eyeDir = normalize( eyeDir );
   //

   //output.worldPos         = float4( worldPos, distance( worldPos, g_cameraPos.xyz ) );
   output.position            = mul( float4( worldPos, 1 ), g_viewProjection );
   output.meshGridPos_detUV   = float4( meshGridPos.xy, detailUV.xy );
   output.vertexConsts        = float4( morphK.xy, output.position.w, distToCamera );
   output.worldPos            = float4( worldPos, 1.0 );
   
   return output;    
}
