//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Common.h"

#include "DxEventNotifier.h"

#include "TiledBitmap/TiledBitmap.h"

#include "CDLODQuadTree.h"
#include "CDLODRenderer.h"

#include "DxShader.h"

#include "DemoShadowsRenderer.h"

class DemoCamera;
class DemoSky;


class DemoRenderer : protected DxEventReceiver
{

public:
   struct Settings
   {
      // [DLOD]
      int                        LeafQuadTreeNodeSize;
      int                        RenderGridResolutionMult;
      int                        LODLevelCount;

      // [Rendering]
      float                      MinViewRange;
      float                      MaxViewRange;
      float                      LODLevelDistanceRatio;

      // [DetailHeightmap]
      int                        DetailHeightmapXYScale;
      float                      DetailHeightmapZSize;
      bool                       DetailHeightmapEnabled;
      int                        DetailMeshLODLevelsAffected;

      // [CascadedShadowMaps]
      bool                       ShadowmapEnabled;
      bool                       ShadowmapHighQuality;
      //int                        ShadowmapDLODLevelsPerCascade;
   };

   struct LastFrameRenderStats
   {
      CDLODRenderStats            TerrainStats;
      CDLODRenderStats            TerrainShadowStats;
      int                        LODLevelCount;

      void                       Reset()  { memset(this, 0, sizeof(*this)); }
   };
   
private:

   Settings                   m_settings;

   LastFrameRenderStats       m_renderStats;

   MapDimensions              m_mapDims;

   int                        m_rasterWidth;
   int                        m_rasterHeight;

   bool                       m_wireframe;
   bool                       m_useTerrainFlatShading;

   VertexAsylum::TiledBitmap *   m_heightmap;

   IDirect3DTexture9 *        m_terrainHMTexture;
   IDirect3DTexture9 *        m_terrainNMTexture;

   int                        m_terrainGridMeshDims;

   double                     m_globalTime;

   CDLODQuadTree               m_terrainQuadTree;
   CDLODRenderer               m_dlodRenderer;

   bool                       m_mouseLeftButtonDown;
   int                        m_mouseClientX;
   int                        m_mouseClientY;

   bool                       m_terrainCursorVisible;
   D3DXVECTOR3                m_terrainCursorPos;

   VertexAsylum::TiledBitmap *   m_detailHeightmap;

   IDirect3DTexture9 *        m_detailHMTexture;
   IDirect3DTexture9 *        m_detailNMTexture;

   DxVertexShader             m_vsTerrainSimple;
   DxPixelShader              m_psTerrainFlat;

   CascadedShadowMap        m_shadowsRenderer;

   int                        m_noiseTextureResolution;
   IDirect3DTexture9 *        m_noiseTexture;

   bool                       m_debugView;

   int                        m_maxRenderGridResolutionMult;

public:
   DemoRenderer(void);
   virtual ~DemoRenderer(void);
   //
public:
   void                          Start( VertexAsylum::TiledBitmap * heightmap, const MapDimensions & mapDims );
   void                          Stop( );
   //
   void                          InitializeFromIni( );
   //
   void                          Tick( DemoCamera * camera, float deltaTime );
   HRESULT                       Render( DemoCamera * camera, DemoSky * lightMgr, float deltaTime );
   //
public:
   const CDLODQuadTree &          GetTerrainQuadTree()  const               { return m_terrainQuadTree; }
   const CDLODRenderer &          GetDLODRenderer() const                   { return m_dlodRenderer; }
   const Settings &              GetSettings() const                       { return m_settings; }
   const IDirect3DTexture9 *     GetTerrainHMTexture() const               { return m_terrainHMTexture; }
   const IDirect3DTexture9 *     GetTerrainDetailHMTexture() const         { return m_detailHMTexture; }
   //
   const LastFrameRenderStats &  GetRenderStats() const                    { return m_renderStats; }
   //
   bool                          GetWireframe() const                      { return m_wireframe; }
   void                          SetWireframe( bool enabled )              { m_wireframe = enabled; }
   //
   bool                          GetDetailmapExists() const                { return m_detailHeightmap != NULL; }
   void                          SetDetailmap( bool enabled )              { if( !GetDetailmapExists() ) return; m_settings.DetailHeightmapEnabled = enabled; UpdateShaderSettings(); }
   //
   // 0 - disabled, 1 - low quality, 2 - high quality
   int                           GetShadowQuality() const                  { if( !m_settings.ShadowmapEnabled ) return 0; return (m_settings.ShadowmapHighQuality)?(2):(1); }
   void                          SetShadowQuality( int shadowQuality );
   //
   bool                          GetDebugView() const                      { return m_debugView; }
   void                          SetDebugView( bool enabled )              { m_debugView = enabled; }
   //
   int                           GetMaxRenderGridResolutionMult() const    { return m_maxRenderGridResolutionMult; }
   void                          SetRenderGridResolutionMult( int mult )   { m_settings.RenderGridResolutionMult = mult; m_terrainGridMeshDims = m_settings.LeafQuadTreeNodeSize * m_settings.RenderGridResolutionMult; }
   //
protected:
   //
   void                          RenderTerrain( DemoCamera * camera, DemoSky * lightMgr, const CDLODQuadTree::LODSelection & cdlodSelection );
   //
   virtual HRESULT               OnCreateDevice();
   virtual HRESULT               OnResetDevice( const D3DSURFACE_DESC * pBackBufferSurfaceDesc );
   //
   void                          SetIndependentGlobalVertexShaderConsts( DxVertexShader & vertexShader, const CDLODQuadTree & cdlodQuadTree, const D3DXMATRIX & viewProj, const D3DXVECTOR3 & camPos );
   //
   void                          InitializeRuntimeData();
   //
   virtual void                  OnLostDevice( );
   virtual void                  OnDestroyDevice( );
   //
   void                          UpdateShaderSettings();
   //
public:
   //
   void                          MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing );
   //
};
