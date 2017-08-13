
///////////////////////////////////////////////////////////////////////////////////////////////////
// VertexAsylum Codebase, Copyright (c) Filip Strugar.
// Contents of this file are distributed under Zlib license.
// (http://en.wikipedia.org/wiki/Zlib_License)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// Types
#include "Core/Threading/vaThreadPool.h"
#include "Core/Threading/vaThreading.h"

using namespace VertexAsylum;


vaThreadPool::vaThreadPool( uint32 workerThreadCount, vaThread::ThreadPriority threadsPriority, uint32 stackSize, uint32 localThreadScratchBufferSize )
 : m_availableJobs( 0, c_maxJobs*2 ), m_localThreadScratchBufferSize(localThreadScratchBufferSize)
{
   m_lastUsedJobID   = -1;
   for( int i = 0; i < _countof(m_jobs); i++ )
   {
      m_jobs[i].Flags = 0;
      m_jobs[i].JobID = i;
      m_jobs[i].LocalThreadScratchBuffer = 0;
      m_jobs[i].ThreadPool = this;
      m_jobs[i].UserData0 = NULL;
      m_jobs[i].UserData1 = NULL;
   }

   for( int i = 0; i < _countof(m_threads); i++ )
   {
      m_threads[i] = NULL;
   }

   {
      vaCriticalSectionRAIILock lock(m_containersAccessMutex);
      m_shutdownSignal = false;
   }

   m_threadCount = workerThreadCount;
   for( uint32 i = 0; i < m_threadCount; i++ )
   {
      m_threads[i] = vaThread::Create( stackSize, StaticThreadProc, this );
      m_threads[i]->SetPriority( threadsPriority );
      m_threads[i]->Resume();
   }
}

vaThreadPool::~vaThreadPool()
{
   WaitFinishAllTasks();
   
   {
      vaCriticalSectionRAIILock lock(m_containersAccessMutex);
      m_shutdownSignal = true;
   }

   m_availableJobs.Release(m_threadCount);

   for( uint32 i = 0; i < m_threadCount; i++ )
   {
      m_threads[i]->WaitExit();
      vaThread::Destroy( m_threads[i] );
   }
  
}

void vaThreadPool::AddJob( vaThreadPoolJobProcPtr jobProc, void * userData0, void * userData1, uint32 & outJobID )
{
   {
      vaCriticalSectionRAIILock lock(m_containersAccessMutex);

      outJobID = GetNewJobID();

      if( outJobID == c_invalidJobID )
         return;

      m_jobs[outJobID].JobProcPtr               = jobProc;
      m_jobs[outJobID].UserData0                = userData0;
      m_jobs[outJobID].UserData1                = userData1;
      m_jobs[outJobID].LocalThreadScratchBuffer = 0;

      assert( (m_jobs[outJobID].Flags & (JF_Pending | JF_Active)) == 0 );
      m_jobs[outJobID].Flags |= JF_Pending;

      m_pendingJobs.push_back( outJobID );
   }

   // Added new job, release a thread
   m_availableJobs.Release();
}

bool vaThreadPool::RemoveQueuedJob( uint32 jobID )
{
   assert( jobID != c_invalidJobID );

   vaCriticalSectionRAIILock lock(m_containersAccessMutex);

   std::deque<uint32>::iterator it = std::find( m_pendingJobs.begin(), m_pendingJobs.end(), jobID );
   if( it == m_pendingJobs.end() )
   {
      // not found
      return false;
   }
   else
   {
      // found, remove it
      *it = c_invalidJobID;
      return true;
   }
}

void vaThreadPool::WaitFinishAllTasks()
{
   for( ;; )
   {
      {
         vaCriticalSectionRAIILock lock(m_containersAccessMutex);
         if( m_pendingJobs.size() == 0 && m_activeJobs.size() == 0 )
            return;
      }
      vaThreading::Sleep(10);
   }
}

void vaThreadPool::ThreadProc()
{
   byte * threadScratchBuffer = new byte[m_localThreadScratchBufferSize];

   for( ;; )
   {
      m_availableJobs.WaitOne();

      m_containersAccessMutex.Enter();
      
      if( m_shutdownSignal )
      {
         m_containersAccessMutex.Leave();
         break; // exits the thread
      }

      while( m_pendingJobs.size() != 0 )
      {
         uint32 jobID;
         jobID = m_pendingJobs.front();
         m_pendingJobs.pop_front();

         // Allowed to have c_invalidJobID-s in the queue (for example, a job was canceled)
         if( jobID == c_invalidJobID )
            continue;

         vaThreadPoolJobPrivateInfo * jobInfo;

         m_activeJobs.insert(jobID);
         //m_activeJobs.push_back(jobID);
         jobInfo = &m_jobs[jobID];
         assert( jobID == jobInfo->JobID );
         jobInfo->LocalThreadScratchBuffer = threadScratchBuffer;

         assert( (jobInfo->Flags & JF_Pending) != 0 );
         assert( (jobInfo->Flags & JF_Active) == 0 );
         jobInfo->Flags &= ~JF_Pending;
         jobInfo->Flags |= JF_Active;

         vaThreadPoolJobInfo localJobInfo = *static_cast<vaThreadPoolJobInfo*>(jobInfo);

         m_containersAccessMutex.Leave();

         // Do the job!
         jobInfo->JobProcPtr( localJobInfo );

         m_containersAccessMutex.Enter();

         // std::vector<uint32>::iterator it = std::find( m_activeJobs.begin(), m_activeJobs.end(), jobInfo->JobID );
         std::set<uint32>::iterator it = m_activeJobs.find(jobInfo->JobID);
         assert( it != m_activeJobs.end() ); // job must be in, we've just finished it?
         if( it != m_activeJobs.end() )
         {
            m_activeJobs.erase( it );
         }

         assert( (jobInfo->Flags & JF_Active) != 0 );
         jobInfo->Flags &= ~JF_Active;
         ReleaseJobID(jobInfo->JobID);
      }
      m_containersAccessMutex.Leave();
   }

   delete[] threadScratchBuffer;
}

uint32 vaThreadPool::StaticThreadProc(void * pUserData)
{
   vaThreadPool * thisPtr = static_cast<vaThreadPool*>(pUserData);
   thisPtr->ThreadProc();
   return 0;
}

void vaThreadPool::GetCurrentJobCount( int & outPendingJobs, int & outActiveJobs )
{
   vaCriticalSectionRAIILock lock(m_containersAccessMutex);

   outPendingJobs = m_pendingJobs.size();
   outActiveJobs = m_activeJobs.size();
}