//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "CDLODRenderer.h"

#include "CDLODQuadTree.h"

#include "DxCanvas.h"

#include "DxShader.h"

#include "CDLODStreaming/CDLODStreamingStorage.h"

//
CDLODRenderer::CDLODRenderer(void)
{

   for( int i = 0, dim = 2; i < sizeof(m_gridMeshes) / sizeof(m_gridMeshes[0]); i++, dim *= 2 )
   {
      m_gridMeshes[i].SetDimensions( dim );
   }

}
//
CDLODRenderer::~CDLODRenderer(void)
{

}
//
const DxGridMesh * CDLODRenderer::PickGridMesh( int dimensions ) const
{
   for( int i = 0; i < sizeof(m_gridMeshes) / sizeof(m_gridMeshes[0]); i++ )
   { 
      if( m_gridMeshes[i].GetDimensions( ) == dimensions ) 
         return &m_gridMeshes[i];
   }
   return NULL;
}
//
void CDLODRenderer::SetIndependentGlobalVertexShaderConsts( DxVertexShader & vertexShader, const CDLODQuadTree & cdlodQuadTree, const D3DXMATRIX & viewProj, const D3DXVECTOR3 & camPos ) const
{
   HRESULT hr;

   const MapDimensions & mapDims = cdlodQuadTree.GetWorldMapDims();

   // Used to clamp edges to correct terrain size (only max-es needs clamping, min-s are clamped implicitly)
   V( vertexShader.SetFloatArray( "g_quadWorldMax", mapDims.MaxX(), mapDims.MaxY() ) );

   vertexShader.SetVector( "g_cameraPos", D3DXVECTOR4( camPos, 1.0f ) );
   V( vertexShader.SetFloatArray( "g_terrainScale", mapDims.SizeX, mapDims.SizeY, mapDims.SizeZ, 0.0f ) );
   V( vertexShader.SetFloatArray( "g_terrainOffset", mapDims.MinX, mapDims.MinY, mapDims.MinZ, 0.0f ) );
}
//
static D3DXVECTOR4 CalcGridToTextureMappingConsts( const D3DXVECTOR3 & quadMin, const D3DXVECTOR3 & quadSize, 
                                                   const CDLODStreamingTypeTextureBase::TextureBlockRenderInfo & textureInfo )
{
   D3DXVECTOR4 gridToHMConsts;
   gridToHMConsts.x = quadSize.x / textureInfo.WorldSizeX;
   gridToHMConsts.y = quadSize.y / textureInfo.WorldSizeY;
   gridToHMConsts.z = (quadMin.x - textureInfo.WorldPosX) / textureInfo.WorldSizeX;
   gridToHMConsts.w = (quadMin.y - textureInfo.WorldPosY) / textureInfo.WorldSizeY;
   return gridToHMConsts;
}
//
HRESULT CDLODRenderer::Render( const CDLODRendererBatchInfo & batchInfo, CDLODRenderStats * renderStats ) const
{
   IDirect3DDevice9* device = GetD3DDevice();
   HRESULT hr;

   const DxGridMesh * gridMesh = PickGridMesh( batchInfo.MeshGridDimensions );
   assert( gridMesh != NULL );
   if( gridMesh == NULL )
      return E_FAIL;

   if( renderStats != NULL )
      renderStats->Reset();

   assert( batchInfo.StreamingStorage != NULL );

   //////////////////////////////////////////////////////////////////////////
   // Setup mesh
   V( device->SetStreamSource(0, (IDirect3DVertexBuffer9*)gridMesh->GetVertexBuffer(), 0, sizeof(PositionVertex) ) );
   V( device->SetIndices((IDirect3DIndexBuffer9*)gridMesh->GetIndexBuffer()) );
   V( device->SetFVF( PositionVertex::FVF ) );
   {
      batchInfo.VertexShader->SetFloatArray( batchInfo.VSGridDimHandle, (float)gridMesh->GetDimensions(), gridMesh->GetDimensions() * 0.5f, 2.0f / gridMesh->GetDimensions(), 0.0f );
   }
   //////////////////////////////////////////////////////////////////////////


   const CDLODQuadTree::SelectedNode * selectionArray = batchInfo.CDLODSelection->GetSelection();
   const int selectionCount = batchInfo.CDLODSelection->GetSelectionCount();

   int qtRasterX = batchInfo.CDLODSelection->GetQuadTree()->GetRasterSizeX();
   int qtRasterY = batchInfo.CDLODSelection->GetQuadTree()->GetRasterSizeY();
   MapDimensions mapDims = batchInfo.CDLODSelection->GetQuadTree()->GetWorldMapDims();

   int prevMorphConstLevelSet = -1;
   for( int i = 0; i < selectionCount; i++ )
   {
      const CDLODQuadTree::SelectedNode & nodeSel = selectionArray[i];

      // Filter out the node if required
      if( batchInfo.FilterLODLevel != -1 && batchInfo.FilterLODLevel != nodeSel.LODLevel )
         continue;

      // No data? Skip, but display not-loaded debug box
      if( nodeSel.StreamingDataID == -1 )
      {
         AABB boundingBox;
         nodeSel.GetAABB( boundingBox, qtRasterX, qtRasterY, mapDims );
         boundingBox.Expand( -0.003f );
         GetCanvas3D()->DrawBox( boundingBox.Min, boundingBox.Max, 0x4000FF00 );
         continue;
      }

      bool useDetailMap = batchInfo.DetailMeshLODLevelsAffected > nodeSel.LODLevel;

      // Set LOD level specific consts (if changed from previous node - shouldn't change too much as array should be sorted)
      if( prevMorphConstLevelSet != nodeSel.LODLevel )
      {
         prevMorphConstLevelSet = nodeSel.LODLevel;
         float v[4];
         batchInfo.CDLODSelection->GetMorphConsts( prevMorphConstLevelSet, v );
         batchInfo.VertexShader->SetFloatArray( batchInfo.VSMorphConstsHandle, v, 4 );

         if( batchInfo.VSUseDetailmapHandle != NULL )
         {
            V( batchInfo.VertexShader->SetBool( batchInfo.VSUseDetailmapHandle, useDetailMap ) );
         }
      }

      // Quad bounding box
      AABB boundingBox;
      nodeSel.GetAABB( boundingBox, qtRasterX, qtRasterY, mapDims );
      const D3DXVECTOR3 boundingBoxSize = boundingBox.Size();

      ///////////////////////////////////////////////////////////////////////////////////////////////////
      // Detailmap
      if( useDetailMap && (batchInfo.VSGridToDetailmapHandle != NULL) )
      {
         // Make fake CDLODStreamingTypeTextureBase::TextureBlockRenderInfo for our detailmap
         CDLODStreamingTypeTextureBase::TextureBlockRenderInfo dmInfo;
         dmInfo.Width = dmInfo.Height = 0; // irrelevant
         
         dmInfo.WorldSizeX = batchInfo.DetailmapSize.x;
         dmInfo.WorldSizeY = batchInfo.DetailmapSize.y;
         // Wrap positions for each quad to prevent wrapping imprecision creeping up on large numbers in shaders
         dmInfo.WorldPosX = floorf( boundingBox.Min.x / dmInfo.WorldSizeX ) * dmInfo.WorldSizeX;
         dmInfo.WorldPosY = floorf( boundingBox.Min.y / dmInfo.WorldSizeY ) * dmInfo.WorldSizeY;
         
         D3DXVECTOR4 gridToDetailmapConsts = CalcGridToTextureMappingConsts( boundingBox.Min, boundingBoxSize, dmInfo );
         V( batchInfo.VertexShader->SetFloatArray( batchInfo.VSGridToDetailmapHandle, (float*)gridToDetailmapConsts, 4 ) );
      }
      ///////////////////////////////////////////////////////////////////////////////////////////////////

      ///////////////////////////////////////////////////////////////////////////////////////////////////
      // Splatting material texture map
      if( batchInfo.PSGridToSplatMatTexConstsHandle != NULL)
      {
         // Make fake CDLODStreamingTypeTextureBase::TextureBlockRenderInfo for our detailmap
         CDLODStreamingTypeTextureBase::TextureBlockRenderInfo dmInfo;
         dmInfo.Width = dmInfo.Height = 0; // irrelevant

         dmInfo.WorldSizeX = batchInfo.SplatmapSize.x;
         dmInfo.WorldSizeY = batchInfo.SplatmapSize.y;
         // Wrap positions for each quad to prevent wrapping imprecision creeping up on large numbers in shaders
         dmInfo.WorldPosX = floorf( boundingBox.Min.x / dmInfo.WorldSizeX ) * dmInfo.WorldSizeX;
         dmInfo.WorldPosY = floorf( boundingBox.Min.y / dmInfo.WorldSizeY ) * dmInfo.WorldSizeY;

         D3DXVECTOR4 gridToSMTConsts = CalcGridToTextureMappingConsts( boundingBox.Min, boundingBoxSize, dmInfo );
         V( batchInfo.VertexShader->SetFloatArray( batchInfo.PSGridToSplatMatTexConstsHandle, (float*)gridToSMTConsts, 4 ) );
      }
      ///////////////////////////////////////////////////////////////////////////////////////////////////

      ///////////////////////////////////////////////////////////////////////////////////////////////////
      // Set streaming data
      {
         const CDLODStreamingStorage::SelectedNodeDataInfo * dataInfo = batchInfo.StreamingStorage->GetSelectedNodeDataInfo( nodeSel.StreamingDataID );

         ///////////////////////////////////////////////////////////////////////////////////////////////////
         // Heightmap
         {
            V( batchInfo.VertexShader->SetTexture( batchInfo.VSHeightmapTexHandle, dataInfo->Heightmap, 
                                                   D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR ) );

            D3DXVECTOR4 gridToHMConsts = CalcGridToTextureMappingConsts( boundingBox.Min, boundingBoxSize, dataInfo->HeightmapInfo );

            // tweak UVs so that heightmap texels correctly map to vertices, as the heightmap with X*Y values covers the
            // area of (X-1)*(Y-1).
            float heightmapTextureScaleX = (dataInfo->HeightmapInfo.Width-1.0f) / (float)dataInfo->HeightmapInfo.Width;
            float heightmapTextureScaleY = (dataInfo->HeightmapInfo.Height-1.0f) / (float)dataInfo->HeightmapInfo.Height;
            gridToHMConsts.x *= heightmapTextureScaleX;
            gridToHMConsts.y *= heightmapTextureScaleY;
            gridToHMConsts.z *= heightmapTextureScaleX;
            gridToHMConsts.w *= heightmapTextureScaleY;

            V( batchInfo.VertexShader->SetFloatArray( batchInfo.VSGridToHMConstsHandle, (float*)gridToHMConsts, 4 ) );
            V( batchInfo.VertexShader->SetFloatArray( batchInfo.VSHeightmapTexInfoHandle,
                                                      (float)dataInfo->HeightmapInfo.Width, (float)dataInfo->HeightmapInfo.Height, 
                                                      1.0f / (float)dataInfo->HeightmapInfo.Width, 1.0f / (float)dataInfo->HeightmapInfo.Height ) );

            batchInfo.VertexShader->SetFloatArray( batchInfo.VSNodeCornersHandle, 
                                                   dataInfo->HeightCornerTL / 65535.0f, dataInfo->HeightCornerTR / 65535.0f,
                                                   dataInfo->HeightCornerBL / 65535.0f, dataInfo->HeightCornerBR / 65535.0f );
         }
         ///////////////////////////////////////////////////////////////////////////////////////////////////

         ///////////////////////////////////////////////////////////////////////////////////////////////////
         // Normalmap
         if( batchInfo.PSNormalmapTexHandle != NULL )
         {
            V( batchInfo.PixelShader->SetTexture( batchInfo.PSNormalmapTexHandle, dataInfo->Normalmap, 
                                                   D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR ) );

            D3DXVECTOR4 gridToNMConsts = CalcGridToTextureMappingConsts( boundingBox.Min, boundingBoxSize, dataInfo->NormalmapInfo );
            V( batchInfo.PixelShader->SetFloatArray( batchInfo.PSGridToNMConstsHandle, (float*)gridToNMConsts, 4 ) );

            if( dataInfo->ParentNormalmap != NULL )
            {
               V( batchInfo.PixelShader->SetTexture( batchInfo.PSNormalmapParentTexHandle, dataInfo->ParentNormalmap, 
                  D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR ) );

               D3DXVECTOR4 gridToNMParentConsts = CalcGridToTextureMappingConsts( boundingBox.Min, boundingBoxSize, dataInfo->ParentNormalmapInfo );
               V( batchInfo.PixelShader->SetFloatArray( batchInfo.PSGridToNMParentConstsHandle, (float*)gridToNMParentConsts, 4 ) );
            }
            else
            {
               V( batchInfo.PixelShader->SetTexture( batchInfo.PSNormalmapParentTexHandle, dataInfo->Normalmap, 
                  D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR ) );
               V( batchInfo.PixelShader->SetFloatArray( batchInfo.PSGridToNMParentConstsHandle, (float*)gridToNMConsts, 4 ) );
            }
         }
         ///////////////////////////////////////////////////////////////////////////////////////////////////

         ///////////////////////////////////////////////////////////////////////////////////////////////////
         // Overlaymap
         if( batchInfo.PSOverlaymapTexHandle != NULL )
         {
            if( dataInfo->Overlaymap != NULL )
            {
               V( batchInfo.PixelShader->SetTexture( batchInfo.PSOverlaymapTexHandle, dataInfo->Overlaymap, 
                  D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR ) );

               D3DXVECTOR4 gridToOMConsts = CalcGridToTextureMappingConsts( boundingBox.Min, boundingBoxSize, dataInfo->OverlaymapInfo );
               V( batchInfo.PixelShader->SetFloatArray( batchInfo.PSGridToOMConstsHandle, (float*)gridToOMConsts, 4 ) );

               if( dataInfo->ParentOverlaymap != NULL )
               {
                  V( batchInfo.PixelShader->SetTexture( batchInfo.PSOverlaymapParentTexHandle, dataInfo->ParentOverlaymap, 
                     D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR ) );

                  D3DXVECTOR4 gridToOMParentConsts = CalcGridToTextureMappingConsts( boundingBox.Min, boundingBoxSize, dataInfo->ParentOverlaymapInfo );
                  V( batchInfo.PixelShader->SetFloatArray( batchInfo.PSGridToOMParentConstsHandle, (float*)gridToOMParentConsts, 4 ) );
               }
               else
               {
                  V( batchInfo.PixelShader->SetTexture( batchInfo.PSOverlaymapParentTexHandle, dataInfo->Overlaymap, 
                     D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR ) );
                  V( batchInfo.PixelShader->SetFloatArray( batchInfo.PSGridToOMParentConstsHandle, (float*)gridToOMConsts, 4 ) );
               }
            }
            else
            {
               // No overlay image texture? Use simple white texture instead!
               D3DXVECTOR4 gridToOMConsts( 0.0f, 0.0f, 0.0f, 0.0f );
               V( batchInfo.PixelShader->SetTexture( batchInfo.PSOverlaymapTexHandle, batchInfo.White1x1Texture ) );
               V( batchInfo.PixelShader->SetFloatArray( batchInfo.PSGridToOMConstsHandle, (float*)gridToOMConsts, 4 ) );
               V( batchInfo.PixelShader->SetTexture( batchInfo.PSOverlaymapParentTexHandle, batchInfo.White1x1Texture ) );
               V( batchInfo.PixelShader->SetFloatArray( batchInfo.PSGridToOMParentConstsHandle, (float*)gridToOMConsts, 4 ) );
            }
         }
         ///////////////////////////////////////////////////////////////////////////////////////////////////
      }


      // debug render - should account for unselected parts below!
      //GetCanvas3D()->DrawBox( D3DXVECTOR3(worldX, worldY, m_mapDims.MinZ), D3DXVECTOR3(worldX+worldSizeX, worldY+worldSizeY, m_mapDims.MaxZ()), 0xFF0000FF, 0x00FF0080 );

      bool drawFull = nodeSel.TL && nodeSel.TR && nodeSel.BL && nodeSel.BR;

      V( batchInfo.VertexShader->SetFloatArray( batchInfo.VSQuadScaleHandle, (boundingBox.Max.x-boundingBox.Min.x), (boundingBox.Max.y-boundingBox.Min.y), (float)nodeSel.LODLevel, 0.0f ) );

      V( batchInfo.VertexShader->SetFloatArray( batchInfo.VSQuadOffsetHandle, boundingBox.Min.x, boundingBox.Min.y, (boundingBox.Min.z + boundingBox.Max.z) * 0.5f, 0.0f ) );

      int gridDim = gridMesh->GetDimensions();

      int renderedTriangles = 0;

      int totalVertices = (gridDim+1) * (gridDim+1);
      int totalIndices = gridDim * gridDim * 2 * 3;
      if( drawFull )
      {
         V( device->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, totalVertices, 0, totalIndices/3 ) );
         renderedTriangles += totalIndices/3;
      }
      else
      {
         int halfd = ((gridDim+1)/2) * ((gridDim+1)/2) * 2;

         // can be optimized by combining calls
         if( nodeSel.TL )
         {
            V( device->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, totalVertices, 0, halfd ) );
            renderedTriangles +=  halfd;
         }
         if( nodeSel.TR ) 
         {
            V( device->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, totalVertices, gridMesh->GetIndexEndTL(), halfd ) );
            renderedTriangles +=  halfd;
         }
         if( nodeSel.BL ) 
         {
            V( device->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, totalVertices, gridMesh->GetIndexEndTR(), halfd ) );
            renderedTriangles +=  halfd;
         }
         if( nodeSel.BR ) 
         {
            V( device->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, totalVertices, gridMesh->GetIndexEndBL(), halfd ) );
            renderedTriangles +=  halfd;
         }
      }

      if( renderStats != NULL )
      {
         renderStats->RenderedQuads[nodeSel.LODLevel]++;
         renderStats->TotalRenderedQuads++;
         renderStats->TotalRenderedTriangles += renderedTriangles;
      }

   }

   return S_OK;
}
