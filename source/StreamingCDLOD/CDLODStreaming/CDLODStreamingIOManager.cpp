//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "CDLODStreamingIOManager.h"
#include "CDLODStreamingStorage.h"

#include "..\DxCanvas.h"

#include "..\DemoCamera.h"

#include "..\CDLODTools.h"

#include "Core/IO/vaFileStream.h"
#include "Core/IO/vaMemoryStream.h"
#include "Core/Common/vaTempMemoryBuffer.h"
#include "Core/Compression/vaCompression.h"
#include "Core/Threading/vaThreadPool.h"

#ifdef MY_EXTENDED_STUFF
#include "../DxCanvas.h"
#endif

static const int c_ThreadCount               = 3;
static const int c_ThreadStackSize           = 256 * 1024;

CDLODStreamingIOManager::CDLODStreamingIOManager( CDLODStreamingStorage & storage, 
   std::vector<CDLODStreamingTypeBase*> & dataHandlers, int32 memoryPerThreadRequired )
: m_storage( storage ), m_dataHandlers( dataHandlers )
{
   m_timeFromLastUpdate = 1000.0f;

   m_minLODOffset = m_storage.GetLODLevelCount()-1;
   m_maxLODOffset = 0;

   for( size_t i = 0; i < m_dataHandlers.size(); i++ )
   {
      m_minLODOffset = ::min( m_dataHandlers[i]->GetLODOffset(), m_minLODOffset );
      m_maxLODOffset = ::max( m_dataHandlers[i]->GetLODOffset(), m_maxLODOffset );
   }

   m_scratchVisibleBlocks = new VisibleBlocksContainerType();
   m_scratchVisibleBlocks->reserve( c_visibleBlocksContainerInitialCapacity );

   m_threadPool = new VertexAsylum::vaThreadPool( c_ThreadCount, VertexAsylum::vaThread::TP_BelowNormal, c_ThreadStackSize, memoryPerThreadRequired );
}

CDLODStreamingIOManager::~CDLODStreamingIOManager()
{
   while( m_observers.size() != 0 )
   {
      RemoveObserver( m_observers[0]->ObserverID );
   }

   FinishAllStreaming();

   delete m_threadPool;

   delete m_scratchVisibleBlocks;
}

uint32 CDLODStreamingIOManager::GetThreadsScratchMemorySize()
{
   return m_threadPool->GetThreadCount() * m_threadPool->GetLocalThreadScratchBufferSize();
}

void CDLODStreamingIOManager::FreeAllObservedBlocks( bool resetObserverBounds )
{
   for( uint32 i = 0; i < m_observers.size(); i++ )
   {
      ObserverInfo * obsInfo = m_observers[i];
      ObsRemoveCurrentlyVisibleBlocks( *obsInfo, resetObserverBounds );
   }
}

uint32 CDLODStreamingIOManager::AddObserver(const D3DXVECTOR3 & pos, float maxViewRange, bool forceLoad)
{
   /*static_*/assert( sizeof(ObserverInfo*) == 4 );  // sorry, 64bit not supported: observerID needs to be changed as currently it's a simple 32bit pointer

   ObserverInfo * obsInfo = new ObserverInfo();

   uint32 observerID = reinterpret_cast<uint32>(obsInfo);

   obsInfo->ObserverID     = observerID;
   obsInfo->Position       = pos;
   obsInfo->MaxViewRange   = maxViewRange;
   obsInfo->VisibleBlocks  = new VisibleBlocksContainerType();
   obsInfo->VisibleBlocks->reserve( c_visibleBlocksContainerInitialCapacity );

   m_observers.push_back( obsInfo );

   ObsUpdateBounds( *obsInfo, forceLoad );

   return observerID;
}

void CDLODStreamingIOManager::UpdateObserver( uint32 observerID, const D3DXVECTOR3 & pos, float maxViewRange )
{
   ObserverInfo * obsInfo = reinterpret_cast<ObserverInfo*>(observerID);

   assert( obsInfo != NULL );

   obsInfo->Position       = pos;
   obsInfo->MaxViewRange   = maxViewRange;
}

bool CDLODStreamingIOManager::RemoveObserver( uint32 observerID )
{
   if( observerID == 0 )
   {
      assert( false );
      return false;
   }

   ObserverInfo * obsInfo = reinterpret_cast<ObserverInfo*>(observerID);

   std::vector<ObserverInfo*>::iterator it = std::find( m_observers.begin(), m_observers.end(), obsInfo );
   if( it == m_observers.end() )
   {
      assert( false );
   }
   else
   {
      m_observers.erase(it);
   }

   ObsRemoveCurrentlyVisibleBlocks( *obsInfo, true );
   delete obsInfo->VisibleBlocks;

   delete obsInfo;

   return true;
}

void CDLODStreamingIOManager::ObsRemoveCurrentlyVisibleBlocks( ObserverInfo & obsInfo, bool resetObserverBounds )
{
   for( uint32 i = 0; i < obsInfo.VisibleBlocks->size(); i++ )
   {
      CDLODStreamingTypeBase::Block * block = (*obsInfo.VisibleBlocks)[i];
      block->RefCount--;

      if( block->RefCount == 0 )
      {
         m_streamingMutex.Enter();
         if( ((block->Flags & CDLODStreamingTypeBase::BF_Loading) != 0) )
         {
            // we're streaming this block! can't delete now but try removing it from the vaThreadPool queue
            if( m_threadPool->RemoveQueuedJob(block->StreamingJobID) )
            {
               block->StreamingJobID = VertexAsylum::vaThreadPool::c_invalidJobID;
               block->Flags &= ~CDLODStreamingTypeBase::BF_Loading;
               block->Flags |= CDLODStreamingTypeBase::BF_LoadingFinished;
               m_finishedBlocks.push_back( block );
            }
            else
            {
               // it's currently being streamed in : nothing we can do, it will be deleted when it finishes
            }
         }
         else if( ((block->Flags & CDLODStreamingTypeBase::BF_Loaded) != 0) )
         {
            // we're not streaming this block, remove it

            assert( (block->Flags & CDLODStreamingTypeBase::BF_LoadingFinished) == 0 );
            assert( (block->Flags & CDLODStreamingTypeBase::BF_Loading) == 0 );   // we shouldn't be BF_Loading

            uint32 arrayIndex = block->GetArrayIndex();
            assert( arrayIndex < m_dataHandlers.size() );
            CDLODStreamingTypeBase & handler = *m_dataHandlers[arrayIndex];

            handler.FreeBlock( block );
         }
         m_streamingMutex.Leave();
      }

      if( resetObserverBounds )
      {
         obsInfo.BoundingSphereSize = 0.0f;
      }
   }
   ::erase( *obsInfo.VisibleBlocks );

}

void CDLODStreamingIOManager::ObsUpdateBounds( ObserverInfo & obsInfo, bool forceLoad )
{
   // This is the first pass of this algorithm: there are magic numbers and arbitrary values which are tuned to work
   // correctly, but there's no guarantees that it will behave in all situations
   // Really correct algorithm would handle each LOD level independently with their own bounding sphere and ranges.

   obsInfo.BoundingSpherePos = obsInfo.Position;
   obsInfo.LastUpdateMaxViewRange = obsInfo.MaxViewRange;

   // expand the streaming sphere by this number of zero LOD level ranges so we don't have to re-stream every
   // time we move (this should be improved by some smarter heuristics, but is currently only adding some additional
   // amount of surrounding data)
   float zeroLODViewRange = m_storage.GetQuadTree().GetLODVisRangeDistRatios()[m_minLODOffset] * obsInfo.MaxViewRange;

   obsInfo.BoundingSphereSize = obsInfo.MaxViewRange + zeroLODViewRange;

   // expand the selection range so that when we move out of the selection sphere there's still enough data
   // to render until the new data gets loaded
   const float zeroLODViewRangeExpandK = 1.5f;
   const float selectionFrom  = zeroLODViewRange * zeroLODViewRangeExpandK;
   const float selectionRange = obsInfo.MaxViewRange * 1.15f + selectionFrom;
   assert( selectionFrom > zeroLODViewRange );
   assert( selectionRange > obsInfo.BoundingSphereSize );

   CDLODQuadTree::LODSelectionOnStack<4096> cdlodSelection( obsInfo.BoundingSpherePos, 
                                                          selectionRange, 
                                                          NULL,
                                                          selectionFrom, 
                                                          m_minLODOffset, 
                                                          0.66f, CDLODQuadTree::LODSelection::IncludeAllNodesInRange );
   m_storage.GetQuadTree().LODSelect( &cdlodSelection );

   CDLODQuadTree::SelectedNode * nodes = cdlodSelection.GetSelection();

   assert( m_scratchVisibleBlocks->size() == 0 );

   for( size_t h = 0; h < m_dataHandlers.size(); h++ )
   {
      CDLODStreamingTypeBase & handler = *m_dataHandlers[h];


      for( int i = 0; i < cdlodSelection.GetSelectionCount(); i++ )
      {
         CDLODQuadTree::SelectedNode & node = nodes[i];

         // No need to get in this deep, blocks already covered by higher level nodes
         if( node.LODLevel < handler.GetLODOffset() )
            continue;

         CDLODStreamingTypeBase::Block * block = handler.FindBlockForQuad( node.X, node.Y, node.Size, node.LODLevel, true );

         if( (block->Flags & CDLODStreamingTypeBase::BF_TouchedInTraversal) != 0 )
            continue;

         block->RefCount++;
         block->Flags |= CDLODStreamingTypeBase::BF_TouchedInTraversal;  // _must_ clean these before exiting this function!
         m_scratchVisibleBlocks->push_back( block );

         if( (block->Flags & (CDLODStreamingTypeBase::BF_Loading | CDLODStreamingTypeBase::BF_LoadingFinished | CDLODStreamingTypeBase::BF_Loaded)) == 0 )
         {
            BeginStreamingBlockData( block );
         }
      }
   }

   // Unmark CDLODStreamingTypeBase::BF_TouchedInTraversal flags
   for( uint32 i = 0; i < m_scratchVisibleBlocks->size(); i++ )
   {
      CDLODStreamingTypeBase::Block * block = (*m_scratchVisibleBlocks)[i];
      block->Flags &= ~CDLODStreamingTypeBase::BF_TouchedInTraversal;
   }

   // Reduce references from the old list and remove if required
   ObsRemoveCurrentlyVisibleBlocks( obsInfo, false );
   
   // Swap in our new block list and put the old in m_scratchVisibleBlocks for use next time.
   ::swap( m_scratchVisibleBlocks, obsInfo.VisibleBlocks );
}

void CDLODStreamingIOManager::UpdateObservers( float deltaTime )
{
   m_timeFromLastUpdate += deltaTime;

   /*
   // debug display of all covered blocks
   #ifdef MY_EXTENDED_STUFF
   float dbgZMin = m_storage.GetWorldMapDims().MinZ - m_storage.GetWorldMapDims().SizeZ * 0.1f;
   float dbgZMax = m_storage.GetWorldMapDims().MinZ;
   uint32 brushColors[4] = { 0x20FF0000, 0x2000FF00, 0x200000FF, 0x20FFFF00 };

   for( uint32 i = 0; i < m_observers.size(); i++ )
   {
   ObserverInfo * obsInfo = m_observers[i];

   for( uint32 b = 0; b < obsInfo->VisibleBlocks->size(); b++ )
   {
   CDLODStreamingTypeBase::Block * block = (*obsInfo->VisibleBlocks)[b];

   uint32 arrayIndex = block->GetArrayIndex();
   uint32 brushColor = brushColors[arrayIndex % _countof(brushColors)];

   if( arrayIndex != 0 )
   continue;

   AABB blockBoundingBox;
   block->GetWorldCoverage( blockBoundingBox ); blockBoundingBox.Min.z = dbgZMin; blockBoundingBox.Max.z = dbgZMax;
   GetCanvas3D()->DrawBox( blockBoundingBox.Min, blockBoundingBox.Max, 0xFF000000, brushColor );
   }
   }
   #endif
   */

   // don't update too frequently
   const float obsUpdateFrequency = 0.2f;
   if( m_timeFromLastUpdate >= obsUpdateFrequency )
   {
      uint32 i;
      for( i = 0; i < m_observers.size(); i++ )
      {
         ObserverInfo * obsInfo = m_observers[i];

         // if we didn't go out of the bounding sphere there's nothing to do
         D3DXVECTOR3 diff = obsInfo->Position - obsInfo->BoundingSpherePos;
         if( 
            // moved out of coverage?
            (D3DXVec3Length(&diff) <= (obsInfo->BoundingSphereSize - obsInfo->MaxViewRange))
            // or resized? (above algorithm does not correctly handle resizing, and also this is needed to reduce memory usage if range reduced)
            && (::fabs(obsInfo->LastUpdateMaxViewRange - obsInfo->MaxViewRange) < obsInfo->LastUpdateMaxViewRange * 0.01f)
            )
            continue;

         ObsUpdateBounds( *obsInfo, false );

         // Early return: ensures only one obs is updated per frame as it can be slow
         break;
      }
      if( i == m_observers.size()-1) 
         m_timeFromLastUpdate = 0;
   }
}

void CDLODStreamingIOManager::UpdateStreaming( float deltaTime )
{
   m_streamingMutex.Enter();
   while( m_finishedBlocks.size() > 0 )
   {
      CDLODStreamingTypeBase::Block * block = m_finishedBlocks.back();
      m_finishedBlocks.pop_back();
      m_streamingMutex.Leave();

      assert( ((block->Flags & CDLODStreamingTypeBase::BF_LoadingFinished) != 0) && 
         ((block->Flags & (CDLODStreamingTypeBase::BF_Loaded | CDLODStreamingTypeBase::BF_Loading)) == 0) );

      block->Flags &= ~CDLODStreamingTypeBase::BF_LoadingFinished;
      block->Flags |= CDLODStreamingTypeBase::BF_Loaded;

      uint32 arrayIndex = block->GetArrayIndex();
      assert( arrayIndex < m_dataHandlers.size() );
      CDLODStreamingTypeBase & handler = *m_dataHandlers[arrayIndex];

      handler.StreamingFinalize( block );

      BlockMap::iterator it = m_streamingBlocks.find( block->HashID );
      assert( it != m_streamingBlocks.end() );
      m_streamingBlocks.erase(it);

      // Handle the case where during streaming block went out of range again and is not needed anymore
      if( block->RefCount == 0 )
      {
         handler.FreeBlock( block );
      }
      m_streamingMutex.Enter();
   }
   m_streamingMutex.Leave();
}

void CDLODStreamingIOManager::FinishAllStreaming()
{
   assert( m_threadPool != NULL );

   // Probably not needed, but for consistency - if any new blocks are to be added to streaming, they will be added now
   UpdateStreaming( 0.0f );

   m_threadPool->WaitFinishAllTasks();

   // Finalize streaming
   UpdateStreaming( 0.0f );
}

void CDLODStreamingIOManager::BeginStreamingBlockData( CDLODStreamingTypeBase::Block * block )
{
   assert( m_threadPool != NULL );

   uint32 arrayIndex = block->GetArrayIndex();
   assert( arrayIndex < m_dataHandlers.size() );
   CDLODStreamingTypeBase & handler = *m_dataHandlers[arrayIndex];

   bool retVal = handler.StreamingInitialize( block );
   assert( retVal );

   if( !retVal )
      return;

   m_streamingMutex.Enter();
   block->Flags |= CDLODStreamingTypeBase::BF_Loading;
   m_streamingBlocks.insert( BlockMapPair(block->HashID, block) );
   m_threadPool->AddJob( JobProc, this, block, block->StreamingJobID );
   assert( block->StreamingJobID != VertexAsylum::vaThreadPool::c_invalidJobID );
   m_streamingMutex.Leave();
}

void CDLODStreamingIOManager::StreamingExecute( CDLODStreamingTypeBase::Block * block, void * scratchBuffer, uint32 scratchBufferSize )
{
   uint32 arrayIndex = block->GetArrayIndex();
   assert( arrayIndex < m_dataHandlers.size() );
   m_streamingMutex.Enter();
   CDLODStreamingTypeBase & handler = *m_dataHandlers[arrayIndex];
   m_streamingMutex.Leave();

   handler.StreamingExecute( m_storage.GetDataStream(), block, &m_storage.GetDataStreamSemaphore(), scratchBuffer, scratchBufferSize );

   m_streamingMutex.Enter();
   block->StreamingJobID = VertexAsylum::vaThreadPool::c_invalidJobID;
   block->Flags &= ~CDLODStreamingTypeBase::BF_Loading;
   block->Flags |= CDLODStreamingTypeBase::BF_LoadingFinished;
   m_finishedBlocks.push_back( block );
   m_streamingMutex.Leave();
}

void CDLODStreamingIOManager::JobProc( const VertexAsylum::vaThreadPoolJobInfo & jobInfo )
{
   CDLODStreamingIOManager * thisPtr = static_cast<CDLODStreamingIOManager *>(jobInfo.UserData0);
   CDLODStreamingTypeBase::Block * block  = static_cast<CDLODStreamingTypeBase::Block *>(jobInfo.UserData1);

   thisPtr->StreamingExecute( block, jobInfo.LocalThreadScratchBuffer, jobInfo.ThreadPool->GetLocalThreadScratchBufferSize() );
}

bool CDLODStreamingIOManager::IsCurrentlyStreaming()
{
   int pendingJobs, activeJobs;
   m_threadPool->GetCurrentJobCount( pendingJobs, activeJobs );

   return (pendingJobs > 0) && (activeJobs > 0);
}