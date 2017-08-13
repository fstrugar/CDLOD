//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "DemoRenderer.h"

#include "DemoCamera.h"

#include "DemoSky.h"

#include "DxCanvas.h"

#include "iniparser\src\IniParser.hpp"
#include "TiledBitmap/TiledBitmap.h"

#ifdef MY_EXTENDED_STUFF
#include "IProf/prof.h"
#endif

class SimpleHeightmapSource : public IHeightmapSource
{
private:
   float *                    m_pData;
   int                        m_pitch;    // in bytes

   int                        m_width;
   int                        m_height;

public:
   SimpleHeightmapSource(float * pData, int pitch, int width, int height)
   {
      m_pData = pData;
      m_pitch = pitch;
      m_width = width;
      m_height = height;
   }

   ~SimpleHeightmapSource()
   {
   }

   virtual int             GetSizeX( )    { return m_width; }
   virtual int             GetSizeY( )    { return m_height; }

   virtual unsigned short  GetHeightAt( int x, int y )
   {
      return (unsigned short)(m_pData[ m_pitch / sizeof(*m_pData) * y + x ] * 65535.0f + 0.5f);
   }

   virtual void            GetAreaMinMaxZ( int x, int y, int sizeX, int sizeY, unsigned short & minZ, unsigned short & maxZ )
   {
      assert( x >= 0 && y >= 0 && (x+sizeX) <= m_width && (y+sizeY) <= m_height );
      minZ = 65535;
      maxZ = 0;

      for( int ry = 0; ry < sizeY; ry++ )
      {
         //if( (ry + y) >= rasterSizeY )
         //   break;
         float * scanLine = &m_pData[ x + (ry+y) * (m_pitch/sizeof(*m_pData)) ];
         //int sizeX = ::min( rasterSizeX - x, size );
         for( int rx = 0; rx < sizeX; rx++ )
         {
            unsigned short h = (unsigned short)(scanLine[rx] * 65535.0f + 0.5f);
            minZ = ::min( minZ, h );
            maxZ = ::max( maxZ, h );
         }
      }

   }
};
//
DemoRenderer::DemoRenderer(void)
{
   m_heightmap                = NULL;

   m_terrainHMTexture         = NULL;
   m_terrainNMTexture         = NULL;

   m_detailHMTexture          = NULL;
   m_detailNMTexture          = NULL;

   m_wireframe                = false;

   memset( &m_mapDims, 0, sizeof(m_mapDims) );

   memset( &m_settings, 0, sizeof(m_settings) );

   m_settings.ShadowmapEnabled = false;

   m_mouseLeftButtonDown      = false;
   m_mouseClientX             = 0;
   m_mouseClientY             = 0;

   m_terrainCursorPos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

   m_detailHeightmap          = NULL;

   m_terrainGridMeshDims      = 0;

   UpdateShaderSettings();

   m_noiseTexture = NULL;
   m_noiseTextureResolution = 512;

   if( GetD3DDevice() != NULL )
      OnCreateDevice();

   m_debugView             = false;
}

DemoRenderer::~DemoRenderer(void)
{
   assert( m_terrainHMTexture == NULL );
   assert( m_terrainNMTexture == NULL );
   assert( m_detailHeightmap == NULL );
   assert( m_detailHMTexture == NULL );
   assert( m_detailNMTexture == NULL );
}

void DemoRenderer::Start( VertexAsylum::TiledBitmap * heightmap, const MapDimensions & mapDims )
{
   assert( m_heightmap == NULL );

   m_heightmap    = heightmap;

   m_rasterWidth  = heightmap->Width();
   m_rasterHeight = heightmap->Height();

   m_mapDims      = mapDims;

   InitializeFromIni();

   m_globalTime = 0.0;

   m_mouseLeftButtonDown = false;
}

void DemoRenderer::InitializeFromIni( )
{
   IniParser   iniParser( vaGetIniPath() );

   // DLOD

   m_settings.LeafQuadTreeNodeSize     = iniParser.getInt("CDLOD:LeafQuadTreeNodeSize", 0);
   m_settings.RenderGridResolutionMult = iniParser.getInt("CDLOD:RenderGridResolutionMult", 0);
   m_settings.LODLevelCount            = iniParser.getInt("CDLOD:LODLevelCount", 0);

   if( !vaIsPowOf2(m_settings.LeafQuadTreeNodeSize) || (m_settings.LeafQuadTreeNodeSize < 2) || (m_settings.LeafQuadTreeNodeSize > 1024) )
      vaFatalError( "CDLOD:LeafQuadTreeNodeSize setting is incorrect" );

   m_maxRenderGridResolutionMult = 1;
   while( m_maxRenderGridResolutionMult * m_settings.LeafQuadTreeNodeSize <= 128 ) m_maxRenderGridResolutionMult *= 2;
   m_maxRenderGridResolutionMult /= 2;

   if( !vaIsPowOf2(m_settings.RenderGridResolutionMult) || (m_settings.RenderGridResolutionMult < 1) || (m_settings.LeafQuadTreeNodeSize > m_maxRenderGridResolutionMult) )
      vaFatalError( "CDLOD:RenderGridResolutionMult setting is incorrect" );

   if( (m_settings.LODLevelCount < 2) || (m_settings.LODLevelCount > CDLODQuadTree::c_maxLODLevels) )
      vaFatalError( "CDLOD:LODLevelCount setting is incorrect" );

   
   // Rendering

   m_settings.MinViewRange             = iniParser.getFloat("Rendering:MinViewRange", 0.0f);
   m_settings.MaxViewRange             = iniParser.getFloat("Rendering:MaxViewRange", 0.0f);
   m_settings.LODLevelDistanceRatio    = iniParser.getFloat("Rendering:LODLevelDistanceRatio", 0.0f);

   if( (m_settings.MinViewRange < 1.0f) || (m_settings.MinViewRange > 10000000.0f) )
      vaFatalError( "MinViewRange setting is incorrect" );
   if( (m_settings.MaxViewRange < 1.0f) || (m_settings.MaxViewRange > 10000000.0f) || (m_settings.MinViewRange > m_settings.MaxViewRange) )
      vaFatalError( "MaxViewRange setting is incorrect" );

   if( (m_settings.LODLevelDistanceRatio < 1.5f) || (m_settings.LODLevelDistanceRatio > 16.0f) )
      vaFatalError( "LODLevelDistanceRatio setting is incorrect" );

   m_terrainGridMeshDims = m_settings.LeafQuadTreeNodeSize * m_settings.RenderGridResolutionMult;


   // DetailHeightmap

   const char * detailHeightmapPathRaw = iniParser.getString("DetailHeightmap:Path", "");

   m_settings.DetailHeightmapEnabled = false;
   if( detailHeightmapPathRaw != NULL && detailHeightmapPathRaw[0] != 0 )
   {
      std::string detailHeightmapPath = vaAddProjectDir( detailHeightmapPathRaw );
      
      m_detailHeightmap = VertexAsylum::TiledBitmap::Open( detailHeightmapPath.c_str(), true );
      if( m_detailHeightmap == NULL )
         vaFatalError( "Cannot load detail heightmap from DetailHeightmap:Path" );

      m_settings.DetailHeightmapXYScale      = iniParser.getInt("DetailHeightmap:XYScale", 0);
      m_settings.DetailHeightmapZSize        = iniParser.getFloat("DetailHeightmap:ZSize", 0.0f);
      m_settings.DetailMeshLODLevelsAffected = iniParser.getInt("DetailHeightmap:MeshLODLevelsAffected", 0);

      if( (m_settings.DetailHeightmapXYScale < 1) && (m_settings.DetailHeightmapXYScale > 2048) )
         vaFatalError( "DetailHeightmap:XYScale setting is incorrect" );

      if( m_settings.DetailHeightmapZSize < 0.0f )
         vaFatalError( "DetailHeightmap:HeightScale setting is incorrect" );

      if( m_settings.DetailMeshLODLevelsAffected <= 0 )
         vaFatalError( "DetailHeightmap:MeshLODLevelsAffected setting is incorrect" );

      m_settings.DetailHeightmapEnabled = true;
   }


   // CascadedShadowMaps

   m_settings.ShadowmapEnabled      = iniParser.getInt("CascadedShadowMaps:Enabled", 0) == 1;

   if( vaGetShadowMapSupport() == smsNotSupported )
   {
      m_settings.ShadowmapEnabled = false;
   }

   m_settings.ShadowmapHighQuality  = iniParser.getBool("CascadedShadowMaps:HighQuality", false);

   //if( m_settings.ShadowmapEnabled && (m_settings.ShadowmapTextureResolution < 32 || m_settings.ShadowmapTextureResolution > 8192) )
   //   vaFatalError( "CascadedShadowMaps:TextureResolution setting is incorrect" );

   if( m_settings.ShadowmapEnabled )
   {
      m_shadowsRenderer.Initialize(this);
   }

   UpdateShaderSettings();
}

void DemoRenderer::Stop( )
{
   OnLostDevice();
   OnDestroyDevice();
   m_heightmap    = NULL;

   m_mouseLeftButtonDown = false;

   delete m_detailHeightmap;
   m_detailHeightmap = NULL;
}

inline D3DXVECTOR3 AvgNormalFromQuad( float ha, float hb, float hc, float hd, float sizex, float sizey, float scalez )
{
   D3DXVECTOR3 n0, n1;

   n0.x = - (hb - ha) * scalez * sizey;
   n0.y = - sizex * (hc - ha) * scalez;
   n0.z = sizex * sizey;

   //D3DXVec3Normalize( &n0, &n0 );

   n1.x = -sizey * (hd-hc) * scalez;
   n1.y = ((hb-hc) * sizex - sizex * (hd-hc)) * scalez;
   n1.z = sizey * sizex;


   //D3DXVec3Normalize( &n1, &n1 );

   n0 += n1;
   //D3DXVec3Normalize( &n0, &n0 );

   return n0;
}
//
static inline int CoordClamp( int val, int limit )
{
   if( val < 0 )        return 0;
   if( val > limit - 1 )  return limit - 1;
   return val;
}
//
static inline int CoordWrap( int val, int limit )
{
   if( val < 0 )        return limit + val;
   if( val > limit - 1 )  return val - limit;
   return val;
}
//
static inline int CoordFix( int val, int limit, bool wrap )
{
   if( wrap )
      return CoordWrap( val, limit );
   else
      return CoordClamp( val, limit );
}
//
static void CreateNormalMap( int sizeX, int sizeY, MapDimensions & mapDims, float * heightmapData, int heightmapDataPitch, unsigned int * normalmapData, int normalmapDataPitch, bool wrapEdges )
{
   float stepx = 1.0f / (sizeX-1) * mapDims.SizeX;
   float stepy = 1.0f / (sizeY-1) * mapDims.SizeY;

   const int smoothSteps = 0; // can be 0, 1, 2, ... more steps == slower algorithm
   for( int dist = 1; dist < 2+smoothSteps; dist++ )
   {
      for( int y = 0; y < sizeY; y++ )
      {
         float * hmScanLine0 = &heightmapData[ CoordFix(y-dist, sizeY, wrapEdges) * (heightmapDataPitch/sizeof(float)) ];
         float * hmScanLine1 = &heightmapData[ CoordFix(y+0, sizeY, wrapEdges) * (heightmapDataPitch/sizeof(float)) ];
         float * hmScanLine2 = &heightmapData[ CoordFix(y+dist, sizeY, wrapEdges) * (heightmapDataPitch/sizeof(float)) ];

         unsigned int * nmScanLine = &normalmapData[ y * (normalmapDataPitch/sizeof(unsigned int)) ];

         for( int x = 0; x < sizeX; x++ )
         {
            int xcoordm   = CoordFix( x-dist, sizeX, wrapEdges );
            int xcoord    = CoordFix( x, sizeX, wrapEdges );
            int xcoordp   = CoordFix( x+dist, sizeX, wrapEdges );

            float ha = hmScanLine0[xcoordm];
            float hb = hmScanLine1[xcoordm];
            float hc = hmScanLine2[xcoordm];
            float hd = hmScanLine0[xcoord];
            float he = hmScanLine1[xcoord];
            float hf = hmScanLine2[xcoord];
            float hg = hmScanLine0[xcoordp];
            float hh = hmScanLine1[xcoordp];
            float hi = hmScanLine2[xcoordp];

            D3DXVECTOR3 norm( 0, 0, 0 );
            norm += AvgNormalFromQuad( ha, hb, hd, he, stepx, stepy, mapDims.SizeZ );
            norm += AvgNormalFromQuad( hb, hc, he, hf, stepx, stepy, mapDims.SizeZ );
            norm += AvgNormalFromQuad( hd, he, hg, hh, stepx, stepy, mapDims.SizeZ );
            norm += AvgNormalFromQuad( he, hf, hh, hi, stepx, stepy, mapDims.SizeZ );

            D3DXVec3Normalize( &norm, &norm );

            if( dist > 1 )
            {
               //D3DXVECTOR3 oldNorm( vaHalfFloatUnpack( (unsigned short)(nmScanLine[x] & 0xFFFF) ), vaHalfFloatUnpack( (unsigned short)(nmScanLine[x] >> 16) ), 0 );
               D3DXVECTOR3 oldNorm( ((nmScanLine[x] >> 16) / 65535.0f - 0.5f) / 0.5f, ((nmScanLine[x] & 0xFFFF ) / 65535.0f - 0.5f) / 0.5f, 0 );
               oldNorm.z = sqrtf( 1 - oldNorm.x*oldNorm.x - oldNorm.y*oldNorm.y );

               norm += oldNorm * 1.0f; // use bigger const to add more weight to normals calculated from smaller quads
               D3DXVec3Normalize( &norm, &norm );
            }

            //unsigned short a = vaHalfFloatPack( norm.x );
            //unsigned short b = vaHalfFloatPack( norm.y );
            unsigned short a = (unsigned short)clamp( 65535.0f * ( norm.x * 0.5f + 0.5f ), 0.0f, 65535.0f );
            unsigned short b = (unsigned short)clamp( 65535.0f * ( norm.y * 0.5f + 0.5f ), 0.0f, 65535.0f );

            nmScanLine[x] = (a << 16) | b;
         }
      }
   }
}

void DemoRenderer::InitializeRuntimeData()
{
   IDirect3DDevice9 * device = GetD3DDevice();

   HRESULT hr;

   // If not loaded, load heightmap texture
   if( m_terrainHMTexture == NULL )
   {
      assert( m_rasterWidth   == m_heightmap->Width() );
      assert( m_rasterHeight  == m_heightmap->Height() );
      assert( m_terrainNMTexture == NULL );

      V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, 0, D3DFMT_R32F, D3DPOOL_MANAGED, &m_terrainHMTexture, NULL ) );
      V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 0, 0, D3DFMT_G16R16, D3DPOOL_MANAGED, &m_terrainNMTexture, NULL ) );
      
      //////////////////////////////////////////////////////////////////////////
      // Load heightmap
      //
      D3DLOCKED_RECT heightmapLockedRect; 
      V( m_terrainHMTexture->LockRect( 0, &heightmapLockedRect, NULL, 0 ) );
      
      float * heightmapData = (float *)heightmapLockedRect.pBits;

      for( int y = 0; y < m_rasterHeight; y++ )
      {
         for( int x = 0; x < m_rasterWidth; x++ )
         {
            unsigned short pixel;
            m_heightmap->GetPixel( x, y, &pixel );
            heightmapData[x] = pixel / 65535.0f;
         }
         heightmapData += heightmapLockedRect.Pitch / sizeof(*heightmapData);
      }

      
      //m_terrainQuadTree.Create( m_mapDims, m_rasterWidth, m_rasterHeight, (unsigned short *)heightmapLockedRect.pBits, heightmapLockedRect.Pitch, m_quadMinSize );
      SimpleHeightmapSource heightmapSrc( (float *)heightmapLockedRect.pBits, heightmapLockedRect.Pitch, m_rasterWidth, m_rasterHeight );
      //
      CDLODQuadTree::CreateDesc createDesc;
      createDesc.pHeightmap         = &heightmapSrc;
      createDesc.LeafRenderNodeSize = m_settings.LeafQuadTreeNodeSize;
      createDesc.LODLevelCount      = m_settings.LODLevelCount;
      createDesc.MapDims            = m_mapDims;

      m_terrainQuadTree.Create( createDesc );

      if( m_terrainNMTexture != NULL )
      {
         D3DLOCKED_RECT normalmapLockedRect;
         V( m_terrainNMTexture->LockRect( 0, &normalmapLockedRect, NULL, 0 ) );
         CreateNormalMap( m_rasterWidth, m_rasterHeight, m_mapDims, (float *)heightmapLockedRect.pBits, heightmapLockedRect.Pitch, (unsigned int *)normalmapLockedRect.pBits, normalmapLockedRect.Pitch, false );
         m_terrainNMTexture->UnlockRect( 0 );
      }

      m_terrainHMTexture->UnlockRect( 0 );

      //V( D3DXFilterTexture( m_terrainHMTexture, NULL, 0, D3DX_FILTER_BOX ) );
      V( D3DXFilterTexture( m_terrainNMTexture, NULL, 0, D3DX_FILTER_BOX ) );
   }

   if( m_detailHeightmap != NULL && m_detailHMTexture == NULL )
   {
      int dmWidth = m_detailHeightmap->Width();
      int dmHeight = m_detailHeightmap->Height();
      V( device->CreateTexture( dmWidth, dmHeight, 1, 0, D3DFMT_R32F, D3DPOOL_MANAGED, &m_detailHMTexture, NULL ) );

      V( device->CreateTexture( dmWidth, dmHeight, 0, 0, D3DFMT_G16R16, D3DPOOL_MANAGED, &m_detailNMTexture, NULL ) );

      //////////////////////////////////////////////////////////////////////////
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
            m_detailHeightmap->GetPixel( x, y, &pixel );
            heightmapData[x] = pixel / 65535.0f;
         }
         heightmapData += heightmapLockedRect.Pitch / sizeof(*heightmapData);
      }

      if( m_detailNMTexture != NULL )
      {
         D3DLOCKED_RECT normalmapLockedRect;
         V( m_detailNMTexture->LockRect( 0, &normalmapLockedRect, NULL, 0 ) );
         MapDimensions dims = m_mapDims;
         dims.SizeX = (dims.SizeX / (float)m_rasterWidth) * dmWidth / m_settings.DetailHeightmapXYScale;
         dims.SizeY = (dims.SizeY / (float)m_rasterHeight) * dmHeight / m_settings.DetailHeightmapXYScale;
         dims.SizeZ = m_settings.DetailHeightmapZSize * 0.5f; // 0.5f is a magic number - although I think the algorithm is correct, it still looks better if there's a bit less normal strength and more vertex displacement strength
         CreateNormalMap( dmWidth, dmHeight, dims, (float *)heightmapLockedRect.pBits, heightmapLockedRect.Pitch, (unsigned int *)normalmapLockedRect.pBits, normalmapLockedRect.Pitch, true );
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

   return S_OK;
}

HRESULT DemoRenderer::OnResetDevice( const D3DSURFACE_DESC * pBackBufferSurfaceDesc )
{
   return S_OK;
}

void DemoRenderer::OnLostDevice( )
{
}

void DemoRenderer::OnDestroyDevice( )
{
   SAFE_RELEASE( m_terrainHMTexture );
   SAFE_RELEASE( m_terrainNMTexture );
   SAFE_RELEASE( m_detailHMTexture );
   SAFE_RELEASE( m_detailNMTexture );
   SAFE_RELEASE( m_noiseTexture );
}

void DemoRenderer::SetShadowQuality( int shadowQuality )
{
   if( (vaGetShadowMapSupport() == smsNotSupported) || (shadowQuality == 0) )
   {
      m_settings.ShadowmapEnabled = false;
      m_settings.ShadowmapHighQuality = false;
   }
   else if( shadowQuality == 1 )
   {
      m_settings.ShadowmapEnabled = true;
      m_settings.ShadowmapHighQuality = false;
   } 
   else if( shadowQuality == 2 )
   {
      m_settings.ShadowmapEnabled = true;
      m_settings.ShadowmapHighQuality = true;
   } 
   else
   {
      assert( false );
   }
   m_shadowsRenderer.Initialize(this);
   UpdateShaderSettings();
}

void DemoRenderer::Tick( DemoCamera * camera, float deltaTime )
{
#ifdef MY_EXTENDED_STUFF
   Prof(DemoRenderer_Tick);
#endif

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

   if( vaIsKeyClicked(kcToggleShadowmap) )
   {
      if( !m_settings.ShadowmapEnabled && ( vaGetShadowMapSupport() != smsNotSupported ) )
      {
         m_settings.ShadowmapEnabled = true;
         m_settings.ShadowmapHighQuality = false;
      }
      else
      {
         if( !m_settings.ShadowmapHighQuality )
            m_settings.ShadowmapHighQuality = true;
         else
            m_settings.ShadowmapEnabled = false;
      }
      m_shadowsRenderer.Initialize(this);
      updateShaderSettings = true;
   }

   if( vaIsKeyClicked(kcToggleDebugView) )
   {
      m_debugView = !m_debugView;
   }

   if( updateShaderSettings )
      UpdateShaderSettings();

   InitializeRuntimeData();

   //m_terrainQuadTree.DebugDrawAllNodes();
   m_globalTime += deltaTime;

   if( vaIsKeyClicked( kcToggleWireframe ) )
      m_wireframe = !m_wireframe;
   
   //if( true ) // prevent camera from going below terrain
   {
      float maxDist = 1000.0f;
      D3DXVECTOR3 camPos = camera->GetPosition();
      D3DXVECTOR3 from = camPos + D3DXVECTOR3( 0.0f, 0.0f, 500.0f );
      D3DXVECTOR3 dir( 0.0f, 0.0f, -1.0f );
      D3DXVECTOR3 hitPos;
      if( m_terrainQuadTree.IntersectRay( from, dir, maxDist, hitPos ) )
      {
         hitPos.z += 1.0f;
         if( camPos.z < hitPos.z )
         {
            camPos.z = hitPos.z;
            camera->SetPosition( camPos );
         }
      }
   }

   if( m_mouseLeftButtonDown )
   {
      D3DXVECTOR3 mousePosN( (float)m_mouseClientX, (float)m_mouseClientY, 0 );
      D3DXVECTOR3 mousePosF( (float)m_mouseClientX, (float)m_mouseClientY, 1 );

      D3DXVECTOR3 camPos;// = camera->GetPosition();
      D3DXVECTOR3 camDir;// = camera->GetDirection();

      D3DVIEWPORT9 viewport;
      GetD3DDevice()->GetViewport( &viewport );

      D3DXMATRIX world; D3DXMatrixIdentity( &world );
      D3DXMATRIX view   = camera->GetViewMatrix();
      D3DXMATRIX proj   = camera->GetProjMatrix();

      D3DXVec3Unproject( &camPos, &mousePosN, &viewport, &proj, &view, &world );
      D3DXVec3Unproject( &camDir, &mousePosF, &viewport, &proj, &view, &world );
      camDir -= camPos;
      D3DXVec3Normalize( &camDir, &camDir );

      float maxDist = camera->GetViewRange();
      D3DXVECTOR3 hitPos;
      if( m_terrainQuadTree.IntersectRay( camPos, camDir, maxDist, hitPos ) )
      {
         m_terrainCursorPos = hitPos;
         m_terrainCursorVisible = true;
      }
      else
         m_terrainCursorVisible = false;
   }

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
   D3DXMACRO macros[6] = { NULL };

   macros[0].Name       = "USE_DETAIL_HEIGHTMAP";
   macros[0].Definition = (m_settings.DetailHeightmapEnabled)?("1"):("0");

   macros[1].Name       = "USE_SHADOWMAP";
   macros[1].Definition = (m_settings.ShadowmapEnabled)?("1"):("0");

   macros[2].Name       = "SHADOWMAP_HIGHQUALITY";
   macros[2].Definition = (m_settings.ShadowmapHighQuality)?("1"):("0");

   macros[3].Name       = "USE_NVIDIA_SHADOWS";
   macros[3].Definition = (vaGetShadowMapSupport()==smsNVidiaShadows)?("1"):("0");

   macros[4].Name       = "BILINEAR_VERTEX_FILTER_SUPPORTED";
   macros[4].Definition = (vaGetBilinearVertexTextureFilterSupport())?("1"):("0");

   macros[5].Name       = NULL;
   macros[5].Definition = NULL;

   m_vsTerrainSimple.SetShaderInfo( "Shaders/CDLODTerrain.vsh", "terrainSimple", macros );
   m_psTerrainFlat.SetShaderInfo( "Shaders/CDLODTerrain.psh", "flatShading", macros );

   if( m_settings.ShadowmapEnabled )
   {
      m_shadowsRenderer.UpdateShaderSettings( macros );
   }
}
//
void DemoRenderer::RenderTerrain( DemoCamera * camera, DemoSky * lightMgr, const CDLODQuadTree::LODSelection & cdlodSelection )
{
   HRESULT hr;
   IDirect3DDevice9* device = GetD3DDevice();

   float dbgCl = 0.2f;
   float dbgLODLevelColors[4][4] = { {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, dbgCl, dbgCl, 1.0f}, {dbgCl, 1.0f, dbgCl, 1.0f}, {dbgCl, dbgCl, 1.0f, 1.0f} };

   //////////////////////////////////////////////////////////////////////////
   // Local shaders
   static DxPixelShader psWireframe( "Shaders/misc.psh", "justOutputColor" );
   //////////////////////////////////////////////////////////////////////////

   //////////////////////////////////////////////////////////////////////////
   // Camera and matrices
   D3DXMATRIX view = camera->GetViewMatrix();
   D3DXMATRIX proj = camera->GetProjMatrix();
   //
   // minor wireframe offset!
   D3DXMATRIX offProjWireframe;
   camera->GetZOffsettedProjMatrix(offProjWireframe, 1.001f, 0.01f);
   //
   D3DXMATRIX viewProj = view * proj;
   D3DXMATRIX offViewProjWireframe = view * offProjWireframe;
   //
   V( m_vsTerrainSimple.SetMatrix( "g_viewProjection", viewProj ) );
   //
   //////////////////////////////////////////////////////////////////////////

   V( device->SetVertexShader( m_vsTerrainSimple ) );

   // This contains all settings used to do rendering through CDLODRenderer
   CDLODRendererBatchInfo cdlodBatchInfo;

   cdlodBatchInfo.VertexShader          = &m_vsTerrainSimple;
   cdlodBatchInfo.VSGridDimHandle       = m_vsTerrainSimple.GetConstantTable()->GetConstantByName( NULL, "g_gridDim" );
   cdlodBatchInfo.VSQuadScaleHandle     = m_vsTerrainSimple.GetConstantTable()->GetConstantByName( NULL, "g_quadScale" );
   cdlodBatchInfo.VSQuadOffsetHandle    = m_vsTerrainSimple.GetConstantTable()->GetConstantByName( NULL, "g_quadOffset" );
   cdlodBatchInfo.VSMorphConstsHandle   = m_vsTerrainSimple.GetConstantTable()->GetConstantByName( NULL, "g_morphConsts" );
   cdlodBatchInfo.VSUseDetailMapHandle  = m_vsTerrainSimple.GetConstantTable()->GetConstantByName( NULL, "g_useDetailMap" );
   cdlodBatchInfo.MeshGridDimensions    = m_terrainGridMeshDims;
   cdlodBatchInfo.DetailMeshLODLevelsAffected = 0;


   D3DTEXTUREFILTERTYPE vertexTextureFilterType = (vaGetBilinearVertexTextureFilterSupport())?(D3DTEXF_LINEAR):(D3DTEXF_POINT);

   //////////////////////////////////////////////////////////////////////////
   // Setup global shader settings
   //
   m_dlodRenderer.SetIndependentGlobalVertexShaderConsts( m_vsTerrainSimple, m_terrainQuadTree, viewProj, camera->GetPosition() );
   //
   // setup detail heightmap globals if any
   if( m_settings.DetailHeightmapEnabled )
   {
      m_vsTerrainSimple.SetTexture( "g_detailHMVertexTexture", m_detailHMTexture, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, vertexTextureFilterType, vertexTextureFilterType );
      m_psTerrainFlat.SetTexture( "g_terrainDetailNMTexture", m_detailNMTexture, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );

      int dmWidth = m_detailHeightmap->Width();
      int dmHeight = m_detailHeightmap->Height();

      float sizeX = (m_mapDims.SizeX / (float)m_rasterWidth) * dmWidth / m_settings.DetailHeightmapXYScale;
      float sizeY = (m_mapDims.SizeY / (float)m_rasterHeight) * dmHeight / m_settings.DetailHeightmapXYScale;

      cdlodBatchInfo.DetailMeshLODLevelsAffected = m_settings.DetailMeshLODLevelsAffected;

      m_vsTerrainSimple.SetFloatArray( "g_detailConsts", sizeX, sizeY, m_settings.DetailHeightmapZSize, (float)m_settings.DetailMeshLODLevelsAffected );
   }
   //
   if( m_settings.ShadowmapEnabled )
   {
      m_psTerrainFlat.SetTexture( "g_noiseTexture", m_noiseTexture, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_POINT );
   }

   //
   // setup light and fog globals
   {
      //
      //float fogStart	= pCamera->GetViewRange() * 0.75f;
      //float fogEnd	= pCamera->GetViewRange() * 0.98f;
      //V( m_vsTerrainSimple.SetVector( "g_fogConsts", fogEnd / ( fogEnd - fogStart ), -1.0f / ( fogEnd - fogStart ), 0, 1.0f/fogEnd ) );
      if( lightMgr != NULL )
      {
         D3DXVECTOR4 diffLightDir = D3DXVECTOR4( lightMgr->GetDirectionalLightDir(), 1.0f );
         V( m_vsTerrainSimple.SetVector( "g_diffuseLightDir", diffLightDir ) );
      }
   }
   //
   // vertex textures
   V( m_vsTerrainSimple.SetTexture( "g_terrainHMVertexTexture", m_terrainHMTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, vertexTextureFilterType, vertexTextureFilterType ) );
   //V( m_vsTerrainSimple.SetTexture( "g_terrainNMVertexTexture", m_terrainNMTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR ) );
   //
   // global pixel shader consts
   m_psTerrainFlat.SetFloatArray( "g_lightColorDiffuse", c_lightColorDiffuse, 4 );
   m_psTerrainFlat.SetFloatArray( "g_lightColorAmbient", c_lightColorAmbient, 4 );
   //psTerrainFlatVersions[i]->SetFloatArray( device, "g_fogColor", c_fogColor, 4 );
   //
   V( m_psTerrainFlat.SetTexture( "g_terrainNMTexture", m_terrainNMTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR ) );
   //
   // end of global shader settings
   //////////////////////////////////////////////////////////////////////////

   //////////////////////////////////////////////////////////////////////////
   // Connect selection to our render batch info
   cdlodBatchInfo.CDLODSelection               = &cdlodSelection;

   m_renderStats.TerrainStats.Reset();

   CDLODRenderStats stepStats;

   //////////////////////////////////////////////////////////////////////////
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
         nodeSel.GetAABB( boundingBox, m_rasterWidth, m_rasterHeight, m_mapDims );
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
   //////////////////////////////////////////////////////////////////////////

   //////////////////////////////////////////////////////////////////////////
   // Render
   //
   for( int i = cdlodSelection.GetMinSelectedLevel(); i <= cdlodSelection.GetMaxSelectedLevel(); i++ )
   {
      if( m_debugView )
      {
         float whiten = 0.8f;
         m_psTerrainFlat.SetFloatArray( "g_colorMult", whiten + (1.0f-whiten)*dbgLODLevelColors[i % 4][0],
                                                       whiten + (1.0f-whiten)*dbgLODLevelColors[i % 4][1],
                                                       whiten + (1.0f-whiten)*dbgLODLevelColors[i % 4][2],
                                                       whiten + (1.0f-whiten)*dbgLODLevelColors[i % 4][3] );
      }
      else
         m_psTerrainFlat.SetFloatArray( "g_colorMult", 1.0f, 1.0f, 1.0f, 1.0f );

      cdlodBatchInfo.FilterLODLevel  = i;
      cdlodBatchInfo.PixelShader     = &m_psTerrainFlat;

      DWORD texFilter = D3DTEXF_LINEAR;
      
      if( m_settings.ShadowmapEnabled )
      {
         const CascadedShadowMap::CSMLayer & csmLayer = m_shadowsRenderer.GetCascadeLayerForDLODLevel( i );

         V( m_vsTerrainSimple.SetMatrix( "g_shadowView", csmLayer.ShadowView ) );
         V( m_vsTerrainSimple.SetMatrix( "g_shadowProj", csmLayer.ShadowProj ) );
         V( m_psTerrainFlat.SetTexture( "g_shadowMapTexture", csmLayer.ShadowMapDepth, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, texFilter, texFilter, texFilter ) );
         float csmL2LayerSamplingTexelRadius = csmLayer.SamplingTexelRadius;

         if( i+1 < m_shadowsRenderer.GetCascadeLayerCount() )
         {
            const CascadedShadowMap::CSMLayer & csmL2Layer = m_shadowsRenderer.GetCascadeLayerForDLODLevel( i+1 );
            V( m_vsTerrainSimple.SetMatrix( "g_shadowL2View", csmL2Layer.ShadowView ) );
            V( m_vsTerrainSimple.SetMatrix( "g_shadowL2Proj", csmL2Layer.ShadowProj ) );
            m_psTerrainFlat.SetTexture( "g_shadowL2MapTexture", csmL2Layer.ShadowMapDepth, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, texFilter, texFilter, texFilter );
            csmL2LayerSamplingTexelRadius = csmL2Layer.SamplingTexelRadius;
         }
         else
         {
            V( m_vsTerrainSimple.SetMatrix( "g_shadowL2View", csmLayer.ShadowView ) );
            V( m_vsTerrainSimple.SetMatrix( "g_shadowL2Proj", csmLayer.ShadowProj ) );
            m_psTerrainFlat.SetTexture( "g_shadowL2MapTexture", csmLayer.ShadowMapDepth, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, texFilter, texFilter, texFilter );
         }
         m_psTerrainFlat.SetFloatArray( "g_shadowMapConsts", (float)m_shadowsRenderer.GetTextureResolution(), csmLayer.SamplingTexelRadius, csmL2LayerSamplingTexelRadius, 0.0f );

         float dummyNoiseScaleModifier = 0.04f;
         float depthNoiseScale = ::max( csmLayer.WorldSpaceTexelSizeX, csmLayer.WorldSpaceTexelSizeY ) * 3.0f;
         V( m_vsTerrainSimple.SetFloatArray( "g_shadowNoiseConsts", csmLayer.NoiseScaleX * dummyNoiseScaleModifier, csmLayer.NoiseScaleY * dummyNoiseScaleModifier, depthNoiseScale, 0.0f ) );

         if( vaGetShadowMapSupport() == smsATIShadows )
         {
            int sms0 = m_psTerrainFlat.GetTextureSamplerIndex( "g_shadowMapTexture" );
            int sms1 = m_psTerrainFlat.GetTextureSamplerIndex( "g_shadowL2MapTexture" );
            device->SetSamplerState( sms0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET4 );
            if( sms1 != -1 )
               device->SetSamplerState( sms1, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET4 );
         }
      }

      V( device->SetPixelShader( *cdlodBatchInfo.PixelShader ) );
      m_dlodRenderer.Render( cdlodBatchInfo, &stepStats );
      m_renderStats.TerrainStats.Add( stepStats );

      if( m_settings.ShadowmapEnabled && vaGetShadowMapSupport() == smsATIShadows )
      {
         int sms0 = m_psTerrainFlat.GetTextureSamplerIndex( "g_shadowMapTexture" );
         int sms1 = m_psTerrainFlat.GetTextureSamplerIndex( "g_shadowL2MapTexture" );
         device->SetSamplerState( sms0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET1 );
         if( sms1 != -1 )
            device->SetSamplerState( sms1, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET1 );
      }

   }
   cdlodBatchInfo.FilterLODLevel  = -1;
   //
   if( m_settings.ShadowmapEnabled )
   {
      m_psTerrainFlat.SetTexture( "g_shadowMapTexture", NULL );
      m_psTerrainFlat.SetTexture( "g_shadowL2MapTexture", NULL );
   }
   //
   if( m_wireframe )
   {
      V( m_vsTerrainSimple.SetMatrix( "g_viewProjection", offViewProjWireframe ) );
      device->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
      psWireframe.SetFloatArray( "g_justOutputColorValue", 0.0f, 0.0f, 0.0f, 1.0f );

      cdlodBatchInfo.PixelShader     = &psWireframe;
      V( device->SetPixelShader( *cdlodBatchInfo.PixelShader ) );
      m_dlodRenderer.Render( cdlodBatchInfo, &stepStats );
      m_renderStats.TerrainStats.Add( stepStats );

      device->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
   }
   //////////////////////////////////////////////////////////////////////////

   V( device->SetStreamSource(0, NULL, 0, 0 ) );
   V( device->SetIndices(NULL) );

   V( device->SetVertexShader( NULL ) );
   V( device->SetPixelShader( NULL ) );
}
//
HRESULT DemoRenderer::Render( DemoCamera * camera, DemoSky * lightMgr, float deltaTime )
{
#ifdef MY_EXTENDED_STUFF
   Prof(DemoRenderer_Render);
#endif

   if( camera->GetViewRange() < m_settings.MinViewRange )
      camera->SetViewRange( m_settings.MinViewRange );

   m_renderStats.Reset();
   m_renderStats.LODLevelCount = m_settings.LODLevelCount;

   D3DXPLANE planes[6];
   camera->GetFrustumPlanes( planes );

   //////////////////////////////////////////////////////////////////////////
   // Do the terrain LOD selection based on our camera
   //
   // this will store the selection of terrain quads that we want to render 
   CDLODQuadTree::LODSelectionOnStack<4096>     cdlodSelection( camera->GetPosition(), camera->GetViewRange() * 0.95f, planes, m_settings.LODLevelDistanceRatio );
   //
   // do the main selection process...
   m_terrainQuadTree.LODSelect( &cdlodSelection );
   //
   /*
   // stress test
   for( int i = 0; i < 100; i++ )
   {
      CDLODQuadTree::LODSelectionOnStack<2048>     dlodSelection2( camera->GetPosition(), camera->GetViewRange() * 0.95f, planes, m_settings.LODLevelDistanceRatio );
      //
      // do the main selection process...
      m_terrainQuadTree.LODSelect( &dlodSelection2 );
   }
   */
   //
   // Check if we have too small visibility distance that causes morph between LOD levels to be incorrect.
   if( cdlodSelection.IsVisDistTooSmall() )
   {
      GetCanvas2D()->DrawString( GetBackbufferSurfaceDesc().Width/2 - 128, GetBackbufferSurfaceDesc().Height/2, 0xFFFF4040, L"Visibility distance might be too low for LOD morph to work correctly!" );
   }
   //////////////////////////////////////////////////////////////////////////

   if( m_settings.ShadowmapEnabled )
   {
      m_shadowsRenderer.Render( deltaTime, camera, lightMgr, m_terrainGridMeshDims, true, cdlodSelection, &m_renderStats.TerrainShadowStats );
   }

   RenderTerrain( camera, lightMgr, cdlodSelection );

 /*
   // display shadow map
   if( m_settings.ShadowmapEnabled )
   {
      if( m_shadowsRenderer.GetCascadeLayer(0).ShadowMap != 0 )
         vaDebugDisplayTexture( m_shadowsRenderer.GetCascadeLayer(0).ShadowMap, 200, 200, 512, 512 );
   }
   */

   return S_OK;
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