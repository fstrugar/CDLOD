//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Common.h"

#include "DxEventNotifier.h"

#include "CDLODQuadTree.h"

#include "DxShader.h"

#include "CascadedVolumeMap.h"

class DemoCamera;
class DemoSky;
class CDLODRenderer;
class DemoRenderer;
struct CDLODRenderStats;

class CascadedShadowMap : private CascadedVolumeMap, protected DxEventReceiver
{
public:


   struct CSMLayer : public CascadedVolumeMap::Layer
   {
      
      IDirect3DTexture9 *     ShadowMap;
      IDirect3DTexture9 *     ShadowMapDepth;

      D3DXMATRIX              ShadowView;
      D3DXMATRIX              ShadowProj;
      D3DXMATRIX              ShadowViewProj;

      bool                    TaggedForRefresh;
      double                  TaggedForRefreshTime;

      float                   ApproxWorldTexelSize;

      float                   OrthoProjWidth;
      float                   OrthoProjHeight;
      float                   OrthoProjDepth;
      D3DXVECTOR3             OrthoEyePos;

      float                   WorldSpaceTexelSizeX;
      float                   WorldSpaceTexelSizeY;

      float                   NoiseScaleX;
      float                   NoiseScaleY;

      float                   SamplingTexelRadius;

      /*
      float                   LessDetailedLayerScaleX;
      float                   LessDetailedLayerScaleY;

      float                   NoiseDepthOffsetScale;
      */

      CSMLayer()              { ShadowMap = NULL; ShadowMapDepth = NULL; TaggedForRefresh = false; }
      ~CSMLayer()             { SAFE_RELEASE( ShadowMap ); SAFE_RELEASE( ShadowMapDepth ); }
      
      virtual bool		      Update( const D3DXVECTOR3 & observerPos, float newVisibilityRange, CascadedVolumeMap * parent, bool forceUpdate = false );

      bool                    RenderShadowMap( void * renderContext, CSMLayer * parentLayer );

   };

private:

   
   //IDirect3DSurface9 *        m_depthBuffer;
   IDirect3DSurface9 *        m_scratchSurface;

   const DemoRenderer *       m_mainRenderer;

   DxVertexShader             m_vsShadow;
   DxPixelShader              m_psShadow;

   bool                       m_defPoolTexturesCreated;

   //int                        m_DLODLevelsPerCascade;

   CSMLayer                   m_cascades[CDLODQuadTree::c_maxLODLevels];

   D3DXVECTOR3                m_lastShadowCamRightVec;
   D3DXVECTOR3                m_shadowForward;
   D3DXVECTOR3                m_shadowRight;
   D3DXVECTOR3                m_shadowUp;

   int                        m_textureResolution;

public:
   CascadedShadowMap(void);
   virtual ~CascadedShadowMap(void);
   //
public:
   void                       Initialize(DemoRenderer * mainRenderer);
   void                       Deinitialize();
   //
   HRESULT                    Render( float deltaTime, DemoCamera * camera, DemoSky * lightMgr, int gridMeshDim, bool useDetailHeightmap, const CDLODQuadTree::LODSelection & terrainDLODSelection, CDLODRenderStats * renderStats = NULL );
   //
   void                       UpdateShaderSettings( D3DXMACRO * newMacroDefines );
   //
   const CSMLayer &           GetCascadeLayer( int index ) const                 { assert( index >= 0 && index < m_layerCount ); return m_cascades[index]; }
   const CSMLayer &           GetCascadeLayerForDLODLevel( int dlodLevel ) const { return GetCascadeLayer( dlodLevel ); }
   int                        GetCascadeLayerCount( ) const                      { return m_layerCount; }
   //
   int                        GetTextureResolution() const                       { return m_textureResolution; }
   //
protected:
   void                       InitializeRuntimeData( );
   //
   bool                       RenderCascade( int cascadeIndex, DemoCamera * camera, int gridMeshDim, bool useDetailHeightmap, const CDLODQuadTree::LODSelection & terrainDLODSelection, CDLODRenderStats * renderStats );
   //
   virtual HRESULT            OnCreateDevice();
   virtual HRESULT            OnResetDevice( const D3DSURFACE_DESC * pBackBufferSurfaceDesc );
   //
   virtual void               OnLostDevice();
   virtual void               OnDestroyDevice();
   //
};
