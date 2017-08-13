
///////////////////////////////////////////////////////////////////////////////////////////////////
// VertexAsylum Codebase, Copyright (c) Filip Strugar.
// Contents of this file are distributed under Zlib license.
// (http://en.wikipedia.org/wiki/Zlib_License)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// Types
#include "Core/Threading/vaThread.h"
#include "Core/Threading/vaCriticalSection.h"
#include "Core/Threading/vaSemaphore.h"

#include <stack>
#include <deque>
#include <vector>
#include <set>
#include <algorithm>

namespace VertexAsylum
{
   class vaThreadPool;

   struct vaThreadPoolJobInfo
   {
      vaThreadPool * ThreadPool;
      void *         LocalThreadScratchBuffer;
      void *         UserData0;
      void *         UserData1;
      uint32         JobID;
   };

   typedef void (*vaThreadPoolJobProcPtr)(const vaThreadPoolJobInfo & jobInfo);

   class vaThreadPool
   {
   public:
      static const uint32           c_invalidJobID    = (uint32)(-1);

   private:
      static const int32            c_maxThreads      = 16;
      static const int32            c_maxJobs         = 2048;

      enum JobFlag
      {
         JF_Pending  = (1 << 0),
         JF_Active   = (1 << 1),
      };

      struct vaThreadPoolJobPrivateInfo : public vaThreadPoolJobInfo
      {
         uint32                  Flags;
         vaThreadPoolJobProcPtr  JobProcPtr;
      };

      vaThread *                 m_threads[c_maxThreads];
      uint32                     m_threadCount;

      vaThreadPoolJobPrivateInfo m_jobs[c_maxJobs];
      int                        m_lastUsedJobID;              // starts at -1
      
      std::stack<uint32>         m_freeJobIDs;
      std::deque<uint32>         m_pendingJobs;
      std::set<uint32>           m_activeJobs;

      uint32                     m_localThreadScratchBufferSize;

      vaSemaphore                m_availableJobs;
      vaCriticalSection          m_containersAccessMutex;      // for m_pendingJobs, m_activeJobs, m_jobs, m_lastUsedJobID, m_freeJobIDs, m_shutdownSignal

      bool                       m_shutdownSignal;

   public:
      vaThreadPool( uint32 workerThreadCount, vaThread::ThreadPriority threadsPriority, uint32 stackSize = 256 * 1024, uint32 localThreadScratchBufferSize = 0 );
      virtual ~vaThreadPool();

      void                       AddJob( vaThreadPoolJobProcPtr jobProc, void * userData0, void * userData1, uint32 & outJobID );

      bool                       RemoveQueuedJob( uint32 jobID );

      void                       WaitFinishAllTasks();

      void                       GetCurrentJobCount( int & outPendingJobs, int & outActiveJobs );

      uint32                     GetLocalThreadScratchBufferSize() const      { return m_localThreadScratchBufferSize; }
      uint32                     GetThreadCount() const                       { return m_threadCount; }

   private:
      inline uint32              GetNewJobID();
      inline void                ReleaseJobID(uint32 jobID);

      void                       ThreadProc();
      static uint32              StaticThreadProc(void * pUserData);
   };

   uint32 vaThreadPool::GetNewJobID()
   {
      uint32 newJobID;
      if( m_freeJobIDs.size() != 0 )
      {
         newJobID = m_freeJobIDs.top();
         m_freeJobIDs.pop();
      }
      else
      {
         if( m_lastUsedJobID >= (c_maxJobs-1) )
         {
            assert( false ); // no more jobs available!
            return c_invalidJobID;
         }
         m_lastUsedJobID++;
         newJobID = m_lastUsedJobID;
      }

      // must be unused as we're reusing it
      assert( (m_jobs[newJobID].Flags & (JF_Active | JF_Pending)) == 0 );
      assert( newJobID != c_invalidJobID );
      return newJobID;
   }

   void vaThreadPool::ReleaseJobID(uint32 jobID)
   {
      // must be finished and not pending as we're releasing it
      assert( (m_jobs[jobID].Flags & (JF_Active | JF_Pending)) == 0 );

      if( (int32)jobID == m_lastUsedJobID )
      {
         do 
         {
            if( m_lastUsedJobID < 0 )
               break;

            m_lastUsedJobID--;
         } while( (m_jobs[m_lastUsedJobID].Flags & JF_Active | JF_Pending) == 0 );
      }
      else
      {
         m_freeJobIDs.push(jobID);
      }
   }

}
