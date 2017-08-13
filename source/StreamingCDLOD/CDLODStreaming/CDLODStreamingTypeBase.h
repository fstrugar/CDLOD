///////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "..\Common.h"

#include "..\CDLODQuadTree.h"

#include "Core/Threading/vaSemaphore.h"
#include "Core/Threading/vaThreadPool.h"

class CDLODStreamingStorage;
namespace VertexAsylum 
{ 
   class vaStream; 
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Base class representing a streaming data type (heightmap, normalmap, overlay image, etc...
///////////////////////////////////////////////////////////////////////////////////////////////////

class CDLODStreamingTypeBase
{
   static const int              s_hashBitsForArrayIndex = 4;
   static const int              s_hashBitsForLevel      = 4;
   static const int              s_hashBitsForDim        = 10;

public:

   struct ExportSettings
   {
      int LeafQuadTreeNodeSize;
      bool NormalmapUseDXT5NMCompression;
      bool OverlaymapUseDXT1Compression;
      bool UseZLIBCompression;
   };

   enum StorageTypeID
   {
      STID_Heightmap    = 0,
      STID_Normalmap    = 1,
      STID_Overlaymap   = 2,
   };

protected:
   friend class CDLODStreamingIOManager;

   enum BlockStateFlags
   {
      BF_Loading              = ( 1 << 0 ),
      BF_LoadingFinished      = ( 1 << 1 ),  // loading finished but waiting for finalize to become BF_Loaded
      BF_Loaded               = ( 1 << 2 ),
      BF_TouchedInTraversal   = ( 1 << 3 ),  // helper flag for use in CDLODStreamingIOManager::OnUpdateBounds and similar
   };

   struct Block
   {
      // This data is accessed only from the main thread (thread calling CDLODStreamingStorage::Update())
      uint32      HashID;
      uint32      Flags;
      int32       RefCount;
      uint32      StreamingJobID;

      virtual void   OnAllocate()     = 0;
      virtual void   OnDeallocate()   = 0;

      virtual void   GetWorldCoverage( AABB & boundingBox )    { boundingBox.Min = D3DXVECTOR3(0, 0, 0); boundingBox.Max = D3DXVECTOR3(0, 0, 0); }

      uint32         GetArrayIndex() const   
      { 
         return (HashID >> (s_hashBitsForDim*2 + s_hashBitsForLevel)) & ((1<<s_hashBitsForArrayIndex)-1);
      }
   };

   class BlockAllocatorInterface
   {
   public:
      virtual ~BlockAllocatorInterface() {}
      //
      virtual Block *	      AllocateBlock( )                 = 0;
      virtual void            FreeBlock( Block * pQuad )       = 0;
      virtual void            Clear( )                         = 0;
   };

private:

   struct BlockStorageMetadata
   {
      Block *     BlockPtr;
      int64       StoragePos;
      int64       StorageSize;

      void        Reset()   { BlockPtr = NULL; StoragePos = 0; StorageSize = 0; }
   };

   struct LODLevelData
   {
      int               BlocksX;
      int               BlocksY;

      BlockStorageMetadata **  Array;
   };

protected:
   int                           m_blockSize;
   int                           m_LODOffset;
   int                           m_LODCount;

   CDLODStreamingStorage * const  m_storage;
   BlockAllocatorInterface*const m_allocatorPtr;

   // Index in the CDLODStreamingStorage-s internal storage (should be renamed to something like ID)
   const int                     m_arrayIndex;
   
   int                           m_currentlyAllocatedBlocks;

private:
   LODLevelData                  m_LODLevels[CDLODQuadTree::c_maxLODLevels];

   // dummy assignment operator to prevent compiler from complaining
   CDLODStreamingTypeBase& operator = ( const CDLODStreamingTypeBase & other ) { assert(false); return *this; }

public:

   CDLODStreamingTypeBase( CDLODStreamingStorage * storage, BlockAllocatorInterface * allocatorPtr, int blockSize, int LODOffset, uint32 arrayIndex );
   CDLODStreamingTypeBase( CDLODStreamingStorage * storage, BlockAllocatorInterface * allocatorPtr, uint32 arrayIndex );
   virtual ~CDLODStreamingTypeBase( );

   inline void                   MapQuadToBlock( const int quadRasterX, const int quadRasterY, const int quadSize, const int quadLODLevel,
                                                         int & outBlockX, int & outBlockY, int & outBlockLODLevel ) const;

   inline Block*                 FindBlockForQuad( int quadRasterX, int quadRasterY, int quadSize, int quadLODLevel, bool createIfNotExisting );

   virtual void                  Clean( );

   void                          FreeAllBlocks( );

   virtual void                  Update( float fDeltaTime );

   int                           GetCurrentlyAllocatedBlockCount() { return m_currentlyAllocatedBlocks; }
   virtual void                  GetCurrentlyAllocatedMemory( uint32 & outDataMemorySize, uint32 & outCacheMemorySize ) = 0;

   virtual void                  ExportBlockForQuad( VertexAsylum::vaStream * outStream, CDLODStreamingTypeBase::Block * block, 
                                                      const int quadRasterX, const int quadRasterY, const int quadSize, const int quadLODLevel, 
                                                      const ExportSettings & exportSettings  ) = 0;
   
   virtual void                  SaveHeader( VertexAsylum::vaStream * outStream, bool justAllocatePass );
   virtual void                  LoadHeaderAndInitialize( VertexAsylum::vaStream * inStream );

   void                          LoadAllBlocksForTestingPurposes( VertexAsylum::vaStream * inStream );

   // called from the main thread when the streaming is initiated
   virtual bool                  StreamingInitialize( Block * block )   { return true; };
   // called from the streaming thread to do the actual streaming
   void                          StreamingExecute( VertexAsylum::vaStream * inStream, Block * block, VertexAsylum::vaSemaphore * streamSemaphore,
                                                void * scratchBuffer, uint32 scratchBufferSize );
   // called from the main thread when the streaming was finished
   virtual bool                  StreamingFinalize( Block * block )     { return true; };
   
   virtual StorageTypeID         GetStorageTypeID() = 0;
   uint32                        GetStorageArrayIndex() const     { return m_arrayIndex; }
   int                           GetLODOffset() const             { return m_LODOffset; }


protected:
   friend class CDLODStreamingStorage;
   inline uint32                 EncodeBlockHashID( int blockX, int blockY, int blockLODLevel ) const;
   static inline void            DecodeBlockHashID( uint32 hashID, int & outBlockX, int & outBlockY, int & outBlockLODLevel );
   inline uint32                 CalcBlockRasterCoverage( int blockX, int blockY, int blockLODLevel, int & outRasterX, int & outRasterY, int & outRasterSize ) const;
   inline Block*                 GetBlock( uint32 hashID );
   inline Block*                 GetBlock( int blockX, int blockY, int blockLODLevel );
   inline BlockStorageMetadata*  GetBlockMetadata( int blockX, int blockY, int blockLODLevel );
   inline Block*                 AllocateBlock( int blockX, int blockY, int blockLODLevel );
   inline void                   FreeBlock( Block* block );
   inline bool                   FreeBlock( int blockX, int blockY, int blockLODLevel );
   inline int                    GetBlockLevelSizeX( int blockLODLevel ) const;
   inline int                    GetBlockLevelSizeY( int blockLODLevel ) const;

   // Called from the streaming thread to process the buffer (after decompression, if there was any)
   virtual bool                  StreamingReadBlockData( byte * dataBuffer, int dataBufferSize, Block * block ) = 0;


   // Simple allocator that reuses recently freed elements. 
   // Some better (block-based?) allocator would probably be more suitable.
   template<class QuadType>
   class SimpleAllocator : public BlockAllocatorInterface
   {
   private:
      static const int           StackSize = 256;
      QuadType *                 blockStack[StackSize];
      int                        currentStackCount;

      int                        allocatedCount;
   public:
      SimpleAllocator()
      {
         currentStackCount = 0;
         allocatedCount = 0;
      }
      virtual ~SimpleAllocator()
      {
         Clear();
         assert( allocatedCount == 0 );
      }
      //
   public:
      // Override these if more optimized allocator is required (block-based, etc)
      virtual Block *	      AllocateBlock( )
      {
         allocatedCount++;
         Block * allocatedBlock;
         if( currentStackCount == 0 )
            allocatedBlock = static_cast<Block *>(new QuadType());
         else
            allocatedBlock = static_cast<Block *>(blockStack[--currentStackCount]);

         allocatedBlock->HashID           = 0;
         allocatedBlock->Flags            = 0;
         allocatedBlock->RefCount         = 0;
         allocatedBlock->StreamingJobID   = VertexAsylum::vaThreadPool::c_invalidJobID;
         allocatedBlock->OnAllocate();

         return allocatedBlock;
      }
      //
      virtual void            FreeBlock( Block * block )
      {
         allocatedCount--;

         assert( block->RefCount == 0 );

         block->OnDeallocate();
         
         if( currentStackCount >= StackSize )
            delete static_cast<QuadType *>(block);
         else
            blockStack[currentStackCount++] = static_cast<QuadType *>(block);
      }
      virtual void            Clear( )
      {
         for( int i = 0; i < currentStackCount; i++ )
            delete static_cast<QuadType *>(blockStack[i]);
         currentStackCount = 0;
      }
   };
};



///////////////////////////////////////////////////////////////////////////////////////////////////
// Inline definitions
///////////////////////////////////////////////////////////////////////////////////////////////////

inline void CDLODStreamingTypeBase::MapQuadToBlock( const int quadRasterX, const int quadRasterY, const int quadSize, const int quadLODLevel,
                                                      int & outBlockX, int & outBlockY, int & outBlockLODLevel ) const
{
   outBlockLODLevel = ::max( quadLODLevel - m_LODOffset, 0 );

   int blockSizeInL0Raster = m_blockSize << outBlockLODLevel;

   // If any of these is not satisfied the system will not work correctly (streaming blocks
   // will not correctly cover render quads)
   assert( quadSize <= blockSizeInL0Raster );

   outBlockX = quadRasterX / blockSizeInL0Raster;
   outBlockY = quadRasterY / blockSizeInL0Raster;

   assert( ((quadRasterX+quadSize-1) / blockSizeInL0Raster) == outBlockX );
   assert( ((quadRasterY+quadSize-1) / blockSizeInL0Raster) == outBlockY );
}

inline uint32 CDLODStreamingTypeBase::EncodeBlockHashID( int blockX, int blockY, int blockLODLevel ) const
{
   assert( (blockLODLevel < (1<<s_hashBitsForLevel)) && (blockX < (1<<s_hashBitsForDim)) && (blockY < (1<<s_hashBitsForDim)) );

   uint32 res = 0;

   res <<= s_hashBitsForArrayIndex;
   res = (res | GetStorageArrayIndex());

   res <<= s_hashBitsForLevel;
   res = (res | blockLODLevel);
   
   res <<= s_hashBitsForDim;
   res = (res | blockX);

   res <<= s_hashBitsForDim;
   res = (res | blockY);

   return res;
}

inline void CDLODStreamingTypeBase::DecodeBlockHashID( uint32 hashID, int & outBlockX, int & outBlockY, int & outBlockLODLevel )
{
   outBlockY = hashID & ((1 << s_hashBitsForDim)-1);
   hashID >>= s_hashBitsForDim;

   outBlockX = hashID & ((1 << s_hashBitsForDim)-1);
   hashID >>= s_hashBitsForDim;

   outBlockLODLevel = hashID & ((1 << s_hashBitsForLevel)-1);
   //hashID >>= s_hashBitsForLevel;
}

inline uint32 CDLODStreamingTypeBase::CalcBlockRasterCoverage( int blockX, int blockY, int blockLODLevel, int & outRasterX, int & outRasterY, int & outRasterSize ) const
{
   uint32 res = 0;

   outRasterSize = m_blockSize << blockLODLevel;
   outRasterX = blockX * outRasterSize;
   outRasterY = blockY * outRasterSize;

   return res;
}

inline CDLODStreamingTypeBase::Block * CDLODStreamingTypeBase::FindBlockForQuad( int quadRasterX, int quadRasterY, int quadSize, 
                                                                              int quadLODLevel, bool createIfNotExisting )
{
   int blockX, blockY, blockLODLevel;
   MapQuadToBlock( quadRasterX, quadRasterY, quadSize, quadLODLevel, blockX, blockY, blockLODLevel );
   
   CDLODStreamingTypeBase::Block * block = GetBlock( blockX, blockY, blockLODLevel );

   if( block != NULL )
      return block;

   if( !createIfNotExisting )
      return NULL;

   block = AllocateBlock( blockX, blockY, blockLODLevel );

   return block;
}

inline CDLODStreamingTypeBase::Block * CDLODStreamingTypeBase::GetBlock( uint32 hashID )
{
   int blockX, blockY, blockLODLevel;
   DecodeBlockHashID( hashID, blockX, blockY, blockLODLevel );

   return GetBlock( blockX, blockY, blockLODLevel );
}

inline CDLODStreamingTypeBase::Block * CDLODStreamingTypeBase::GetBlock( int blockX, int blockY, int blockLODLevel )
{
   assert( blockLODLevel >= 0 && blockLODLevel < m_LODCount );
   LODLevelData & level = m_LODLevels[blockLODLevel];

   assert( blockX >= 0 && blockX < level.BlocksX );
   assert( blockY >= 0 && blockY < level.BlocksY );
   return level.Array[blockY][blockX].BlockPtr;
}

inline CDLODStreamingTypeBase::BlockStorageMetadata * CDLODStreamingTypeBase::GetBlockMetadata( int blockX, int blockY, int blockLODLevel )
{
   assert( blockLODLevel >= 0 && blockLODLevel < m_LODCount );
   LODLevelData & level = m_LODLevels[blockLODLevel];

   assert( blockX >= 0 && blockX < level.BlocksX );
   assert( blockY >= 0 && blockY < level.BlocksY );
   return &level.Array[blockY][blockX];
}

inline CDLODStreamingTypeBase::Block * CDLODStreamingTypeBase::AllocateBlock( int blockX, int blockY, int blockLODLevel )
{
   assert( blockLODLevel >= 0 && blockLODLevel < m_LODCount );
   LODLevelData & level = m_LODLevels[blockLODLevel];

   assert( blockX >= 0 && blockX < level.BlocksX );
   assert( blockY >= 0 && blockY < level.BlocksY );

   assert( level.Array[blockY][blockX].BlockPtr == NULL );
   assert( m_allocatorPtr != NULL );

   level.Array[blockY][blockX].BlockPtr = m_allocatorPtr->AllocateBlock();
   level.Array[blockY][blockX].BlockPtr->HashID = EncodeBlockHashID( blockX, blockY, blockLODLevel );

   m_currentlyAllocatedBlocks++;

   return level.Array[blockY][blockX].BlockPtr;
}

inline void CDLODStreamingTypeBase::FreeBlock( Block * block )
{
   assert( m_currentlyAllocatedBlocks >= 0 );

   int blockX, blockY, blockLODLevel;
   DecodeBlockHashID( block->HashID, blockX, blockY, blockLODLevel );
   FreeBlock( blockX, blockY, blockLODLevel );
}
//
inline bool CDLODStreamingTypeBase::FreeBlock( int blockX, int blockY, int blockLODLevel )
{
   assert( blockLODLevel >= 0 && blockLODLevel < m_LODCount );
   LODLevelData & level = m_LODLevels[blockLODLevel];

   assert( blockX >= 0 && blockX < level.BlocksX );
   assert( blockY >= 0 && blockY < level.BlocksY );

   if( level.Array[blockY][blockX].BlockPtr == NULL )
      return false;

   m_currentlyAllocatedBlocks--;
   m_allocatorPtr->FreeBlock( level.Array[blockY][blockX].BlockPtr );
   level.Array[blockY][blockX].BlockPtr = NULL;

   return true;
}

inline int CDLODStreamingTypeBase::GetBlockLevelSizeX( int blockLODLevel ) const
{ 
   assert( blockLODLevel >= 0 && blockLODLevel < m_LODCount ); 
   return m_LODLevels[blockLODLevel].BlocksX;
}
inline int CDLODStreamingTypeBase::GetBlockLevelSizeY( int blockLODLevel ) const
{ 
   assert( blockLODLevel >= 0 && blockLODLevel < m_LODCount ); 
   return m_LODLevels[blockLODLevel].BlocksY;
}

