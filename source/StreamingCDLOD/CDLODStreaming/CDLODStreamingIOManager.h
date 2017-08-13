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

#include "Core/Threading/vaCriticalSection.h"

#include <map>
#include <vector>

class CDLODStreamingStorage;
namespace VertexAsylum
{ 
   class vaThreadPool; 
   struct vaThreadPoolJobInfo; 
}

class CDLODStreamingIOManager
{
   static const int                 c_visibleBlocksContainerInitialCapacity = 512;
   typedef std::vector<CDLODStreamingTypeBase::Block *> VisibleBlocksContainerType;

private:

   // This should be a class in a separate file...
   struct ObserverInfo
   {
      uint32                        ObserverID;

      D3DXVECTOR3                   Position;
      float                         MaxViewRange;

      // Currently required blocks area & info
      D3DXVECTOR3                   BoundingSpherePos;
      float                         BoundingSphereSize;
      float                         LastUpdateMaxViewRange;

      // List of current potentially visible blocks (those that will be streamed in)
      VisibleBlocksContainerType *  VisibleBlocks;

      ObserverInfo() : Position(0.0f, 0.0f, 0.0f), MaxViewRange(0.0f), 
                        BoundingSpherePos(0.0f, 0.0f, 0.0f), BoundingSphereSize(0.0f), VisibleBlocks(0) { }
   };

   typedef std::map<uint32, CDLODStreamingTypeBase::Block*>   BlockMap;
   typedef std::pair<uint32, CDLODStreamingTypeBase::Block*>  BlockMapPair;

private:

   CDLODStreamingStorage &          m_storage;
   std::vector<CDLODStreamingTypeBase*> & m_dataHandlers;

   std::vector<ObserverInfo *>      m_observers;

   float                            m_timeFromLastUpdate;

   int                              m_minLODOffset;
   int                              m_maxLODOffset;
   
   VisibleBlocksContainerType *     m_scratchVisibleBlocks;

   VertexAsylum::vaThreadPool *     m_threadPool;

   // currently streaming blocks
   BlockMap                         m_streamingBlocks;
   
   // currently streaming blocks that have finished multithreaded work and are waiting to be enabled for rendering
   std::vector<CDLODStreamingTypeBase::Block*>
                                    m_finishedBlocks;
   VertexAsylum::vaCriticalSection  m_streamingMutex;

private:
   // dummy assignment operator to prevent compiler from complaining
   CDLODStreamingIOManager& operator = ( const CDLODStreamingIOManager & other ) { assert(false); return *this; }

public:
   CDLODStreamingIOManager( CDLODStreamingStorage & storage, std::vector<CDLODStreamingTypeBase*> & dataHandlers, int32 memoryPerThreadRequired );
   ~CDLODStreamingIOManager();

   uint32                  AddObserver( const D3DXVECTOR3 & pos, float maxViewRange, bool forceLoad );
   void                    UpdateObserver( uint32 observerID, const D3DXVECTOR3 & pos, float maxViewRange );
   bool                    RemoveObserver( uint32 observerID );

   void                    FreeAllObservedBlocks( bool resetObserverBounds );

   void                    UpdateObservers( float deltaTime );
   void                    UpdateStreaming( float deltaTime );

   bool                    IsCurrentlyStreaming();
   void                    FinishAllStreaming();

   uint32                  GetThreadsScratchMemorySize();

private:
   void                    ObsUpdateBounds( ObserverInfo & obsInfo, bool forceLoad );
   void                    ObsRemoveCurrentlyVisibleBlocks( ObserverInfo & obsInfo, bool resetObserverBounds );

   void                    BeginStreamingBlockData( CDLODStreamingTypeBase::Block * block );

   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // Thread-safe
   void                    StreamingExecute( CDLODStreamingTypeBase::Block * block, void * scratchBuffer, uint32 scratchBufferSize );
   static void             JobProc( const VertexAsylum::vaThreadPoolJobInfo & jobInfo );
   ///////////////////////////////////////////////////////////////////////////////////////////////////
};

