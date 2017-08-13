//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#pragma once


#include "Common.h"

#include "DxEventNotifier.h"

#include "DxGridMesh.h"

#include "CDLODQuadTree.h"

class DemoCamera;
class DemoSky;
class DxVertexShader;
class DxPixelShader;

struct CDLODRenderStats
{
   int                                 RenderedQuads[CDLODQuadTree::c_maxLODLevels];
   int                                 TotalRenderedQuads;
   int                                 TotalRenderedTriangles;

   void                                Reset()     { memset(this, 0, sizeof(*this)); }
   
   void                                Add( const CDLODRenderStats & other )
                                       {
                                          for( int i = 0; i < CDLODQuadTree::c_maxLODLevels; i++ )
                                             RenderedQuads[i] += other.RenderedQuads[i];
                                          TotalRenderedQuads      += other.TotalRenderedQuads;
                                          TotalRenderedTriangles  += other.TotalRenderedTriangles;
                                       }
};

struct CDLODRendererBatchInfo
{
   MapDimensions                       MapDims;
   const CDLODQuadTree::LODSelection *  CDLODSelection;
   int                                 MeshGridDimensions;
   int                                 DetailMeshLODLevelsAffected;  // set to 0 if not using detail mesh!

   DxVertexShader *                    VertexShader;
   DxPixelShader *                     PixelShader;

   D3DXHANDLE                          VSGridDimHandle;
   D3DXHANDLE                          VSQuadScaleHandle;
   D3DXHANDLE                          VSQuadOffsetHandle;
   D3DXHANDLE                          VSMorphConstsHandle;

   D3DXHANDLE                          VSUseDetailMapHandle;

   //D3DXHANDLE                          

   int                                 FilterLODLevel;         // only render quad if it is of FilterLODLevel; if -1 then render all

   CDLODRendererBatchInfo() :  CDLODSelection( NULL ), MeshGridDimensions(0), VertexShader(NULL), PixelShader(NULL),
                              VSGridDimHandle( 0 ), 
                              FilterLODLevel( -1 )
                           {}
};

class CDLODRenderer : protected DxEventReceiver
{
private:
   DxGridMesh                 m_gridMeshes[7];

public:
   CDLODRenderer(void);
   virtual ~CDLODRenderer(void);
   //
public:
   //
   void                       SetIndependentGlobalVertexShaderConsts( DxVertexShader & vertexShader, const CDLODQuadTree & cdlodQuadTree, const D3DXMATRIX & viewProj, const D3DXVECTOR3 & camPos ) const;
   HRESULT                    Render( const CDLODRendererBatchInfo & batchInfo, CDLODRenderStats * renderStats = NULL ) const;
   //
protected:
   //
   const DxGridMesh *         PickGridMesh( int dimensions ) const;
   //
};

