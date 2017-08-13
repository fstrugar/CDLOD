///////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "..\Common.h"
#include "..\CDLODQuadTree.h"
#include "..\DxEventNotifier.h"

#include "CDLODStreamingTypeBase.h"
#include "CDLODStreamingTypeTextureBase.h"
#include "CDLODStreamingIOManager.h"

#include "Core/Threading/vaSemaphore.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// Main class for all terrain data storage - loading, storing, initialization, etc
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// A bit of info here: 
//
//  - Base coordinate system used by CDLODStreamingStorage and subsystems is the input heightmap 
//    raster space. Various BlockSize-s and other coordinates are represented in it, and in case 
//    other coordinate systems are used, the base one is usually marked as a 'raster' space. This 
//    is only used for specifying streaming block location and sizes, the actual streaming data can 
//    be in any resolution and/or format (can also be vector data), as long as it can be split into 
//    fixed size streamable LOD blocks.
//  - If not otherwise stated, 'block' is a streamable block in the CDLODStreamingStorage system,
//    and 'quad' is a node of the CDLODQuadTree.
///////////////////////////////////////////////////////////////////////////////////////////////////

class CDLODStreamingTypeHeightmap;

class CDLODStreamingStorage : DxEventReceiver
{
public:
   // Types

   struct SelectedNodeDataInfo
   {
      uint16                        HeightCornerTL;
      uint16                        HeightCornerTR;
      uint16                        HeightCornerBL;
      uint16                        HeightCornerBR;

      // Heightmap
      IDirect3DTexture9 *           Heightmap;
      CDLODStreamingTypeTextureBase::TextureBlockRenderInfo
                                    HeightmapInfo;

      // Normalmap
      IDirect3DTexture9 *           Normalmap;
      CDLODStreamingTypeTextureBase::TextureBlockRenderInfo
                                    NormalmapInfo;
      IDirect3DTexture9 *           ParentNormalmap;
      CDLODStreamingTypeTextureBase::TextureBlockRenderInfo
                                    ParentNormalmapInfo;

      // Overlay image
      IDirect3DTexture9 *           Overlaymap;
      CDLODStreamingTypeTextureBase::TextureBlockRenderInfo
                                    OverlaymapInfo;
      IDirect3DTexture9 *           ParentOverlaymap;
      CDLODStreamingTypeTextureBase::TextureBlockRenderInfo
                                    ParentOverlaymapInfo;
   };

public:

   struct SourceDataDesc
   {
      VertexAsylum::TiledBitmap *   Heightmap;
      VertexAsylum::TiledBitmap *   Normalmap;
      VertexAsylum::TiledBitmap *   Overlaymap;
      bool                          NormalmapUseDXT5NMCompression;
      bool                          OverlaymapUseDXT1Compression;
      bool                          UseZLIBCompression;

      // Used to store any other CDLOD-unrelated stuff like detailmap, camera settings, etc, that
      // should be embedded into streamable storage in the form of one buffer.
      const void *                  MetaDataBuffer;
      int                           MetaDataBufferSize;

      // Determines the streaming block raster size (width and height)
      int                           HeightmapBlockSize;
      int                           NormalmapBlockSize;
      int                           OverlaymapBlockSize;

      // Level of detail offset at which a quad will sample
      int                           HeightmapLODOffset;
      int                           NormalmapLODOffset;
      int                           OverlaymapLODOffset;
   };

   struct ExportResults
   {
      int                        QuadTreeStorageSize;
      int64                      HeightmapStorageSize;
      int64                      NormalmapStorageSize;
      int64                      OverlaymapStorageSize;
      int64                      TotalStorageSize;
   };

   struct WorldDesc
   {
      // Base heightmap size
      int                        RasterSizeX;
      int                        RasterSizeY;

      // Map size in world space
      MapDimensions              MapDims;

      // LOD settings
      int                        LeafQuadTreeNodeSize;
      int                        RenderGridResolutionMult;
      int                        LODLevelCount;
      float                      LODLevelDistanceRatio;
   };

   struct MemoryUseInfo
   {
      uint32                     QuadtreeMemory;
      uint32                     StreamingThreadsMemory;
      uint32                     HeightmapLoadedBlocks;
      uint32                     HeightmapTextureMemory;
      uint32                     HeightmapCacheMemory;
      uint32                     NormalmapLoadedBlocks;
      uint32                     NormalmapTextureMemory;
      uint32                     NormalmapCacheMemory;
      uint32                     OverlaymapLoadedBlocks;
      uint32                     OverlaymapTextureMemory;
      uint32                     OverlaymapCacheMemory;

      void                       Reset()  { memset(this, 0, sizeof(*this)); }
   };


   // Variables

private:

   // 
   CDLODMinMaxMap                    m_minMaxMaps[CDLODQuadTree::c_maxLODLevels];
   WorldDesc                         m_worldDesc;

   std::vector<CDLODStreamingTypeBase*>   m_streamingDataHandlers;

   // specific data handlers
   CDLODStreamingTypeHeightmap *     m_heightmapDataHandler;
   CDLODStreamingTypeTextureBase *   m_normalmapDataHandler;
   CDLODStreamingTypeTextureBase *   m_overlaymapDataHandler;

   CDLODQuadTree                     m_quadTree;

   static const int                 c_selectedNodeDataLinkBufferSize = 2048;
   SelectedNodeDataInfo             m_selectedNodeDataLinkBuffer[c_selectedNodeDataLinkBufferSize];
   int                              m_selectedNodeDataLinkBufferCount;

   VertexAsylum::vaStream *         m_storageStream;
   VertexAsylum::vaSemaphore        m_storageStreamSemaphore;

   unsigned char *                  m_metadataBuffer;
   int                              m_metadataBufferSize;

   CDLODStreamingIOManager *         m_IOManager;

   MemoryUseInfo                    m_memoryUseInfo;

public:
   CDLODStreamingStorage();
   virtual ~CDLODStreamingStorage();

   bool                       Create( const WorldDesc & worldSettings, const SourceDataDesc & sourceDataDesc, const char * scdlodPath, 
                                       vaProgressReportProcPtr progressReportProcPtr = NULL, ExportResults * exportResults = NULL );
   bool                       Open( const char * scdlodPath );
   void                       Close();
   void                       Clean();

   bool                       IsOpen();

   int32                      GetMetadataBufferSize() const             { return m_metadataBufferSize; }
   const unsigned char *      GetMetadataBuffer() const                 { return m_metadataBuffer; }

   inline uint32              AddObserver(const D3DXVECTOR3 & pos, float maxViewRange, bool forceLoad);
   inline void                UpdateObserver( uint32 observerID, const D3DXVECTOR3 & pos, float maxViewRange );
   inline bool                RemoveObserver( uint32 observerID );

   void                       Tick( float deltaTime );

   bool                       IsCurrentlyStreaming();

   const WorldDesc &          GetWorldDesc() const                      { return m_worldDesc; }
   const MapDimensions &      GetWorldMapDims() const                   { return m_worldDesc.MapDims; }

   const CDLODQuadTree &       GetQuadTree() const                       { return m_quadTree; }         
   int                        GetLODLevelCount() const                  { return m_quadTree.GetLODLevelCount(); }

   inline void                RasterToWorld2D( int rasterX, int rasterY, float & outWorldX, float & outWorldY ) const;
   inline void                RasterToWorld3D( int rasterX, int rasterY, int rasterZ, float & outWorldX, float & outWorldY, float & outWorldZ ) const;

   // Fills all the CDLODQuadTree::SelectedNode instances with streamed data and metadata required for rendering.
   // While any selection data is connected certain streaming operations will be prevented, such as deletion of data, so
   // this should be used only while rendering with FlushSelectionStreamingData() called immediately after.
   bool                       ConnectSelectionStreamingData( CDLODQuadTree::LODSelection & selection );
   // Cleans the internal storage used by FillSelectionStreamingData() - must be called when finished rendering.
   void                       FlushSelectionStreamingData( CDLODQuadTree::LODSelection & selection );
   const SelectedNodeDataInfo *  
                              GetSelectedNodeDataInfo( int streamingDataID ) const;

   const MemoryUseInfo &      GetCurrentMemoryUseInfo( ) const          { return m_memoryUseInfo; }

   int                        GetHeightmapDataLODOffset( ) const;

   bool                       HasNormalmapData() const                  { return m_normalmapDataHandler != NULL; }

private:
   // DirectX-related stuff
   virtual HRESULT 				OnCreateDevice();
   virtual HRESULT            OnResetDevice(const D3DSURFACE_DESC* pBackBufferSurfaceDesc);
   virtual void               OnLostDevice();
   virtual void               OnDestroyDevice();

private:
   friend class CDLODStreamingIOManager;
   VertexAsylum::vaStream *   GetDataStream()                           { return m_storageStream; }
   VertexAsylum::vaSemaphore& GetDataStreamSemaphore()                  { return m_storageStreamSemaphore; }

};


///////////////////////////////////////////////////////////////////////////////////////////////////
// Inline definitions
///////////////////////////////////////////////////////////////////////////////////////////////////

inline void CDLODStreamingStorage::RasterToWorld2D( int rasterX, int rasterY, float & outWorldX, float & outWorldY ) const
{
   // RasterSizeX and RasterSizeY are heightmap texture resolution; since heightmap texels represent vertex height values
   // they have no volume; the volume is made in between them, thus the are they cover is '(RasterSizeX-1) * (RasterSizeY-1)'

   outWorldX = m_worldDesc.MapDims.MinX + m_worldDesc.MapDims.SizeX * rasterX / (float)(m_worldDesc.RasterSizeX-1);
   outWorldY = m_worldDesc.MapDims.MinY + m_worldDesc.MapDims.SizeY * rasterY / (float)(m_worldDesc.RasterSizeY-1);
}

inline void CDLODStreamingStorage::RasterToWorld3D( int rasterX, int rasterY, int rasterZ, float & outWorldX, float & outWorldY, float & outWorldZ ) const
{
   // RasterSizeX and RasterSizeY are heightmap texture resolution; since heightmap texels represent vertex height values
   // they have no volume; the volume is made in between them, thus the are they cover is '(RasterSizeX-1) * (RasterSizeY-1)'

   outWorldX = m_worldDesc.MapDims.MinX + m_worldDesc.MapDims.SizeX * rasterX / (float)(m_worldDesc.RasterSizeX-1);
   outWorldY = m_worldDesc.MapDims.MinY + m_worldDesc.MapDims.SizeY * rasterY / (float)(m_worldDesc.RasterSizeY-1);
   outWorldZ = m_worldDesc.MapDims.MinZ + m_worldDesc.MapDims.SizeZ * rasterZ / 65535.0f;
}

inline const CDLODStreamingStorage::SelectedNodeDataInfo * CDLODStreamingStorage::GetSelectedNodeDataInfo( int streamingDataID ) const
{
   assert( (streamingDataID >= 0) && (streamingDataID < m_selectedNodeDataLinkBufferCount) );
   return &m_selectedNodeDataLinkBuffer[streamingDataID];
}

uint32 CDLODStreamingStorage::AddObserver(const D3DXVECTOR3 & pos, float maxViewRange, bool forceLoad)
{
   assert( m_IOManager != NULL );
   return m_IOManager->AddObserver(pos, maxViewRange, forceLoad);
}

void CDLODStreamingStorage::UpdateObserver( uint32 observerID, const D3DXVECTOR3 & pos, float maxViewRange )
{
   assert( m_IOManager != NULL );
   return m_IOManager->UpdateObserver( observerID, pos, maxViewRange );
}

bool CDLODStreamingStorage::RemoveObserver( uint32 observerID )
{
   assert( m_IOManager != NULL );
   return m_IOManager->RemoveObserver( observerID );
}
