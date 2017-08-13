//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Common.h"

#include "DxEventNotifier.h"

#include "TiledBitmap/TiledBitmap.h"

#include "CDLODQuadTree.h"
#include "CDLODRenderer.h"

#include "DxShader.h"

class DemoCamera;
class DemoSky;
class CDLODStreamingStorage;

class DemoRenderer : protected DxEventReceiver
{

public:

   enum TexturingMode
   {
      TM_Simple,
      TM_Full,

      TM_MaxValue
   };

   struct Settings
   {
      // [Rendering]
      float                      MinViewRange;
      float                      MaxViewRange;

      // [DetailHeightmap]
      float                      DetailHMSizeX;
      float                      DetailHMSizeY;
      float                      DetailHMSizeZ;
      bool                       DetailHeightmapEnabled;
      int                        DetailMeshLODLevelsAffected;

      // [Splatting]
      bool                       SplattingEnabled;
      float                      SplatMatTexTileSizeX;
      float                      SplatMatTexTileSizeY;
   };

   struct LastFrameRenderStats
   {
      CDLODRenderStats           TerrainStats;
      CDLODRenderStats           TerrainShadowStats;
      int                        LODLevelCount;

      void                       Reset()  { memset(this, 0, sizeof(*this)); }
   };
   
private:

   struct SplattingData
   {
      static const int        SplatCount = 5;

      IDirect3DTexture9 *     ColorTextures[SplatCount];
      IDirect3DTexture9 *     NormalmapTextures[SplatCount];
      bool                    Initialized;

      float                   AlphaAdd[SplatCount];
      float                   AlphaMul[SplatCount];
      float                   SpecularPow[SplatCount];
      float                   SpecularMul[SplatCount];
   };

   Settings                   m_settings;

   LastFrameRenderStats       m_renderStats;

   bool                       m_wireframe;
   bool                       m_useTerrainFlatShading;

   int                        m_terrainGridMeshDims;

   double                     m_globalTime;

   CDLODRenderer              m_dlodRenderer;

   bool                       m_mouseLeftButtonDown;
   int                        m_mouseClientX;
   int                        m_mouseClientY;

   bool                       m_terrainCursorVisible;
   D3DXVECTOR3                m_terrainCursorPos;

   std::string                m_detailHeightmapPath;

   IDirect3DTexture9 *        m_detailHMTexture;
   IDirect3DTexture9 *        m_detailNMTexture;

   DxVertexShader             m_vsTerrainSimple;
   DxPixelShader              m_psTerrainForwardBasic;
   DxPixelShader              m_psTerrainForwardWireframe;
   DxPixelShader              m_psTerrainDeferredStandard;

   DxPixelShader              m_psDeferredApplyDirectionalLight;
   DxPixelShader              m_psDeferredApplyDbgAlbedo;
   DxPixelShader              m_psDeferredApplyDbgNormal;
   DxPixelShader              m_psDeferredApplyDbgDepth;

   int                        m_noiseTextureResolution;
   IDirect3DTexture9 *        m_noiseTexture;

   IDirect3DTexture9 *        m_white1x1Texture;

   bool                       m_debugView;

   int                        m_maxRenderGridResolutionMult;
   int                        m_currentRenderGridResolutionMult;

   int                        m_showDeferredBufferType;

   CDLODStreamingStorage *    m_storage;

   IDirect3DTexture9 *        m_gbufferAlbedo;
   IDirect3DTexture9 *        m_gbufferNormal;
   IDirect3DTexture9 *        m_gbufferDepth;

   TexturingMode              m_texturingMode;

   SplattingData              m_splattingData;

   bool                       m_lightingSupported;
   bool                       m_lightingEnabled;

public:
   DemoRenderer(void);
   virtual ~DemoRenderer(void);
   //
public:
   void                          Start( CDLODStreamingStorage * storage );
   void                          Stop( );
   //
   void                          InitializeFromIni( );
   //
   void                          Tick( DemoCamera * camera, float deltaTime );
   HRESULT                       Render( DemoCamera * camera, DemoSky * lightMgr, float deltaTime );
   //
public:
   const CDLODStreamingStorage *  GetStorage() const                        { return m_storage; }
   const CDLODRenderer &          GetDLODRenderer() const                   { return m_dlodRenderer; }
   const Settings &              GetSettings() const                       { return m_settings; }
   const IDirect3DTexture9 *     GetTerrainDetailHMTexture() const         { return m_detailHMTexture; }
   //
   const LastFrameRenderStats &  GetRenderStats() const                    { return m_renderStats; }
   //
   bool                          GetWireframe() const                      { return m_wireframe; }
   void                          SetWireframe( bool enabled )              { m_wireframe = enabled; }
   //
   bool                          GetDetailmapExists() const                { return m_detailHeightmapPath != ""; }
   void                          SetDetailmap( bool enabled )              { if( !GetDetailmapExists() ) return; m_settings.DetailHeightmapEnabled = enabled; UpdateShaderSettings(); }
   //
   bool                          GetDebugView() const                      { return m_debugView; }
   void                          SetDebugView( bool enabled )              { m_debugView = enabled; }
   //
   int                           GetMaxRenderGridResolutionMult() const    { return m_maxRenderGridResolutionMult; }
   void                          SetRenderGridResolutionMult( int mult );
   //
   int                           GetShowDeferredBufferType() const         { return m_showDeferredBufferType; }
   void                          SetShowDeferredBufferType( int type )     { m_showDeferredBufferType = type; }
   //
   TexturingMode                 GetTexturingMode() const                  { return m_texturingMode; }
   void                          SetTexturingMode( TexturingMode newMode ) { m_texturingMode = newMode; UpdateShaderSettings(); }
   //
   bool                          GetLightingSupported() const              { return m_lightingSupported; }
   bool                          GetLightingEnabled() const                { return m_lightingEnabled; }
   void                          SetLightingEnabled( bool enabled )        { m_lightingEnabled = enabled && m_lightingSupported; UpdateShaderSettings(); }
   //
protected:
   //
   void                          SetupSplatting( DxPixelShader * pixelShaderPtr );
   //
   void                          RenderTerrain( DemoCamera * camera, const CDLODQuadTree::LODSelection & cdlodSelection, 
                                                DxPixelShader * pixelShaderPtr, bool drawWireframe );
   void                          ApplyDeferred( DemoCamera * camera, DemoSky * lightMgr );
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
