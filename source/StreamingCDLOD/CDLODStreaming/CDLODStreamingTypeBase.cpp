//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////


#include "CDLODStreamingTypeBase.h"

#include "CDLODStreamingStorage.h"

#include "Core/IO/vaStream.h"
#include "Core/IO/vaMemoryStream.h"
#include "Core/Common/vaTempMemoryBuffer.h"
#include "Core/Compression/vaCompression.h"


CDLODStreamingTypeBase::CDLODStreamingTypeBase( CDLODStreamingStorage * storage, BlockAllocatorInterface * allocatorPtr, int blockSize, 
                                             int LODOffset, uint32 arrayIndex )
 : m_storage( storage ), m_allocatorPtr( allocatorPtr ), m_blockSize( blockSize ), 
   m_LODOffset( LODOffset ), m_arrayIndex( arrayIndex ), m_LODCount( storage->GetLODLevelCount() - m_LODOffset ),
   m_currentlyAllocatedBlocks( 0 )
{ 
   assert( m_LODCount > 0 );

   for( int i = 0; i < _countof(m_LODLevels); i++ )
   {
      m_LODLevels[i].Array = NULL;
      m_LODLevels[i].BlocksX = 0;
      m_LODLevels[i].BlocksY = 0;
   }

   for( int i = 0; i < m_LODCount; i++ )
   {
      int levelBlockSize = m_blockSize << i;
      m_LODLevels[i].BlocksX = (storage->GetWorldDesc().RasterSizeX-1+levelBlockSize-1) / levelBlockSize;
      m_LODLevels[i].BlocksY = (storage->GetWorldDesc().RasterSizeY-1+levelBlockSize-1) / levelBlockSize;

      m_LODLevels[i].Array = new BlockStorageMetadata*[m_LODLevels[i].BlocksY];
      for( int y = 0; y < m_LODLevels[i].BlocksY; y++ )
      {
         m_LODLevels[i].Array[y] = new BlockStorageMetadata[m_LODLevels[i].BlocksX];
         for( int x = 0; x < m_LODLevels[i].BlocksX; x++ )
         {
            m_LODLevels[i].Array[y][x].Reset();
         }
      }
   }
}

CDLODStreamingTypeBase::CDLODStreamingTypeBase( CDLODStreamingStorage * storage, BlockAllocatorInterface * allocatorPtr, uint32 arrayIndex )
 : m_storage( storage ), m_allocatorPtr( allocatorPtr ), m_arrayIndex( arrayIndex ), m_blockSize( 0 ),
   m_LODOffset( 0 ), m_LODCount( 0 ), m_currentlyAllocatedBlocks( 0 )
{
   for( int i = 0; i < _countof(m_LODLevels); i++ )
   {
      m_LODLevels[i].Array = NULL;
      m_LODLevels[i].BlocksX = 0;
      m_LODLevels[i].BlocksY = 0;
   }
}

CDLODStreamingTypeBase::~CDLODStreamingTypeBase( )
{
   assert( m_LODCount == 0 );
   assert( m_currentlyAllocatedBlocks == 0 );
}

void CDLODStreamingTypeBase::Clean( )
{
   FreeAllBlocks();

   for( int i = 0; i < m_LODCount; i++ )
   {
      for( int y = 0; y < m_LODLevels[i].BlocksY; y++ )
      {
         for( int x = 0; x < m_LODLevels[i].BlocksX; x++ )
         {
            assert( m_LODLevels[i].Array[y][x].BlockPtr == NULL );
         }
         delete[] m_LODLevels[i].Array[y];
      }
      delete[] m_LODLevels[i].Array;

      m_LODLevels[i].BlocksX = 0;
      m_LODLevels[i].BlocksY = 0;
   }
   m_LODCount = 0;
}

void CDLODStreamingTypeBase::FreeAllBlocks( )
{
   for( int i = 0; i < m_LODCount; i++ )
   {
      for( int y = 0; y < GetBlockLevelSizeY(i); y++ )
      {
         for( int x = 0; x < GetBlockLevelSizeX(i); x++ )
         {
            FreeBlock( x, y, i );
         }
      }
   }
}

void CDLODStreamingTypeBase::SaveHeader( VertexAsylum::vaStream * outStream, bool justAllocatePass )
{
   // Version
   if( !outStream->WriteValue<int32>(1) )
      vaFatalError( "Unable to save CDLODStreamingTypeBase header" );

   if( !outStream->WriteValue<int32>(m_blockSize) )
      vaFatalError( "Unable to save CDLODStreamingTypeBase header" );

   if( !outStream->WriteValue<int32>(m_LODOffset) )
      vaFatalError( "Unable to save CDLODStreamingTypeBase header" );

   if( !outStream->WriteValue<int32>(m_LODCount) )
      vaFatalError( "Unable to save CDLODStreamingTypeBase header" );

   for( int i = 0; i < m_LODCount; i++ )
   {
      LODLevelData & level = m_LODLevels[i];

      if( !outStream->WriteValue<int32>(level.BlocksX) )
         vaFatalError( "Unable to save CDLODStreamingTypeBase header" );
      if( !outStream->WriteValue<int32>(level.BlocksY) )
         vaFatalError( "Unable to save CDLODStreamingTypeBase header" );

      for( int y = 0; y < level.BlocksY; y++ )
      {
         for( int x = 0; x < level.BlocksX; x++ )
         {
            assert( level.Array[y][x].BlockPtr == NULL );
            if( justAllocatePass )
            {
               // should be empty now
               assert( level.Array[y][x].StoragePos == 0 );
               assert( level.Array[y][x].StorageSize == 0 );
            }
            else
            {
               // should be filled in now
               assert( level.Array[y][x].StoragePos != 0 );
               assert( level.Array[y][x].StorageSize != 0 );
            }

         }
         int arrayXSize = level.BlocksX * sizeof( level.Array[y][0] );

         if( outStream->Write( level.Array[y], arrayXSize ) != arrayXSize )
            vaFatalError( "Unable to save CDLODStreamingTypeBase header" );
      }
   }
}

void CDLODStreamingTypeBase::LoadHeaderAndInitialize( VertexAsylum::vaStream * inStream )
{
   int version;
   if( !inStream->ReadValue<int32>(version) )
      vaFatalError( "Unable to load CDLODStreamingTypeBase header" );

   if( version != 1 )
      vaFatalError( "Unable to load CDLODStreamingTypeBase header - invalid version" );

   if( !inStream->ReadValue<int32>(m_blockSize) )
      vaFatalError( "Unable to load CDLODStreamingTypeBase header" );

   if( !inStream->ReadValue<int32>(m_LODOffset) )
      vaFatalError( "Unable to load CDLODStreamingTypeBase header" );

   assert( m_LODCount == 0 );
   if( !inStream->ReadValue<int32>(m_LODCount) )
      vaFatalError( "Unable to load CDLODStreamingTypeBase header" );

   for( int i = 0; i < m_LODCount; i++ )
   {
      LODLevelData & level = m_LODLevels[i];

      assert( level.BlocksX == 0 );
      assert( level.BlocksY == 0 );

      if( !inStream->ReadValue<int32>(level.BlocksX) )
         vaFatalError( "Unable to save CDLODStreamingTypeBase header" );
      if( !inStream->ReadValue<int32>(level.BlocksY) )
         vaFatalError( "Unable to save CDLODStreamingTypeBase header" );

      m_LODLevels[i].Array = new BlockStorageMetadata*[m_LODLevels[i].BlocksY];

      for( int y = 0; y < level.BlocksY; y++ )
      {
         m_LODLevels[i].Array[y] = new BlockStorageMetadata[m_LODLevels[i].BlocksX];

         int arrayXSize = level.BlocksX * sizeof( level.Array[y][0] );
         if( inStream->Read( level.Array[y], arrayXSize ) != arrayXSize )
            vaFatalError( "Unable to load CDLODStreamingTypeBase header" );

         for( int x = 0; x < level.BlocksX; x++ )
         {
            assert( level.Array[y][x].BlockPtr == NULL );
         }
      }
   }
}

void CDLODStreamingTypeBase::Update( float fDeltaTime )
{

}

void CDLODStreamingTypeBase::LoadAllBlocksForTestingPurposes( VertexAsylum::vaStream * inStream )
{
   for( int i = 0; i < m_LODCount; i++ )
   {
      LODLevelData & level = m_LODLevels[i];
      for( int y = 0; y < level.BlocksY; y++ )
      {
         for( int x = 0; x < level.BlocksX; x++ )
         {
            assert( level.Array[y][x].BlockPtr == NULL );
            assert( level.Array[y][x].StoragePos != 0 );
            assert( level.Array[y][x].StorageSize != 0 );
            AllocateBlock( x, y, i );

            inStream->Seek( level.Array[y][x].StoragePos );

            static VertexAsylum::vaTempMemoryBuffer tempBufferStorage;
            int tempBufferSize = (int)level.Array[y][x].StorageSize;

            byte * tempBuffer = tempBufferStorage.GetBuffer( tempBufferSize );

            if( inStream->Read( tempBuffer, tempBufferSize ) != tempBufferSize )
            {
               vaFatalError("Unable to read block");
            }

            static VertexAsylum::vaTempMemoryBuffer tempBufferCompStorage;
            int tempCompBufferSize = VertexAsylum::vaCompression::SimpleUnpackJustGetSize( tempBuffer, tempBufferSize );

            byte * tempCompBuffer = tempBufferCompStorage.GetBuffer( tempCompBufferSize );
            if( !VertexAsylum::vaCompression::SimpleUnpack( tempBuffer, tempBufferSize, tempCompBuffer, tempCompBufferSize ) )
            {
               vaFatalError("Unable to unpack block");
            }

            StreamingReadBlockData( tempCompBuffer, tempCompBufferSize, level.Array[y][x].BlockPtr );
         }
      }
   }
}

void CDLODStreamingTypeBase::StreamingExecute( VertexAsylum::vaStream * inStream, Block * block, VertexAsylum::vaSemaphore * streamSemaphore,
                                             void * scratchBuffer, uint32 scratchBufferSize )
{
   int blockX, blockY, blockLODLevel;
   DecodeBlockHashID( block->HashID, blockX, blockY, blockLODLevel );

   BlockStorageMetadata & blockMetadata = *GetBlockMetadata( blockX, blockY, blockLODLevel );

   int loadBufferSize = (int)blockMetadata.StorageSize;
   
   assert( ((int)scratchBufferSize) >= loadBufferSize ); 
   if( ((int)scratchBufferSize) < loadBufferSize )
      return;

   byte * loadBuffer = (byte*)scratchBuffer;

   streamSemaphore->WaitOne();
   inStream->Seek( blockMetadata.StoragePos );
   if( inStream->Read( loadBuffer, loadBufferSize ) != loadBufferSize )
   {
      vaFatalError("Unable to read block");
   }
   streamSemaphore->Release();

   int unpackBufferSize = VertexAsylum::vaCompression::SimpleUnpackJustGetSize( loadBuffer, loadBufferSize );

   assert( ((int)scratchBufferSize) >= (loadBufferSize+unpackBufferSize) ); 
   if( (int)scratchBufferSize < (loadBufferSize+unpackBufferSize) )
      return;

   static int s_maxBufferSize = 0;
   s_maxBufferSize = ::max( s_maxBufferSize, (loadBufferSize+unpackBufferSize) );

   byte * unpackBuffer = (loadBuffer + loadBufferSize);
   if( !VertexAsylum::vaCompression::SimpleUnpack( loadBuffer, loadBufferSize, unpackBuffer, unpackBufferSize ) )
   {
      vaFatalError("Unable to unpack block");
   }

   StreamingReadBlockData( unpackBuffer, unpackBufferSize, block );
}