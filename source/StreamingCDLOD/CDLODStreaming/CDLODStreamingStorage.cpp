//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "CDLODStreamingStorage.h"

#include "..\DxCanvas.h"

#include "..\DemoCamera.h"

#include "..\CDLODTools.h"

#include "CDLODStreamingTypeHeightmap.h"
#include "CDLODStreamingTypeNormalmap.h"
#include "CDLODStreamingTypeOverlayImg.h"

#include "Core/IO/vaFileStream.h"
#include "Core/IO/vaMemoryStream.h"
#include "Core/Common/vaTempMemoryBuffer.h"
#include "Core/Compression/vaCompression.h"


CDLODStreamingStorage::CDLODStreamingStorage()
 : m_storageStreamSemaphore(1, 1)
{
   memset( &m_worldDesc, 0, sizeof(m_worldDesc) );
   //memset( &m_sourceDataDesc, 0, sizeof(m_sourceDataDesc) );
   m_selectedNodeDataLinkBufferCount = 0;

   m_heightmapDataHandler = NULL;
   m_normalmapDataHandler = NULL;
   m_overlaymapDataHandler = NULL;

   m_storageStream = NULL;

   m_metadataBuffer = 0;
   m_metadataBufferSize = 0;

   m_IOManager = NULL;

   m_memoryUseInfo.Reset();
}

CDLODStreamingStorage::~CDLODStreamingStorage()
{
   OnLostDevice();
   OnDestroyDevice();
   Close();
}

bool CDLODStreamingStorage::Create( const WorldDesc & worldDesc, const SourceDataDesc & sourceDataDesc, 
                                    const char * scdlodPath, vaProgressReportProcPtr progressReportProcPtr, ExportResults * exportResults )
{
   if( !vaIsPowOf2(worldDesc.LeafQuadTreeNodeSize) || (worldDesc.LeafQuadTreeNodeSize < 4) || (worldDesc.LeafQuadTreeNodeSize > 1024) )
   {
      assert( false );
      vaFatalError( "CDLOD:LeafQuadTreeNodeSize setting is incorrect" );
      return false;
   }

   int maxRenderGridResolutionMult = 1;
   while( (maxRenderGridResolutionMult * worldDesc.LeafQuadTreeNodeSize) <= 128 ) 
      maxRenderGridResolutionMult *= 2;
   maxRenderGridResolutionMult /= 2;

   if( !vaIsPowOf2(worldDesc.RenderGridResolutionMult) || (worldDesc.RenderGridResolutionMult < 1) || 
      (worldDesc.RenderGridResolutionMult > maxRenderGridResolutionMult) )
   {
      assert( false );
      vaFatalError( "CDLOD:RenderGridResolutionMult setting is incorrect" );
      return false;
   }

   if( (worldDesc.LODLevelCount < 2) || (worldDesc.LODLevelCount > CDLODQuadTree::c_maxLODLevels) )
   {
      assert( false );
      vaFatalError( "CDLOD:LODLevelCount setting is incorrect" );
      return false;
   }

   if( (worldDesc.LODLevelDistanceRatio < 1.5f ) || (worldDesc.LODLevelDistanceRatio > 10.0f ) )
   {
      assert( false );
      vaFatalError( "CDLOD:LODLevelDistanceRatio setting is incorrect" );
      return false;
   }


   // HeightmapLODOffset must be big enough to allow for given RenderGridResolutionMult
   int minBaseLODOffset = vaLog2( worldDesc.RenderGridResolutionMult );
   if( (1 << minBaseLODOffset) < worldDesc.RenderGridResolutionMult )
      minBaseLODOffset++;

   if( sourceDataDesc.HeightmapLODOffset < minBaseLODOffset )
   {
      assert( false );
      vaFatalError( "Streaming:HeightmapLODOffset setting is incorrect (too small in relation to RenderGridResolutionMult)" );
      return false;
   }

   // grid size used to render a quad
   const int renderGridSize = worldDesc.LeafQuadTreeNodeSize * worldDesc.RenderGridResolutionMult;

   // maybe this could be the same as below: (sourceDataDesc.HeightmapBlockSize >> sourceDataDesc.HeightmapLODOffset) < worldDesc.LeafQuadTreeNodeSize
   if( (sourceDataDesc.HeightmapBlockSize >> sourceDataDesc.HeightmapLODOffset) < renderGridSize )
   {
      assert( false );
      vaFatalError( "Streaming:HeightmapBlockSize/HeightmapLODOffset incorrect ( (HeightmapBlockSize >> HeightmapLODOffset) < (LeafQuadTreeNodeSize * RenderGridResolutionMult) )" );
      return false;
   }

   if( (sourceDataDesc.Normalmap != NULL) && 
       ((sourceDataDesc.NormalmapBlockSize >> sourceDataDesc.NormalmapLODOffset) < worldDesc.LeafQuadTreeNodeSize) )
   {
      assert( false );
      vaFatalError( "Streaming:NormalmapBlockSize/NormalmapLODOffset incorrect ( (NormalmapBlockSize >> NormalmapLODOffset) < LeafQuadTreeNodeSize )" );
      return false;
   }

   if( (sourceDataDesc.Overlaymap != NULL) && 
       (sourceDataDesc.OverlaymapBlockSize >> sourceDataDesc.OverlaymapLODOffset) < worldDesc.LeafQuadTreeNodeSize )
   {
      assert( false );
      vaFatalError( "Streaming:OverlaymapBlockSize/NormalmapLODOffset incorrect ( (OverlaymapBlockSize >> OverlaymapLODOffset) < LeafQuadTreeNodeSize )" );
      return false;
   }

   if( progressReportProcPtr != NULL ) progressReportProcPtr(0.02f);

   m_memoryUseInfo.Reset();

   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // Create min-max maps required for the quadtree 
   vaGenerateMinMaxMapLevels( (VertexAsylum::TiledBitmap *)sourceDataDesc.Heightmap, worldDesc.LeafQuadTreeNodeSize, 
                              worldDesc.LODLevelCount, m_minMaxMaps, true,
                              progressReportProcPtr, 0.2f, 0.0f );
   ///////////////////////////////////////////////////////////////////////////////////////////////////

   if( progressReportProcPtr != NULL ) progressReportProcPtr(0.2f);

   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // create output file and fill in header and non-streamable data

   VertexAsylum::vaFileStream outFileStream;
   if( !outFileStream.Open( scdlodPath, false ) )
   {
      assert( false );
      vaFatalError( "Unable to open output .scdlod file name" );
      return false;
   }

   // write header

   // version
   if( !outFileStream.WriteValue<int32>( 3 ) )
   {
      vaFatalError("Unable to write CDLODStreamingStorage header");
      return false;
   }

   // write world description
   if( !outFileStream.WriteValue<WorldDesc>( worldDesc ) )
   {
      vaFatalError("Unable to write CDLODStreamingStorage header");
      return false;
   }

   // write metadata buffer
   if( !outFileStream.WriteValue<int32>(sourceDataDesc.MetaDataBufferSize) )
   {
      vaFatalError("Unable to write CDLODStreamingStorage header");
      return false;
   }

   if( sourceDataDesc.MetaDataBufferSize > 0 )
   {
      if( outFileStream.Write( sourceDataDesc.MetaDataBuffer, sourceDataDesc.MetaDataBufferSize ) != sourceDataDesc.MetaDataBufferSize )
      {
         vaFatalError("Unable to write CDLODStreamingStorage header");
         return false;
      }
   }

   int64 quadTreeStorageSize = outFileStream.GetPosition();
   for( int i = 0; i < worldDesc.LODLevelCount; i++ )
   {
      CDLODMinMaxMap & minMaxMap = m_minMaxMaps[i];
      minMaxMap.Save( &outFileStream );
      if( progressReportProcPtr != NULL ) progressReportProcPtr(0.25f);
   }
   quadTreeStorageSize = outFileStream.GetPosition() - quadTreeStorageSize;
   ///////////////////////////////////////////////////////////////////////////////////////////////////

   if( progressReportProcPtr != NULL ) progressReportProcPtr(0.25f);

   // Create the actual quadtree
   CDLODQuadTree::CreateDesc qtCreateDesc;
   qtCreateDesc.LeafRenderNodeSize     = worldDesc.LeafQuadTreeNodeSize;
   qtCreateDesc.LODLevelCount          = worldDesc.LODLevelCount;
   qtCreateDesc.LODLevelDistanceRatio  = worldDesc.LODLevelDistanceRatio;
   qtCreateDesc.MapDims                = worldDesc.MapDims;
   qtCreateDesc.RasterSizeX            = worldDesc.RasterSizeX;
   qtCreateDesc.RasterSizeY            = worldDesc.RasterSizeY;
   qtCreateDesc.MinMaxMaps             = m_minMaxMaps;
   m_quadTree.Create( qtCreateDesc );

   m_worldDesc = worldDesc;

   //m_sourceDataDesc = sourceDataDesc;

   CDLODStreamingTypeBase::ExportSettings exportSettings;
   exportSettings.LeafQuadTreeNodeSize          = worldDesc.LeafQuadTreeNodeSize;
   exportSettings.NormalmapUseDXT5NMCompression = sourceDataDesc.NormalmapUseDXT5NMCompression;
   exportSettings.OverlaymapUseDXT1Compression  = sourceDataDesc.OverlaymapUseDXT1Compression;
   exportSettings.UseZLIBCompression            = sourceDataDesc.UseZLIBCompression;


   // Create streamable data handlers

   m_heightmapDataHandler = new CDLODStreamingTypeHeightmap( this, sourceDataDesc.HeightmapBlockSize, sourceDataDesc.HeightmapLODOffset,
                                                            m_streamingDataHandlers.size(), sourceDataDesc.Heightmap );
   m_streamingDataHandlers.push_back( m_heightmapDataHandler );

   if( sourceDataDesc.Normalmap != NULL )
   {
      m_normalmapDataHandler = new CDLODStreamingTypeNormalmap( this, sourceDataDesc.NormalmapBlockSize, sourceDataDesc.NormalmapLODOffset,
                                                               m_streamingDataHandlers.size(), sourceDataDesc.Normalmap, exportSettings );
      m_streamingDataHandlers.push_back( m_normalmapDataHandler );
   }

   if( sourceDataDesc.Overlaymap != NULL )
   {
      m_overlaymapDataHandler = new CDLODStreamingTypeOverlayImg( this, sourceDataDesc.OverlaymapBlockSize, sourceDataDesc.OverlaymapLODOffset,
                                                                  m_streamingDataHandlers.size(), sourceDataDesc.Overlaymap, exportSettings );
      m_streamingDataHandlers.push_back( m_overlaymapDataHandler );
   }

   // Save header and streamable data handlers headers - first pass (only reserve header space for later use)

   // Number of data handlers
   if( !outFileStream.WriteValue<int32>( (int32)m_streamingDataHandlers.size() ) )
   {
      vaFatalError("Unable to write CDLODStreamingStorage header");
      return false;
   }

   int64 headersBegin = outFileStream.GetPosition();

   // Write thread memory that will be required to decompress
   outFileStream.WriteValue<int32>(0);

   for( size_t i = 0; i < m_streamingDataHandlers.size(); i++ )
   {
      CDLODStreamingTypeBase * item = m_streamingDataHandlers[i];
      CDLODStreamingTypeBase::StorageTypeID typeID = item->GetStorageTypeID();

      if( !outFileStream.WriteValue<uint32>( (uint32)typeID ) )
      {
         vaFatalError("Unable to write CDLODStreamingStorage header");
         return false;
      }

      item->SaveHeader( &outFileStream, true );
   }
   int64 headersEnd = outFileStream.GetPosition();

   struct ExportInfo
   {
      int64                   HeightmapDataSize;
      int64                   NormalmapDataSize;
      int64                   OverlaymapDataSize;

      int                     ProgressQuadsX;
      int                     ProgressQuadsY;
      int                     ProgressQuadsSize;
      int                     ProgressQuadsLODLevel;
      vaProgressReportProcPtr   ProgressProcPtr;

      int32                   StreamingThreadMemoryRequired;
   };

   ExportInfo exportInfo;
   memset( &exportInfo, 0, sizeof(exportInfo) );

   exportInfo.ProgressProcPtr = progressReportProcPtr;
   if( progressReportProcPtr != NULL ) progressReportProcPtr(0.25f);

   exportInfo.ProgressQuadsSize     = m_quadTree.GetTopNodeSize()/4;
   exportInfo.ProgressQuadsLODLevel = m_quadTree.GetLODLevelCount()-3;
   exportInfo.ProgressQuadsX        = (m_worldDesc.RasterSizeX + exportInfo.ProgressQuadsSize - 1) /  exportInfo.ProgressQuadsSize;
   exportInfo.ProgressQuadsY        = (m_worldDesc.RasterSizeY + exportInfo.ProgressQuadsSize - 1) /  exportInfo.ProgressQuadsSize;
   
   exportInfo.StreamingThreadMemoryRequired = 0;

   class StreamableDataExporterHelper : public CDLODQuadTree::QuadEnumeratorCallback
   {
      std::vector<CDLODStreamingTypeBase*> * m_streamingDataHandlersPtr;
      VertexAsylum::vaStream *               m_outStream;
      CDLODStreamingTypeBase::ExportSettings m_exportSettings;
      ExportInfo*                            m_exportInfo;

      int                                    m_progressQuadsToDo;
      int                                    m_progressQuadsDone;

   public:
      StreamableDataExporterHelper( std::vector<CDLODStreamingTypeBase*> * streamingData, VertexAsylum::vaStream * outStream, 
         const CDLODStreamingTypeBase::ExportSettings & exportSettings, ExportInfo * exportInfo ) 
         : m_streamingDataHandlersPtr( streamingData ), m_outStream(outStream), m_exportSettings(exportSettings),
            m_exportInfo(exportInfo)
      { 
         m_progressQuadsDone = 0;
         m_progressQuadsToDo = m_exportInfo->ProgressQuadsX * m_exportInfo->ProgressQuadsY;
      }


      virtual bool   OnQuadTouched( const int rasterX, const int rasterY, const int size, const int LODLevel, 
                                    const unsigned short minZ, const unsigned short maxZ, const AABB & boundingBox )
      {
         for( size_t i = 0; i < m_streamingDataHandlersPtr->size(); i++ )
         {
            CDLODStreamingTypeBase * streamingDataMgr = (*m_streamingDataHandlersPtr)[i];

            int blockX, blockY, blockLODLevel;
            streamingDataMgr->MapQuadToBlock( rasterX, rasterY, size, LODLevel, blockX, blockY, blockLODLevel );

            if( streamingDataMgr->GetBlock( blockX, blockY, blockLODLevel ) != NULL )
            {
               assert( false ); // since we're doing FreeBlock() after exporting each block this shouldn't ever happen
               continue;
            }

            // Already exported? Just skip it then!
            CDLODStreamingTypeBase::BlockStorageMetadata * blockMetadata = streamingDataMgr->GetBlockMetadata( blockX, blockY, blockLODLevel );
            if( blockMetadata->StorageSize != NULL )
               continue;

            // Create new block
            CDLODStreamingTypeBase::Block * block = static_cast<CDLODStreamingTypeBase::Block*>( 
               streamingDataMgr->AllocateBlock(blockX, blockY, blockLODLevel) );

            static VertexAsylum::vaTempMemoryBuffer tempBufferStorage;
            const int tempBufferSize = 64 * 1024 * 1024; // 64meg should be enough for any block...
            
            static VertexAsylum::vaTempMemoryBuffer tempBufferCompStorage;
            const int tempBufferCompSize = 64 * 1024 * 1024; // 64meg should be enough for any block...

            byte * tempBuffer = tempBufferStorage.GetBuffer( tempBufferSize );
            byte * tempCompBuffer = tempBufferCompStorage.GetBuffer( tempBufferSize );

            VertexAsylum::vaMemoryStream memStream( tempBuffer, tempBufferSize );

            // Export to memory stream
            streamingDataMgr->ExportBlockForQuad( &memStream, block, rasterX, rasterY, size, LODLevel, m_exportSettings );

            // Compress
            VertexAsylum::vaCompression::CompressionType compressionType = 
               (m_exportSettings.UseZLIBCompression)?(VertexAsylum::vaCompression::CT_ZLIB):(VertexAsylum::vaCompression::CT_None);
            int outCmpSize = tempBufferCompSize;
            VertexAsylum::vaCompression::SimplePack( compressionType, tempBuffer, (int32)memStream.GetPosition(), tempCompBuffer, outCmpSize );

            m_exportInfo->StreamingThreadMemoryRequired = 
               ::max( m_exportInfo->StreamingThreadMemoryRequired, (int32)memStream.GetPosition() + outCmpSize + 64 );
        
            // Write to storage
            int64 blockPosFrom = m_outStream->GetPosition();
            m_outStream->Write( tempCompBuffer, outCmpSize );
            int64 blockPosTo = m_outStream->GetPosition();

            streamingDataMgr->FreeBlock( blockX, blockY, blockLODLevel );

            blockMetadata->StoragePos  = blockPosFrom;
            blockMetadata->StorageSize = blockPosTo - blockPosFrom;

            switch( streamingDataMgr->GetStorageTypeID() )
            {
            case( CDLODStreamingTypeBase::STID_Heightmap ):  m_exportInfo->HeightmapDataSize += blockMetadata->StorageSize; break;
            case( CDLODStreamingTypeBase::STID_Normalmap ):  m_exportInfo->NormalmapDataSize += blockMetadata->StorageSize; break;
            case( CDLODStreamingTypeBase::STID_Overlaymap ): m_exportInfo->OverlaymapDataSize += blockMetadata->StorageSize; break;
            }

            float progress = m_progressQuadsDone / (float)(m_progressQuadsToDo); //-1);
            if( m_exportInfo->ProgressProcPtr != NULL ) m_exportInfo->ProgressProcPtr(0.3f + progress * 0.7f);
         }
         
         if( LODLevel == m_exportInfo->ProgressQuadsLODLevel )
         {
            m_progressQuadsDone++;
         }

         float progress = m_progressQuadsDone / (float)(m_progressQuadsToDo); //-1);
         if( m_exportInfo->ProgressProcPtr != NULL ) m_exportInfo->ProgressProcPtr(0.3f + progress * 0.7f);

         return true;
      }
   };

   StreamableDataExporterHelper dataExporter( &m_streamingDataHandlers, &outFileStream, exportSettings, &exportInfo );

   m_quadTree.EnumerateQuadsRecursive( static_cast<CDLODQuadTree::QuadEnumeratorCallback*>(&dataExporter), 
                                       0, 0, m_quadTree.GetRasterSizeX(), m_quadTree.GetRasterSizeY() );

   int64 dataEnd = outFileStream.GetPosition();

   // Save our and streamable data handlers headers - second pass (now write required header data into reserve header space)
   outFileStream.Seek( headersBegin );

   // Write thread memory that will be required to decompress
   exportInfo.StreamingThreadMemoryRequired = (int)(exportInfo.StreamingThreadMemoryRequired) + 256; // add some safety
   outFileStream.WriteValue<int32>(exportInfo.StreamingThreadMemoryRequired);

   for( size_t i = 0; i < m_streamingDataHandlers.size(); i++ )
   {
      CDLODStreamingTypeBase * item = m_streamingDataHandlers[i];
      CDLODStreamingTypeBase::StorageTypeID typeID = item->GetStorageTypeID();

      if( !outFileStream.WriteValue<uint32>( (uint32)typeID ) )
      {
         vaFatalError("Unable to write CDLODStreamingStorage header");
         return false;
      }

      item->SaveHeader( &outFileStream, false );
   }
   int64 headersEndTwo = outFileStream.GetPosition();
   if( headersEnd != headersEndTwo )
   {
      vaFatalError("Unable to write CDLODStreamingStorage file (mismatched streamable header sizes)");
      return false;
   }

   outFileStream.Seek( dataEnd );

   Clean();

   if( progressReportProcPtr != NULL ) progressReportProcPtr(1.0f);

   if( exportResults != NULL )
   {
      exportResults->QuadTreeStorageSize = (int)quadTreeStorageSize;
      exportResults->HeightmapStorageSize = exportInfo.HeightmapDataSize;
      exportResults->NormalmapStorageSize = exportInfo.NormalmapDataSize;
      exportResults->OverlaymapStorageSize = exportInfo.OverlaymapDataSize;
      exportResults->TotalStorageSize = outFileStream.GetPosition();
   }

   return true;
}

bool CDLODStreamingStorage::IsOpen( )
{
   return m_storageStream != NULL;
}

bool CDLODStreamingStorage::Open( const char * scdlodPath )
{
   if( IsOpen() )
   {
      // already open?
      return false;
   }

   VertexAsylum::vaFileStream * storageStream = new VertexAsylum::vaFileStream();
   m_storageStream = storageStream;

   if( !storageStream->Open( scdlodPath, true ) )
   {
      delete m_storageStream;
      return false;
   }

   m_memoryUseInfo.Reset();

   int version;
   if( !m_storageStream->ReadValue<int32>(version) ) { vaFatalError("Unable to read CDLODStreamingStorage version"); return false; }

   if( version != 3 )
   {
      vaFatalError("Incorrect CDLODStreamingStorage version");
      return false;
   }

   if( !m_storageStream->ReadValue<WorldDesc>(m_worldDesc) )  { vaFatalError("Unable to read CDLODStreamingStorage header"); return false; }


   if( !m_storageStream->ReadValue<int32>(m_metadataBufferSize) ) { vaFatalError("Unable to read CDLODStreamingStorage header"); return false; }

   if( m_metadataBufferSize > 0 )
   {
      m_metadataBuffer = new unsigned char[m_metadataBufferSize];
      if( m_storageStream->Read( m_metadataBuffer, m_metadataBufferSize ) != m_metadataBufferSize ) 
         { vaFatalError("Unable to read CDLODStreamingStorage header"); return false; }
   }

   int64 minMaxMapStartPos = m_storageStream->GetPosition();

   for( int i = 0; i < m_worldDesc.LODLevelCount; i++ )
   {
      CDLODMinMaxMap & minMaxMap = m_minMaxMaps[i];
      minMaxMap.Load( m_storageStream );
   }

   m_memoryUseInfo.QuadtreeMemory = (int32)(m_storageStream->GetPosition()-minMaxMapStartPos);

   // Create the actual quadtree
   CDLODQuadTree::CreateDesc qtCreateDesc;
   qtCreateDesc.LeafRenderNodeSize     = m_worldDesc.LeafQuadTreeNodeSize;
   qtCreateDesc.LODLevelCount          = m_worldDesc.LODLevelCount;
   qtCreateDesc.LODLevelDistanceRatio  = m_worldDesc.LODLevelDistanceRatio;
   qtCreateDesc.MapDims                = m_worldDesc.MapDims;
   qtCreateDesc.RasterSizeX            = m_worldDesc.RasterSizeX;
   qtCreateDesc.RasterSizeY            = m_worldDesc.RasterSizeY;
   qtCreateDesc.MinMaxMaps             = m_minMaxMaps;
   m_quadTree.Create( qtCreateDesc );

   int32 streamingDataHandlersCount;
   if( !m_storageStream->ReadValue<int32>(streamingDataHandlersCount) ) 
   { 
      vaFatalError("Unable to read CDLODStreamingStorage header"); 
      return false; 
   }

   assert( m_streamingDataHandlers.size() == 0 );

   int32 streamingThreadMemoryRequired = 8 * 1024 * 1024;
   if( !m_storageStream->ReadValue<int32>(streamingThreadMemoryRequired) ) 
   { 
      vaFatalError("Unable to read CDLODStreamingStorage header"); 
      return false; 
   }
   assert( streamingThreadMemoryRequired > 0 );

   for( int i = 0; i < streamingDataHandlersCount; i++ )
   {
      uint32 typeIDStorage;
      if( !m_storageStream->ReadValue<uint32>(typeIDStorage) )      { vaFatalError("Unable to read CDLODStreamingStorage header"); return false; }
      CDLODStreamingTypeBase::StorageTypeID typeID = (CDLODStreamingTypeBase::StorageTypeID)typeIDStorage;

      CDLODStreamingTypeBase * storage = NULL;
      switch( typeID )
      {
      case( CDLODStreamingTypeBase::STID_Heightmap ):
         {
            assert( m_heightmapDataHandler == NULL );
            if( m_heightmapDataHandler != NULL ) vaFatalError("Heightmap streaming type already created?");
            m_heightmapDataHandler = new CDLODStreamingTypeHeightmap( this, m_streamingDataHandlers.size(), 3 * 1024 * 1024 );
            storage = m_heightmapDataHandler;
         }
         break;
      case( CDLODStreamingTypeBase::STID_Normalmap ):
         {
            assert( m_normalmapDataHandler == NULL );
            if( m_normalmapDataHandler != NULL ) vaFatalError("Heightmap streaming type already created?");
            m_normalmapDataHandler = new CDLODStreamingTypeNormalmap( this, m_streamingDataHandlers.size(), 4 * 1024 * 1024 );
            storage = m_normalmapDataHandler;
         }
         break;
      case( CDLODStreamingTypeBase::STID_Overlaymap ):
         {
            assert( m_overlaymapDataHandler == NULL );
            if( m_overlaymapDataHandler != NULL ) vaFatalError("Heightmap streaming type already created?");
            m_overlaymapDataHandler = new CDLODStreamingTypeOverlayImg( this, m_streamingDataHandlers.size(), 4 * 1024 * 1024 );
            storage = m_overlaymapDataHandler;
         }
         break;
      default:
         vaFatalError("Unrecognized streaming type");
         return false;
         break;
      }

      storage->LoadHeaderAndInitialize( m_storageStream );

      m_streamingDataHandlers.push_back( storage );
   }

   m_IOManager = new CDLODStreamingIOManager( *this, m_streamingDataHandlers, streamingThreadMemoryRequired );

   return true;
}

void CDLODStreamingStorage::Close( )
{
   Clean();
}

void CDLODStreamingStorage::Clean( )
{
   // stop all streaming
   if( m_IOManager != NULL )
   {
      OnLostDevice();

      delete m_IOManager;
      m_IOManager = NULL;
   }

   if( m_storageStream != NULL )
   {
      m_storageStream->Close();
      delete m_storageStream;
   }

   memset( &m_worldDesc, 0, sizeof(m_worldDesc) );
   for( int i = 0; i < _countof(m_minMaxMaps); i++ ) 
      m_minMaxMaps[i].Deinitialize();

   for( size_t i = 0; i < m_streamingDataHandlers.size(); i++ )
   {
      m_streamingDataHandlers[i]->Clean();
      delete m_streamingDataHandlers[i];
   }
   ::erase( m_streamingDataHandlers );

   m_heightmapDataHandler = NULL;
   m_normalmapDataHandler = NULL;
   m_overlaymapDataHandler = NULL;

   //if( m_sourceDataDesc.HeightmapAutodelete && (m_sourceDataDesc.Heightmap != NULL) )
   //   VertexAsylum::TiledBitmap::Close( m_sourceDataDesc.Heightmap );
   //if( m_sourceDataDesc.NormalmapAutodelete && (m_sourceDataDesc.Normalmap != NULL) )
   //   VertexAsylum::TiledBitmap::Close( m_sourceDataDesc.Normalmap );
   //if( m_sourceDataDesc.OverlaymapAutodelete && (m_sourceDataDesc.Overlaymap != NULL) )
   //   VertexAsylum::TiledBitmap::Close( m_sourceDataDesc.Overlaymap );
   //memset( &m_sourceDataDesc, 0, sizeof(m_sourceDataDesc) );

   if( m_metadataBuffer != NULL )
   {
      delete[] m_metadataBuffer;
      m_metadataBuffer = NULL;
      m_metadataBufferSize = 0;
   }
}

// Fills all the CDLODQuadTree::SelectedNode instances with streamed data and metadata required for rendering
bool CDLODStreamingStorage::ConnectSelectionStreamingData( CDLODQuadTree::LODSelection & selection )
{
   // Have we already filled in the data? If so, that is not supported for simplicity reasons.
   assert( selection.GetStreamingDataHandler() == NULL );
   if( selection.GetStreamingDataHandler() != NULL ) 
      return false;

   // Currently not supporting multiple simultaneous connections
   assert( m_selectedNodeDataLinkBufferCount == 0 );

   selection.SetStreamingDataHandler( reinterpret_cast<uint32>(this) );

   CDLODQuadTree::SelectedNode * nodes = selection.GetSelection();
   for( int i = 0; i < selection.GetSelectionCount(); i++ )
   {
      CDLODQuadTree::SelectedNode & node = nodes[i];
      
      CDLODStreamingTypeHeightmap::HeightmapBlock * blockHeightmap = static_cast<CDLODStreamingTypeHeightmap::HeightmapBlock*>(
         m_heightmapDataHandler->FindBlockForQuad( node.X, node.Y, node.Size, node.LODLevel, false ) );

      // Early-out
      if( (blockHeightmap == NULL) || ((blockHeightmap->Flags & CDLODStreamingTypeBase::BF_Loaded) == 0) )
      {
         // mark as block not found
         node.StreamingDataID = -1;
         continue;
      }
      assert( (blockHeightmap->Flags & (CDLODStreamingTypeBase::BF_Loading | CDLODStreamingTypeBase::BF_LoadingFinished)) == 0 );

      // This is not needed, but we'll leave it in for a while for consistency/stability reasons.
      memset( &m_selectedNodeDataLinkBuffer[m_selectedNodeDataLinkBufferCount], 0, sizeof(SelectedNodeDataInfo) );

      SelectedNodeDataInfo & outNodeDataInfo = m_selectedNodeDataLinkBuffer[m_selectedNodeDataLinkBufferCount];
      node.StreamingDataID = m_selectedNodeDataLinkBufferCount;

      {
         int blockX, blockY, blockLODLevel, blockRasterX, blockRasterY, blockRasterSize;
         m_heightmapDataHandler->DecodeBlockHashID( blockHeightmap->HashID, blockX, blockY, blockLODLevel );
         m_heightmapDataHandler->CalcBlockRasterCoverage( blockX, blockY, blockLODLevel, blockRasterX, blockRasterY, blockRasterSize );

         int cornerDataStep = m_worldDesc.LeafQuadTreeNodeSize << blockLODLevel;

         int nodeRight = ::min( (int)(node.X + node.Size), m_worldDesc.RasterSizeX-1 );
         int nodeBottom = ::min( (int)(node.Y + node.Size), m_worldDesc.RasterSizeY-1 );

         assert( ((node.X - blockRasterX) % cornerDataStep) == 0 );
         assert( ((node.Y - blockRasterY) % cornerDataStep) == 0 );
         assert( ((node.X + node.Size - blockRasterX) % cornerDataStep) == 0 );
         assert( ((node.Y + node.Size - blockRasterY) % cornerDataStep) == 0 );
         int cx0 = (node.X - blockRasterX) / cornerDataStep;
         int cy0 = (node.Y - blockRasterY) / cornerDataStep;
         int cx1 = (nodeRight - blockRasterX) / cornerDataStep;
         int cy1 = (nodeBottom - blockRasterY) / cornerDataStep;
         assert( cx0 < blockHeightmap->CornerPointsSizeX );
         assert( cy0 < blockHeightmap->CornerPointsSizeY );
         assert( cx1 < blockHeightmap->CornerPointsSizeX );
         assert( cy1 < blockHeightmap->CornerPointsSizeY );

         outNodeDataInfo.HeightCornerTL = blockHeightmap->CornerPoints[ cx0 + cy0 * blockHeightmap->CornerPointsSizeX ];
         outNodeDataInfo.HeightCornerTR = blockHeightmap->CornerPoints[ cx1 + cy0 * blockHeightmap->CornerPointsSizeX ];
         outNodeDataInfo.HeightCornerBL = blockHeightmap->CornerPoints[ cx0 + cy1 * blockHeightmap->CornerPointsSizeX ];
         outNodeDataInfo.HeightCornerBR = blockHeightmap->CornerPoints[ cx1 + cy1 * blockHeightmap->CornerPointsSizeX ];
      }

      bool noParent = node.LODLevel == (GetLODLevelCount()-1);
      assert( node.LODLevel < GetLODLevelCount() );

      int parentNodeSize   = node.Size * 2;
      int parentNodeX      = (node.X / parentNodeSize) * parentNodeSize;
      int parentNodeY      = (node.Y / parentNodeSize) * parentNodeSize;

      CDLODStreamingTypeTextureBase::TextureBlock * blockNormalmap = NULL;
      if( m_normalmapDataHandler != NULL ) blockNormalmap = static_cast<CDLODStreamingTypeTextureBase::TextureBlock*>(
         m_normalmapDataHandler->FindBlockForQuad( node.X, node.Y, node.Size, node.LODLevel, false ) );
      if( (blockNormalmap != NULL) && ((blockNormalmap->Flags & CDLODStreamingTypeBase::BF_Loaded) == 0) )
         blockNormalmap = NULL;

      CDLODStreamingTypeTextureBase::TextureBlock * blockOverlaymap = NULL;
      if( m_overlaymapDataHandler != NULL ) blockOverlaymap = static_cast<CDLODStreamingTypeTextureBase::TextureBlock*>(
         m_overlaymapDataHandler->FindBlockForQuad( node.X, node.Y, node.Size, node.LODLevel, false ) );
      if( (blockOverlaymap != NULL) && ((blockOverlaymap->Flags & CDLODStreamingTypeBase::BF_Loaded) == 0) )
         blockOverlaymap = NULL;

      outNodeDataInfo.Heightmap     = blockHeightmap->Texture;
      outNodeDataInfo.HeightmapInfo = blockHeightmap->TextureInfo;

      if( blockNormalmap != NULL )
      {
         assert( (blockNormalmap->Flags & (CDLODStreamingTypeBase::BF_Loading | CDLODStreamingTypeBase::BF_LoadingFinished) ) == 0 );
         outNodeDataInfo.Normalmap     = blockNormalmap->Texture;
         outNodeDataInfo.NormalmapInfo = blockNormalmap->TextureInfo;

         outNodeDataInfo.ParentNormalmap = NULL;
         if( !noParent )
         {
            CDLODStreamingTypeTextureBase::TextureBlock * blockParentNormalmap = static_cast<CDLODStreamingTypeTextureBase::TextureBlock*>(
               m_normalmapDataHandler->FindBlockForQuad( parentNodeX, parentNodeY, parentNodeSize, node.LODLevel+1, false ) );
            
            if( (blockParentNormalmap != NULL) && ((blockParentNormalmap->Flags & CDLODStreamingTypeBase::BF_Loaded) != 0) )
            {
               outNodeDataInfo.ParentNormalmap     = blockParentNormalmap->Texture;
               outNodeDataInfo.ParentNormalmapInfo = blockParentNormalmap->TextureInfo;
            }
         }
      }

      // duplicated code (from normalmap above) - me doesn't like it not one bit, but will leave it in for now...
      if( blockOverlaymap != NULL )
      {
         assert( (blockOverlaymap->Flags & (CDLODStreamingTypeBase::BF_Loading | CDLODStreamingTypeBase::BF_LoadingFinished)) == 0 );
         outNodeDataInfo.Overlaymap    = blockOverlaymap->Texture;
         outNodeDataInfo.OverlaymapInfo= blockOverlaymap->TextureInfo;

         outNodeDataInfo.ParentOverlaymap = NULL;
         if( !noParent )
         {
            CDLODStreamingTypeTextureBase::TextureBlock * blockParentOverlaymap = static_cast<CDLODStreamingTypeTextureBase::TextureBlock*>(
               m_overlaymapDataHandler->FindBlockForQuad( parentNodeX, parentNodeY, parentNodeSize, node.LODLevel+1, false ) );

            if( (blockParentOverlaymap != NULL) && ((blockParentOverlaymap->Flags & CDLODStreamingTypeBase::BF_Loaded) != 0) )
            {
               outNodeDataInfo.ParentOverlaymap     = blockParentOverlaymap->Texture;
               outNodeDataInfo.ParentOverlaymapInfo = blockParentOverlaymap->TextureInfo;
            }
         }
      }

      m_selectedNodeDataLinkBufferCount++;
   }

   return true;
}

// Cleans the internal storage used by FillSelectionStreamingData() - must be called when finished rendering.
void CDLODStreamingStorage::FlushSelectionStreamingData( CDLODQuadTree::LODSelection & selection )
{
   // Must already be connected with us.
   assert( reinterpret_cast<uint32>(this) == selection.GetStreamingDataHandler() );

   selection.SetStreamingDataHandler( NULL );

   m_selectedNodeDataLinkBufferCount = 0;
}

HRESULT CDLODStreamingStorage::OnCreateDevice()
{ 
   return S_OK; 
};

HRESULT CDLODStreamingStorage::OnResetDevice(const D3DSURFACE_DESC* pBackBufferSurfaceDesc)
{ 
   // Recreate resources (since they all are in D3DPOOL_DEFAULT)
   // (will be restreamed in automatically after next Tick())

   return S_OK; 
}

void CDLODStreamingStorage::OnLostDevice()
{
   // Stop all streaming here, empty queues and destroy resources (since they all are in D3DPOOL_DEFAULT)

   if( m_IOManager != NULL )
   {
      m_IOManager->FreeAllObservedBlocks( true );
      m_IOManager->FinishAllStreaming();
      m_IOManager->UpdateStreaming(0.0f);
   }

   for( size_t i = 0; i < m_streamingDataHandlers.size(); i++ )
   {
      CDLODStreamingTypeBase * item = m_streamingDataHandlers[i];
      item->FreeAllBlocks();
   }
}

void CDLODStreamingStorage::OnDestroyDevice()
{
   OnLostDevice();
}

void CDLODStreamingStorage::Tick( float deltaTime )
{
   if( m_IOManager == NULL )
      return;

   m_IOManager->UpdateObservers( deltaTime );
   m_IOManager->UpdateStreaming( deltaTime );

   m_memoryUseInfo.StreamingThreadsMemory = m_IOManager->GetThreadsScratchMemorySize();

   if( m_heightmapDataHandler != NULL )
   {
      m_memoryUseInfo.HeightmapLoadedBlocks = m_heightmapDataHandler->GetCurrentlyAllocatedBlockCount();
      m_heightmapDataHandler->GetCurrentlyAllocatedMemory( m_memoryUseInfo.HeightmapTextureMemory, m_memoryUseInfo.HeightmapCacheMemory );
   }

   if( m_normalmapDataHandler != NULL )
   {
      m_memoryUseInfo.NormalmapLoadedBlocks = m_normalmapDataHandler->GetCurrentlyAllocatedBlockCount();
      m_normalmapDataHandler->GetCurrentlyAllocatedMemory( m_memoryUseInfo.NormalmapTextureMemory, m_memoryUseInfo.NormalmapCacheMemory );
   }
   
   if( m_overlaymapDataHandler != NULL )
   {
      m_memoryUseInfo.OverlaymapLoadedBlocks = m_overlaymapDataHandler->GetCurrentlyAllocatedBlockCount();
      m_overlaymapDataHandler->GetCurrentlyAllocatedMemory( m_memoryUseInfo.OverlaymapTextureMemory, m_memoryUseInfo.OverlaymapCacheMemory );
   }

}

bool CDLODStreamingStorage::IsCurrentlyStreaming()
{
   if( m_IOManager )
   {
      return m_IOManager->IsCurrentlyStreaming();
   }
   return false;
}

int CDLODStreamingStorage::GetHeightmapDataLODOffset( ) const
{ 
   assert( m_heightmapDataHandler != NULL );
   return m_heightmapDataHandler->GetLODOffset(); 
}