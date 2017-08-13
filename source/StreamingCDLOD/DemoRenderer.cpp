//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "DemoRenderer.h"

#include "DemoCamera.h"

#include "DemoSky.h"

#include "DxCanvas.h"

#include "iniparser\src\IniParser.hpp"
#include "TiledBitmap/TiledBitmap.h"

#include "CDLODStreaming\CDLODStreamingStorage.h"

#include "SimpleProfiler.h"

using namespace std;

//
DemoRenderer::DemoRenderer(void)
{
   m_detailHMTexture = NULL;
   m_detailNMTexture = NULL;

   m_wireframe = false;

   memset( &m_settings, 0, sizeof(m_settings) );

   m_mouseLeftButtonDown = false;
   m_mouseClientX        = 0;
   m_mouseClientY        = 0;

   m_terrainCursorPos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

   m_detailHeightmapPath = "";

   m_terrainGridMeshDims = 0;

   m_noiseTexture             = NULL;
   m_noiseTextureResolution   = 512;

   m_white1x1Texture = NULL;

   if( GetD3DDevice() != NULL )
      OnCreateDevice();

   m_debugView = false;

   m_showDeferredBufferType = 0;

   m_gbufferAlbedo = NULL;
   m_gbufferNormal = NULL;
   m_gbufferDepth  = NULL;

   m_texturingMode = TM_Full;

   memset( &m_splattingData, 0, sizeof(m_splattingData) );
   m_splattingData.Initialized = false;
}

DemoRenderer::~DemoRenderer(void)
{
   OnLostDevice();
   OnDestroyDevice();
}

void DemoRenderer::Start( CDLODStreamingStorage * storage )
{
   m_storage      = storage;

   InitializeFromIni();

   m_globalTime = 0.0;

   m_mouseLeftButtonDown = false;
}

void DemoRenderer::InitializeFromIni( )
{
   const IniParser & iniParser = vaGetIniParser();

   // CDLOD

   m_currentRenderGridResolutionMult = m_storage->GetWorldDesc().RenderGridResolutionMult;

   const int leafQuadTreeNodeSize = m_storage->GetWorldDesc().LeafQuadTreeNodeSize;

   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // This stepsAllowed limit is only needed because there is no morphing for heightmap data between 
   // LOD levels for performance reasons, unlike it is done for other data (normal maps, overlay maps). 
   // If higher detailed mesh is needed than it's available by streaming settings (in order to save on
   // streamed data memory), additional morph between different data LOD levels must be added.
   int stepsAllowed = m_storage->GetHeightmapDataLODOffset();

   m_maxRenderGridResolutionMult = 1;
   while( (m_maxRenderGridResolutionMult * leafQuadTreeNodeSize <= 128) && (stepsAllowed >= 0) ) 
   {
      stepsAllowed--;
      m_maxRenderGridResolutionMult *= 2;
   }
   m_maxRenderGridResolutionMult /= 2;

   
   // Rendering

   m_settings.MinViewRange             = iniParser.getFloat("Rendering:MinViewRange", 0.0f);
   m_settings.MaxViewRange             = iniParser.getFloat("Rendering:MaxViewRange", 0.0f);

   if( (m_settings.MinViewRange < 1.0f) || (m_settings.MinViewRange > 10000000.0f) )
      vaFatalError( "MinViewRange setting is incorrect" );
   if( (m_settings.MaxViewRange < 1.0f) || (m_settings.MaxViewRange > 10000000.0f) || (m_settings.MinViewRange > m_settings.MaxViewRange) )
      vaFatalError( "MaxViewRange setting is incorrect" );

   m_terrainGridMeshDims = leafQuadTreeNodeSize * m_currentRenderGridResolutionMult;


   // DetailHeightmap

   m_settings.DetailHeightmapEnabled = iniParser.getBool("DetailHeightmap:Enabled", false);
   if( m_settings.DetailHeightmapEnabled )
   {
      m_detailHeightmapPath = vaFindResource( "media\\detailhmap.tbmp" );

      VertexAsylum::TiledBitmap * detailHeightmap = VertexAsylum::TiledBitmap::Open( m_detailHeightmapPath.c_str(), true );
      if( detailHeightmap == NULL )
         vaFatalError( "Cannot load detail heightmap from DetailHeightmap:Path" );
      VertexAsylum::TiledBitmap::Close( detailHeightmap );

      m_settings.DetailHMSizeX        = iniParser.getFloat("DetailHeightmap:SizeX", 0.0f);
      m_settings.DetailHMSizeY        = iniParser.getFloat("DetailHeightmap:SizeY", 0.0f);
      m_settings.DetailHMSizeZ        = iniParser.getFloat("DetailHeightmap:SizeZ", 0.0f);
      m_settings.DetailMeshLODLevelsAffected = iniParser.getInt("DetailHeightmap:MeshLODLevelsAffected", 0);

      if( m_settings.DetailHMSizeX <= 0.0f )
         vaFatalError( "DetailHeightmap:DetailHMSizeX setting is incorrect" );
      if( m_settings.DetailHMSizeY <= 0.0f )
         vaFatalError( "DetailHeightmap:DetailHMSizeY setting is incorrect" );
      if( m_settings.DetailHMSizeZ <= 0.0f )
         vaFatalError( "DetailHeightmap:DetailHMSizeZ setting is incorrect" );

      if( m_settings.DetailMeshLODLevelsAffected <= 0 )
         vaFatalError( "DetailHeightmap:MeshLODLevelsAffected setting is incorrect" );
   }

   m_settings.SplattingEnabled = iniParser.getBool("Splatting:Enabled", false);
   if( m_settings.SplattingEnabled )
   {
      m_settings.SplatMatTexTileSizeX  = iniParser.getFloat("Splatting:SizeX", 0.0f);
      m_settings.SplatMatTexTileSizeY  = iniParser.getFloat("Splatting:SizeY", 0.0f);
   }

   m_lightingSupported = m_storage->HasNormalmapData();
   m_lightingEnabled = m_lightingSupported;

   UpdateShaderSettings();
}

void DemoRenderer::Stop( )
{
   OnLostDevice();
   OnDestroyDevice();
   //m_heightmap    = NULL;

   m_mouseLeftButtonDown = false;
}

void DemoRenderer::InitializeRuntimeData()
{
   IDirect3DDevice9 * device = GetD3DDevice();

   HRESULT hr;

   if( m_detailHeightmapPath != "" && m_detailHMTexture == NULL )
   {
      VertexAsylum::TiledBitmap * detailHeightmap = VertexAsylum::TiledBitmap::Open( m_detailHeightmapPath.c_str(), true );
      if( detailHeightmap == NULL )
         vaFatalError( "Cannot load detail heightmap from DetailHeightmap:Path" );

      int dmWidth = detailHeightmap->Width();
      int dmHeight = detailHeightmap->Height();
      V( device->CreateTexture( dmWidth, dmHeight, 1, 0, D3DFMT_R32F, D3DPOOL_MANAGED, &m_detailHMTexture, NULL ) );

      V( device->CreateTexture( dmWidth, dmHeight, 0, 0, D3DFMT_G16R16, D3DPOOL_MANAGED, &m_detailNMTexture, NULL ) );

      ///////////////////////////////////////////////////////////////////////////////////////////////////
      // Load heightmap
      //
      D3DLOCKED_RECT heightmapLockedRect; 
      V( m_detailHMTexture->LockRect( 0, &heightmapLockedRect, NULL, 0 ) );

      float * heightmapData = (float *)heightmapLockedRect.pBits;

      for( int y = 0; y < dmHeight; y++ )
      {
         for( int x = 0; x < dmWidth; x++ )
         {
            unsigned short pixel;
            detailHeightmap->GetPixel( x, y, &pixel );
            heightmapData[x] = pixel / 65535.0f;
         }
         heightmapData += heightmapLockedRect.Pitch / sizeof(*heightmapData);
      }

      if( m_detailNMTexture != NULL )
      {
         D3DLOCKED_RECT normalmapLockedRect;
         V( m_detailNMTexture->LockRect( 0, &normalmapLockedRect, NULL, 0 ) );

         const float heightNMFudge = 0.4f;

         vaCreateNormalMap( dmWidth, dmHeight, m_settings.DetailHMSizeX, m_settings.DetailHMSizeY, m_settings.DetailHMSizeZ * heightNMFudge, 
                           (float *)heightmapLockedRect.pBits, heightmapLockedRect.Pitch, 
                           (unsigned int *)normalmapLockedRect.pBits, normalmapLockedRect.Pitch, true );
         m_detailNMTexture->UnlockRect( 0 );

         V( D3DXFilterTexture( m_detailNMTexture, NULL, 0, D3DX_FILTER_BOX ) );

         // grayout mip levels
         float grayoutK = 0.8f;
         float grayStrength = 1.0f;
         for( unsigned int i = 1; i < m_detailNMTexture->GetLevelCount(); i++ )
         {
            dmWidth /= 2;
            dmHeight /= 2;
            grayStrength *= grayoutK;

            D3DLOCKED_RECT normalmapLockedRect;
            V( m_detailNMTexture->LockRect( i, &normalmapLockedRect, NULL, 0 ) );

            unsigned int * normalmapData = (unsigned int *)normalmapLockedRect.pBits;

            for( int y = 0; y < dmHeight; y++ )
            {
               unsigned int * nmScanLine = &normalmapData[ y * (normalmapLockedRect.Pitch/sizeof(*normalmapData)) ];

               for( int x = 0; x < dmWidth; x++ )
               {
                  D3DXVECTOR3 norm( ((nmScanLine[x] >> 16) / 65535.0f - 0.5f) * 2.0f, ((nmScanLine[x] & 0xFFFF) / 65535.0f - 0.5f) * 2.0f, 0 );
                  norm.x *= grayStrength;
                  norm.y *= grayStrength;
                  norm.z = sqrtf( 1 - norm.x*norm.x - norm.y*norm.y );
                  D3DXVec3Normalize( &norm, &norm );

                  unsigned short a = (unsigned short)clamp( 65535.0f * ( norm.x * 0.5f + 0.5f ), 0.0f, 65535.0f );
                  unsigned short b = (unsigned short)clamp( 65535.0f * ( norm.y * 0.5f + 0.5f ), 0.0f, 65535.0f );
                  nmScanLine[x] = (a << 16) | b;
               }
            }

            m_detailNMTexture->UnlockRect( i );
         }
      }

      m_detailHMTexture->UnlockRect( 0 );

      VertexAsylum::TiledBitmap::Close( detailHeightmap );
   }

   /*
   if( m_gbufferAlbedo == NULL )
   {
      assert( m_gbufferNormal == NULL );
      assert( m_gbufferDepth == NULL );

      const D3DSURFACE_DESC & bbSurfDesc = DxEventNotifier::GetBackbufferSurfaceDesc();

      V( device->CreateTexture( bbSurfDesc.Width, bbSurfDesc.Height, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_gbufferAlbedo, NULL ) );
      V( device->CreateTexture( bbSurfDesc.Width, bbSurfDesc.Height, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_gbufferNormal, NULL ) );
      V( device->CreateTexture( bbSurfDesc.Width, bbSurfDesc.Height, 0, D3DUSAGE_RENDERTARGET, D3DFMT_R16F, D3DPOOL_DEFAULT, &m_gbufferDepth, NULL ) );
   }
   */

   if( m_settings.SplattingEnabled && !m_splattingData.Initialized )
   {
      string iniName = vaFindResource( "Media\\splatting.ini" );

      IniParser iniParser;
      iniParser.Open( iniName.c_str() );

      assert( iniParser.getInt("Main:NumLayers", 0) == SplattingData::SplatCount );

      for( int i = 0; i < SplattingData::SplatCount; i++ )
      {
         wstring nameColor     = vaFindResource( vaStringFormat( L"Media\\s%d.dds", i ) );
         wstring nameNormalmap = vaFindResource( vaStringFormat( L"Media\\s%dn.dds", i ) );
         V( D3DXCreateTextureFromFile( GetD3DDevice(), nameColor.c_str(), &m_splattingData.ColorTextures[i] ) );
         V( D3DXCreateTextureFromFile( GetD3DDevice(), nameNormalmap.c_str(), &m_splattingData.NormalmapTextures[i] ) );

         m_splattingData.AlphaAdd[i]    = iniParser.getFloat(vaStringFormat( "Layer%d:AlphaAdd", i ).c_str(), 0.0f );
         m_splattingData.AlphaMul[i]    = iniParser.getFloat(vaStringFormat( "Layer%d:AlphaMul", i ).c_str(), 0.0f );
         m_splattingData.SpecularPow[i] = iniParser.getFloat(vaStringFormat( "Layer%d:SpecularPow", i ).c_str(), 0.0f );
         m_splattingData.SpecularMul[i] = iniParser.getFloat(vaStringFormat( "Layer%d:SpecularMul", i ).c_str(), 0.0f );
      }

      m_splattingData.Initialized = true;
   }
}

HRESULT DemoRenderer::OnCreateDevice( )
{
   HRESULT hr;
 
   {
      SAFE_RELEASE( m_noiseTexture );

      V( GetD3DDevice()->CreateTexture( m_noiseTextureResolution, m_noiseTextureResolution, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_noiseTexture, NULL) );

      int levelCount = m_noiseTexture->GetLevelCount();
      int resolution = m_noiseTextureResolution;

      for( int level = 0; level < levelCount; level++ )
      {
         D3DLOCKED_RECT noiseLockedRect; 
         V( m_noiseTexture->LockRect( level, &noiseLockedRect, NULL, 0 ) );

         unsigned int * texRow = (unsigned int *)noiseLockedRect.pBits;

         for( int y = 0; y < resolution; y++ )
         {
            for( int x = 0; x < resolution; x++ )
            {
               texRow[x] = (0xFF & (int)(randf() * 256.0f));
               texRow[x] |= (0xFF & (int)(randf() * 256.0f)) << 8;

               float ang = randf();
               float fx = sinf( ang * (float)PI * 2.0f ) * 0.5f + 0.5f;
               float fy = sinf( ang * (float)PI * 2.0f ) * 0.5f + 0.5f;

               texRow[x] |= (0xFF & (int)(fx * 256.0f)) << 16;
               texRow[x] |= (0xFF & (int)(fy * 256.0f)) << 24;
            }
            texRow += noiseLockedRect.Pitch / sizeof(*texRow);
         }
         V( m_noiseTexture->UnlockRect(level) );
         resolution /= 2;
      }
   }

   {
      SAFE_RELEASE( m_white1x1Texture );

      V( GetD3DDevice()->CreateTexture( 1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_white1x1Texture, NULL) );

      D3DLOCKED_RECT lockedRect; 
      V( m_white1x1Texture->LockRect( 0, &lockedRect, NULL, 0 ) );
      unsigned int * texRow = (unsigned int *)lockedRect.pBits;
      texRow[0] = 0xFFFFFFFF;
      V( m_white1x1Texture->UnlockRect(0) );

      D3DSURFACE_DESC sd;
      m_white1x1Texture->GetLevelDesc(0, &sd);

      int dbg = 0;
      dbg++;

   }

   return S_OK;
}

HRESULT DemoRenderer::OnResetDevice( const D3DSURFACE_DESC * pBackBufferSurfaceDesc )
{
   return S_OK;
}

void DemoRenderer::OnLostDevice( )
{
   SAFE_RELEASE( m_gbufferAlbedo );
   SAFE_RELEASE( m_gbufferNormal );
   SAFE_RELEASE( m_gbufferDepth );
}

void DemoRenderer::OnDestroyDevice( )
{
   SAFE_RELEASE( m_detailHMTexture );
   SAFE_RELEASE( m_detailNMTexture );
   SAFE_RELEASE( m_noiseTexture );
   SAFE_RELEASE( m_white1x1Texture );

   for( int i = 0; i < _countof( m_splattingData.ColorTextures ); i++ )
      SAFE_RELEASE( m_splattingData.ColorTextures[i] );
   for( int i = 0; i < _countof( m_splattingData.NormalmapTextures ); i++ )
      SAFE_RELEASE( m_splattingData.NormalmapTextures [i] );
   m_splattingData.Initialized = false;
}

void DemoRenderer::Tick( DemoCamera * camera, float deltaTime )
{
   if( vaIsKeyClicked(kcReloadAll) )
   {
      InitializeFromIni();

      OnDestroyDevice();
      OnLostDevice();
      OnCreateDevice();
   }

   bool updateShaderSettings = false;
   
   if( GetDetailmapExists() && vaIsKeyClicked(kcToggleDetailmap) )
   {
      m_settings.DetailHeightmapEnabled = !m_settings.DetailHeightmapEnabled;
      updateShaderSettings = true;
   }

   if( vaIsKeyClicked(kcToggleDebugView) )
   {
      m_debugView = !m_debugView;
   }

   if( vaIsKeyClicked(kcToggleTexturingMode) )
   {
      m_texturingMode = (TexturingMode)((m_texturingMode+1)%TM_MaxValue);
   }
   

   if( updateShaderSettings )
      UpdateShaderSettings();

   InitializeRuntimeData();

   //m_terrainQuadTree.DebugDrawAllNodes();
   m_globalTime += deltaTime;

   if( vaIsKeyClicked( kcToggleWireframe ) )
      m_wireframe = !m_wireframe;
   
   /*
   //if( true ) // prevent camera from going below terrain
   {
      float maxDist = 1000.0f;
      D3DXVECTOR3 camPos = camera->GetPosition();
      D3DXVECTOR3 from = camPos + D3DXVECTOR3( 0.0f, 0.0f, 500.0f );
      D3DXVECTOR3 dir( 0.0f, 0.0f, -1.0f );
      D3DXVECTOR3 hitPos;
      if( m_terrainQuadTree->CoarseIntersectRay( from, dir, maxDist, hitPos ) )
      {
         hitPos.z += 1.0f;
         if( camPos.z < hitPos.z )
         {
            camPos.z = hitPos.z;
            camera->SetPosition( camPos );
         }
      }
   }
   */

   if( m_terrainCursorPos )
   {
      D3DXVECTOR3 a = m_terrainCursorPos;
      D3DXVECTOR3 b = m_terrainCursorPos;

      GetCanvas3D()->DrawBox( a + D3DXVECTOR3(-1.0f, -1.0f, 0.0f), b + D3DXVECTOR3(+1.0f, +1.0f, 100.0f), 0xFF000000, 0xFFFF0000 );
   }
}
//
void DemoRenderer::UpdateShaderSettings()
{
   D3DXMACRO macros[8] = { NULL };

   macros[0].Name       = "USE_DETAIL_HEIGHTMAP";
   macros[0].Definition = (m_settings.DetailHeightmapEnabled)?("1"):("0");

   macros[1].Name       = "USE_SHADOWMAP";
   macros[1].Definition = (false)?("1"):("0");

   macros[2].Name       = "BILINEAR_VERTEX_FILTER_SUPPORTED";
   macros[2].Definition = (vaGetBilinearVertexTextureFilterSupport())?("1"):("0");

   macros[3].Name       = (m_texturingMode==TM_Simple)?("TEXTURING_MODE_SIMPLE"):
                           ((m_settings.SplattingEnabled)?("TEXTURING_MODE_SPLATTING"):("TEXTURING_MODE_IMAGEOVERLAY") );
   macros[3].Definition = "";

   macros[4].Name       = "LIGHTING_ENABLED";
   macros[4].Definition = (m_lightingEnabled)?("1"):("0");

   macros[5].Name       = NULL;
   macros[5].Definition = NULL;

   m_vsTerrainSimple.SetShaderInfo( "Shaders/CDLODTerrain.vsh", "terrainSimple", macros );
   m_psTerrainForwardBasic.SetShaderInfo( "Shaders/CDLODTerrain.psh", "forwardBasic", macros );
   m_psTerrainForwardWireframe.SetShaderInfo( "Shaders/CDLODTerrain.psh", "forwardWireframe", macros );
   m_psTerrainDeferredStandard.SetShaderInfo( "Shaders/CDLODTerrain.psh", "deferredStandard", macros );

   m_psDeferredApplyDirectionalLight.SetShaderInfo( "Shaders/deferred.psh", "applyDirectionalLight", NULL );
   m_psDeferredApplyDbgAlbedo.SetShaderInfo( "Shaders/deferred.psh", "showAlbedoBuffer", NULL );
   m_psDeferredApplyDbgNormal.SetShaderInfo( "Shaders/deferred.psh", "showNormalBuffer", NULL );
   m_psDeferredApplyDbgDepth.SetShaderInfo( "Shaders/deferred.psh", "showDepthBuffer", NULL );
}
//
void DemoRenderer::RenderTerrain( DemoCamera * camera, const CDLODQuadTree::LODSelection & cdlodSelection, 
                                 DxPixelShader * pixelShaderPtr, bool drawWireframe )
{
   HRESULT hr;
   IDirect3DDevice9* device = GetD3DDevice();

   float dbgCl = 0.2f;
   float dbgLODLevelColors[4][4] = { {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, dbgCl, dbgCl, 1.0f}, {dbgCl, 1.0f, dbgCl, 1.0f}, {dbgCl, dbgCl, 1.0f, 1.0f} };

   DxPixelShader & pixelShader = *pixelShaderPtr;

   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // Get the terrain quadtree from the storage
   const CDLODQuadTree & terrainQuadTree = m_storage->GetQuadTree();
   ///////////////////////////////////////////////////////////////////////////////////////////////////

   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // Camera and matrices
   D3DXMATRIX view = camera->GetViewMatrix();
   D3DXMATRIX proj = camera->GetProjMatrix();
   //
   D3DXMATRIX viewProj = view * proj;
   //
   if( drawWireframe )
   {
      // minor wireframe offset!
      D3DXMATRIX offProjWireframe;
      camera->GetZOffsettedProjMatrix(offProjWireframe, 1.003f, 0.03f);
      //
      D3DXMATRIX offViewProjWireframe = view * offProjWireframe;
      V( m_vsTerrainSimple.SetMatrix( "g_viewProjection", offViewProjWireframe ) );
   }
   else
   {
      V( m_vsTerrainSimple.SetMatrix( "g_viewProjection", viewProj ) );
   }
   //
   ///////////////////////////////////////////////////////////////////////////////////////////////////

   ///////////////////////////////////////////////////////////////////////////////////////////////////
   if( m_settings.SplattingEnabled && !drawWireframe )
      SetupSplatting(pixelShaderPtr);
   ///////////////////////////////////////////////////////////////////////////////////////////////////

   V( device->SetVertexShader( m_vsTerrainSimple ) );

   // This contains all settings used to do rendering through CDLODRenderer
   CDLODRendererBatchInfo cdlodBatchInfo;

   cdlodBatchInfo.StreamingStorage         = m_storage;
   cdlodBatchInfo.VertexShader             = &m_vsTerrainSimple;
   cdlodBatchInfo.DetailmapSize            = D3DXVECTOR3( m_settings.DetailHMSizeX, m_settings.DetailHMSizeY, m_settings.DetailHMSizeZ );
   cdlodBatchInfo.VSHeightmapTexHandle     = m_vsTerrainSimple.GetConstantTable()->GetConstantByName( NULL, "g_heightmapVertexTexture" );
   cdlodBatchInfo.VSHeightmapTexInfoHandle = m_vsTerrainSimple.GetConstantTable()->GetConstantByName( NULL, "g_heightmapTextureInfo" );
   cdlodBatchInfo.VSGridToHMConstsHandle   = m_vsTerrainSimple.GetConstantTable()->GetConstantByName( NULL, "g_gridToHMConsts" );
   cdlodBatchInfo.VSGridDimHandle          = m_vsTerrainSimple.GetConstantTable()->GetConstantByName( NULL, "g_gridDim" );
   cdlodBatchInfo.VSQuadScaleHandle        = m_vsTerrainSimple.GetConstantTable()->GetConstantByName( NULL, "g_quadScale" );
   cdlodBatchInfo.VSQuadOffsetHandle       = m_vsTerrainSimple.GetConstantTable()->GetConstantByName( NULL, "g_quadOffset" );
   cdlodBatchInfo.VSMorphConstsHandle      = m_vsTerrainSimple.GetConstantTable()->GetConstantByName( NULL, "g_morphConsts" );
   cdlodBatchInfo.VSNodeCornersHandle      = m_vsTerrainSimple.GetConstantTable()->GetConstantByName( NULL, "g_nodeCorners" );
   cdlodBatchInfo.VSUseDetailmapHandle     = m_vsTerrainSimple.GetConstantTable()->GetConstantByName( NULL, "g_useDetailMap" );
   cdlodBatchInfo.VSGridToDetailmapHandle  = m_vsTerrainSimple.GetConstantTable()->GetConstantByName( NULL, "g_gridToDMConsts" );
   cdlodBatchInfo.PSNormalmapTexHandle        = pixelShader.GetConstantTable()->GetConstantByName( NULL, "g_normalmapTexture" );
   cdlodBatchInfo.PSNormalmapParentTexHandle  = pixelShader.GetConstantTable()->GetConstantByName( NULL, "g_normalmapParentTexture" );
   cdlodBatchInfo.PSGridToNMConstsHandle      = pixelShader.GetConstantTable()->GetConstantByName( NULL, "g_gridToNMConsts" );
   cdlodBatchInfo.PSGridToNMParentConstsHandle= pixelShader.GetConstantTable()->GetConstantByName( NULL, "g_gridToNMParentConsts" );
   cdlodBatchInfo.PSOverlaymapTexHandle       = pixelShader.GetConstantTable()->GetConstantByName( NULL, "g_overlaymapTexture" );
   cdlodBatchInfo.PSOverlaymapParentTexHandle = pixelShader.GetConstantTable()->GetConstantByName( NULL, "g_overlaymapParentTexture" );
   cdlodBatchInfo.PSGridToOMConstsHandle      = pixelShader.GetConstantTable()->GetConstantByName( NULL, "g_gridToOMConsts" );
   cdlodBatchInfo.PSGridToOMParentConstsHandle= pixelShader.GetConstantTable()->GetConstantByName( NULL, "g_gridToOMParentConsts" );
   cdlodBatchInfo.White1x1Texture             = m_white1x1Texture;
   if( m_settings.SplattingEnabled )
      cdlodBatchInfo.PSGridToSplatMatTexConstsHandle = pixelShader.GetConstantTable()->GetConstantByName( NULL, "g_gridToSplatMaterialTextureConsts" );
   else
      cdlodBatchInfo.PSGridToSplatMatTexConstsHandle = NULL;
   cdlodBatchInfo.SplatmapSize             = D3DXVECTOR3( m_settings.SplatMatTexTileSizeX, m_settings.SplatMatTexTileSizeY, 0.0f );

   cdlodBatchInfo.MeshGridDimensions       = m_terrainGridMeshDims;
   cdlodBatchInfo.DetailMeshLODLevelsAffected = 0;

   D3DTEXTUREFILTERTYPE vertexTextureFilterType = (vaGetBilinearVertexTextureFilterSupport())?(D3DTEXF_LINEAR):(D3DTEXF_POINT);

   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // Setup global shader settings
   //
   m_dlodRenderer.SetIndependentGlobalVertexShaderConsts( m_vsTerrainSimple, terrainQuadTree, viewProj, camera->GetPosition() );
   //
   // setup detail heightmap globals if any
   if( m_settings.DetailHeightmapEnabled )
   {
      m_vsTerrainSimple.SetTexture( "g_detailHMVertexTexture", m_detailHMTexture, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, vertexTextureFilterType, vertexTextureFilterType );
      pixelShader.SetTexture( "g_terrainDetailNMTexture", m_detailNMTexture, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      cdlodBatchInfo.DetailMeshLODLevelsAffected = m_settings.DetailMeshLODLevelsAffected;
      m_vsTerrainSimple.SetFloatArray( "g_detailConsts", 0, 0, m_settings.DetailHMSizeZ, (float)m_settings.DetailMeshLODLevelsAffected );
   }
   // end of global shader settings
   ///////////////////////////////////////////////////////////////////////////////////////////////////

   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // Connect selection to our render batch info
   cdlodBatchInfo.CDLODSelection               = &cdlodSelection;

   m_renderStats.TerrainStats.Reset();

   CDLODRenderStats stepStats;

   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // Debug view
   if( m_debugView )
   {
      for( int i = 0; i < cdlodSelection.GetSelectionCount(); i++ )
      {
         const CDLODQuadTree::SelectedNode & nodeSel = cdlodSelection.GetSelection()[i];
         int lodLevel = nodeSel.LODLevel;
         
         bool drawFull = nodeSel.TL && nodeSel.TR && nodeSel.BL && nodeSel.BR;

         unsigned int penColor = (((int)(dbgLODLevelColors[lodLevel % 4][0] * 255.0f) & 0xFF) << 16)
                                 | (((int)(dbgLODLevelColors[lodLevel % 4][1] * 255.0f) & 0xFF) << 8)
                                 | (((int)(dbgLODLevelColors[lodLevel % 4][2] * 255.0f) & 0xFF) << 0)
                                 | (((int)(dbgLODLevelColors[lodLevel % 4][3] * 255.0f) & 0xFF) << 24);

         AABB boundingBox;
         nodeSel.GetAABB( boundingBox, m_storage->GetWorldDesc().RasterSizeX, m_storage->GetWorldDesc().RasterSizeY, m_storage->GetWorldMapDims() );
         boundingBox.Expand( -0.003f );
         if( drawFull )
         {
            GetCanvas3D()->DrawBox( boundingBox.Min, boundingBox.Max, penColor );
         }
         else
         {
            float midX = boundingBox.Center().x;
            float midY = boundingBox.Center().y;

            if( nodeSel.TL )
            {
               AABB bbSub = boundingBox; bbSub.Max.x = midX; bbSub.Max.y = midY;
               bbSub.Expand( -0.002f );
               GetCanvas3D()->DrawBox( bbSub.Min, bbSub.Max, penColor );
            }
            if( nodeSel.TR ) 
            {
               AABB bbSub = boundingBox; bbSub.Min.x = midX; bbSub.Max.y = midY;
               bbSub.Expand( -0.002f );
               GetCanvas3D()->DrawBox( bbSub.Min, bbSub.Max, penColor );
            }
            if( nodeSel.BL ) 
            {
               AABB bbSub = boundingBox; bbSub.Max.x = midX; bbSub.Min.y = midY;
               bbSub.Expand( -0.002f );
               GetCanvas3D()->DrawBox( bbSub.Min, bbSub.Max, penColor );
            }
            if( nodeSel.BR ) 
            {
               AABB bbSub = boundingBox; bbSub.Min.x = midX; bbSub.Min.y = midY;
               bbSub.Expand( -0.002f );
               GetCanvas3D()->DrawBox( bbSub.Min, bbSub.Max, penColor );
            }
         }
      }
   }
   ///////////////////////////////////////////////////////////////////////////////////////////////////

   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // Render
   //
   if( drawWireframe )
   {
      device->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
   }

   for( int i = cdlodSelection.GetMinSelectedLevel(); i <= cdlodSelection.GetMaxSelectedLevel(); i++ )
   {
      if( m_debugView )
      {
         float whiten = 0.8f;
         pixelShader.SetFloatArray( "g_colorMult", whiten + (1.0f-whiten)*dbgLODLevelColors[i % 4][0],
                                                       whiten + (1.0f-whiten)*dbgLODLevelColors[i % 4][1],
                                                       whiten + (1.0f-whiten)*dbgLODLevelColors[i % 4][2],
                                                       whiten + (1.0f-whiten)*dbgLODLevelColors[i % 4][3] );
      }
      else
      {
         pixelShader.SetFloatArray( "g_colorMult", 1.0f, 1.0f, 1.0f, 1.0f );
      }

      cdlodBatchInfo.FilterLODLevel  = i;
      cdlodBatchInfo.PixelShader     = &pixelShader;

      V( device->SetPixelShader( *cdlodBatchInfo.PixelShader ) );
      m_dlodRenderer.Render( cdlodBatchInfo, &stepStats );
      m_renderStats.TerrainStats.Add( stepStats );
   }
   cdlodBatchInfo.FilterLODLevel  = -1;
   //
   if( drawWireframe )
   {
      device->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
   }
   ///////////////////////////////////////////////////////////////////////////////////////////////////
   

   V( device->SetStreamSource( 0, NULL, 0, 0 ) );
   V( device->SetIndices(NULL) );

   V( device->SetVertexShader( NULL ) );
   V( device->SetPixelShader( NULL ) );
}
//
HRESULT DemoRenderer::Render( DemoCamera * camera, DemoSky * lightMgr, float deltaTime )
{

   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // Setup view range and ready ourselves for selection stuff
   if( camera->GetViewRange() < m_settings.MinViewRange )
      camera->SetViewRange( m_settings.MinViewRange );
   if( camera->GetViewRange() > m_settings.MaxViewRange )
      camera->SetViewRange( m_settings.MaxViewRange );

   m_renderStats.Reset();
   m_renderStats.LODLevelCount = m_storage->GetWorldDesc().LODLevelCount;

   D3DXPLANE planes[6];
   camera->GetFrustumPlanes( planes );
   ///////////////////////////////////////////////////////////////////////////////////////////////////

   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // Do the terrain LOD selection based on our camera frustum and settings
   //
   const CDLODQuadTree & quadTree = m_storage->GetQuadTree();
   // this will store the selection of terrain quads that we want to render 
   CDLODQuadTree::LODSelectionOnStack<4096>     cdlodSelection( camera->GetPosition(), camera->GetViewRange(), planes);
   //
   // do the main selection process...
   {
      ProfileScope(CDLOD_QuadTree_LODSelect_DataConnect);
      {
         ProfileScope(CDLOD_QuadTree_LODSelect);
         quadTree.LODSelect( &cdlodSelection );
      }
      m_storage->ConnectSelectionStreamingData( cdlodSelection );
   }
   ///////////////////////////////////////////////////////////////////////////////////////////////////

   {
      ProfileScope(CDLOD_Render);

      HRESULT hr;
      IDirect3DDevice9* device = GetD3DDevice();

      // this is unused in this demo
      bool doDeferred = false;

      IDirect3DSurface9* backbufferSurface = NULL;
      if( doDeferred )
      {
         V( device->GetRenderTarget(0, &backbufferSurface) );

         V( vaSetRenderTargetTexture( 0, m_gbufferAlbedo ) );
         V( vaSetRenderTargetTexture( 1, m_gbufferNormal ) );
         V( vaSetRenderTargetTexture( 2, m_gbufferDepth ) );

         V( device->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB( 1, 0, 0, 0 ), 1.0f, 0 ) );
      }

      device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
      device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
      device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

      //
      // Check if we have too small visibility distance that causes morph between LOD levels to be incorrect.
      if( cdlodSelection.IsVisDistTooSmall() )
      {
   #ifdef _DEBUG
         GetCanvas2D()->DrawString( GetBackbufferSurfaceDesc().Width/2 - 128, GetBackbufferSurfaceDesc().Height/2, 0xFFFF4040, L"Visibility distance might be too low for LOD morph to work correctly!" );
   #endif
      }
      ///////////////////////////////////////////////////////////////////////////////////////////////////

      float fogStart	= camera->GetViewRange() * 0.75f;
      float fogEnd	= camera->GetViewRange() * 0.99f;

      if( doDeferred )
      {
         RenderTerrain( camera, cdlodSelection, &m_psTerrainDeferredStandard, false );
      }
      else
      {
         D3DXVECTOR4 directionalLightDir = D3DXVECTOR4( lightMgr->GetDirectionalLightDir(), 1.0f );
         m_psTerrainForwardBasic.SetFloatArray( "g_directionalLightDir", directionalLightDir, 4 );
         m_psTerrainForwardBasic.SetFloatArray( "g_directionalLightColor", c_lightColorDiffuse, 4 );
         m_psTerrainForwardBasic.SetFloatArray( "g_ambientLightColor", c_lightColorAmbient, 4 );
         m_psTerrainForwardBasic.SetFloatArray( "g_cameraPos", camera->GetPosition(), 3 );

         m_psTerrainForwardBasic.SetFloatArray( "g_fogColor", 100.0f / 255.0f, 100.0f / 255.0f, 128.0f / 255.0f, 0.0 );
         m_psTerrainForwardBasic.SetFloatArray( "g_fogConsts",	fogEnd / ( fogEnd - fogStart ), -1.0f / ( fogEnd - fogStart ), 0, 1.0f/fogEnd );

         RenderTerrain( camera, cdlodSelection, &m_psTerrainForwardBasic, false );
      }

      if( doDeferred )
      {
         V( device->SetRenderTarget( 0, backbufferSurface )  );
         V( device->SetRenderTarget( 1, NULL )  );
         V( device->SetRenderTarget( 2, NULL )  );
         V( device->SetRenderTarget( 3, NULL )  );

         ApplyDeferred( camera, lightMgr );
      }

      if( m_wireframe )
      {
         m_psTerrainForwardWireframe.SetFloatArray( "g_fogColor", 100.0f / 255.0f, 100.0f / 255.0f, 128.0f / 255.0f, 0.0 );
         m_psTerrainForwardWireframe.SetFloatArray( "g_fogConsts",	fogEnd / ( fogEnd - fogStart ), -1.0f / ( fogEnd - fogStart ), 0, 1.0f/fogEnd );

         RenderTerrain( camera, cdlodSelection, &m_psTerrainForwardWireframe, true );
      }

      m_storage->FlushSelectionStreamingData( cdlodSelection );

      SAFE_RELEASE( backbufferSurface );

   }
   return S_OK;
}
//
void DemoRenderer::ApplyDeferred( DemoCamera * camera, DemoSky * lightMgr )
{
   HRESULT hr;
   IDirect3DDevice9* device = GetD3DDevice();

   DxPixelShader * pixelShaderPtr = NULL;
   switch( m_showDeferredBufferType )
   {
   case (0) : pixelShaderPtr = &m_psDeferredApplyDirectionalLight; break;
   case (1) : pixelShaderPtr = &m_psDeferredApplyDbgAlbedo; break;
   case (2) : pixelShaderPtr = &m_psDeferredApplyDbgNormal; break;
   case (3) : pixelShaderPtr = &m_psDeferredApplyDbgDepth; break;
   }
   assert( pixelShaderPtr != NULL );
   DxPixelShader & pixelShader = *pixelShaderPtr;

   const D3DSURFACE_DESC & bbSurfDesc = DxEventNotifier::GetBackbufferSurfaceDesc();

   device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
   device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
   device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );

   V( device->SetPixelShader( pixelShader ) );

   pixelShader.SetTexture( "g_gbufferAlbedoTexture", m_gbufferAlbedo );
   pixelShader.SetTexture( "g_gbufferNormalTexture", m_gbufferNormal );
   pixelShader.SetTexture( "g_gbufferDepthTexture", m_gbufferDepth );

   pixelShader.SetFloatArray( "g_backbufferSize", (float)bbSurfDesc.Width, (float)bbSurfDesc.Height, 
                                                   1.0f / (float)bbSurfDesc.Width, 1.0f / (float)bbSurfDesc.Height );


   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // Camera and matrices and other consts
   //
   D3DXMATRIX view = camera->GetViewMatrix();
   D3DXMATRIX proj = camera->GetProjMatrix();
   //
   D3DXMATRIX viewProj = view * proj;
   //
   pixelShader.SetFloatArray( "g_cameraPos", camera->GetPosition(), 3 );
   //
   // if these are not satisfied, the reconstruction of world space coordinates will not work properly
   assert( proj._34 == 1.0f );
   assert( proj._23 == 0.0f );
   assert( proj._13 == 0.0f );
   assert( view._44 == 1.0f );
   //
   D3DXMATRIX invViewProj;
   D3DXMatrixInverse( &invViewProj, NULL, &viewProj );
   pixelShader.SetFloatArray( "g_projMatrixConsts", proj._33, proj._43 );
   pixelShader.SetMatrix( "g_invViewProjMatrix", invViewProj );
   ///////////////////////////////////////////////////////////////////////////////////////////////////

   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // light and fog consts
   //float fogStart	= camera->GetViewRange() * 0.75f;
   //float fogEnd	= camera->GetViewRange() * 0.99f;
   //V( m_pEffect->SetVector( "g_vFogColor", 0.5, 0.5, 0.75, 0.0 );
   //V( m_pEffect->SetVector( "g_vFogConsts",	fogEnd / ( fogEnd - fogStart ), -1.0f / ( fogEnd - fogStart ), 0, 1.0f/fogEnd ) ) );

   D3DXVECTOR4 directionalLightDir = D3DXVECTOR4( lightMgr->GetDirectionalLightDir(), 1.0f );
   pixelShader.SetFloatArray( "g_directionalLightDir", directionalLightDir, 4 );
   
   pixelShader.SetFloatArray( "g_directionalLightColor", c_lightColorDiffuse, 4 );
   pixelShader.SetFloatArray( "g_ambientLightColor", c_lightColorAmbient, 4 );
   ///////////////////////////////////////////////////////////////////////////////////////////////////


   vaRenderQuad( bbSurfDesc.Width, bbSurfDesc.Height );


   pixelShader.SetTexture( "g_gbufferAlbedoTexture", NULL );
   pixelShader.SetTexture( "g_gbufferNormalTexture", NULL );
   pixelShader.SetTexture( "g_gbufferDepthTexture", NULL );
}
//
void DemoRenderer::MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing )
{
   switch( msg )
   {
   case WM_LBUTTONDOWN:
      m_mouseLeftButtonDown = true;
      *pbNoFurtherProcessing = true;
      break;
   case WM_LBUTTONUP:
      m_mouseLeftButtonDown = false;
      *pbNoFurtherProcessing = true;
      break;
   case WM_MOUSEMOVE:
      m_mouseClientX = ( short )LOWORD( lParam );
      m_mouseClientY = ( short )HIWORD( lParam );
      break;
   case WM_KILLFOCUS:
      m_mouseLeftButtonDown = false;
      break;
   }
}
//
void DemoRenderer::SetRenderGridResolutionMult( int mult )
{ 
   m_currentRenderGridResolutionMult = mult; 
   m_terrainGridMeshDims = m_storage->GetWorldDesc().LeafQuadTreeNodeSize * m_currentRenderGridResolutionMult; 
}
//
void DemoRenderer::SetupSplatting( DxPixelShader * pixelShaderPtr )
{
   for( int i = 0; i < ::min<int>( _countof( m_splattingData.ColorTextures ), _countof( m_splattingData.NormalmapTextures ) ); i++ )
   {
      string nameColor     = vaStringFormat( "g_splatMatTex%d", i );
      string nameNormalmap = vaStringFormat( "g_splatMatNormTex%d", i );
      pixelShaderPtr->SetTexture( nameColor.c_str(), m_splattingData.ColorTextures[i], 
         D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      pixelShaderPtr->SetTexture( nameNormalmap.c_str(), m_splattingData.NormalmapTextures[i], 
         D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
   }

   //float alphaAdd[4]   = { -0.05f, -0.15f, -0.1f, -0.08f };
   //float alphaMul[4]   = { 4.0f,   20.0f,  20.0f, 25.0f  };
   pixelShaderPtr->SetFloatArray( "g_splatAlphaModAdd", &m_splattingData.AlphaAdd[1], 4 );
   pixelShaderPtr->SetFloatArray( "g_splatAlphaModMul", &m_splattingData.AlphaMul[1], 4 );

   //float specularPow[5]   = { 64.0f, 64.0f, 64.0f, 32.0f, 64.0f };
   //float specularMul[5]   = { 0.0f,  0.0f,  0.0f,  0.1f,  0.75f  };
   pixelShaderPtr->SetFloatArray( "g_splatSpecularPow", m_splattingData.SpecularPow, 5 );
   pixelShaderPtr->SetFloatArray( "g_splatSpecularMul", m_splattingData.SpecularMul, 5 );

}
//