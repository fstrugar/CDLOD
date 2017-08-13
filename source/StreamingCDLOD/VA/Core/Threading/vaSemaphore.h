
///////////////////////////////////////////////////////////////////////////////////////////////////
// VertexAsylum Codebase, Copyright (c) Filip Strugar.
// Contents of this file are distributed under Zlib license.
// (http://en.wikipedia.org/wiki/Zlib_License)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// Types
#include "Core/Threading/vaThreadingTypes.h"

namespace VertexAsylum
{

   class vaSemaphore
   {
      vaThreadingSyncObjectHandle     m_semaphore;

   public:
      inline vaSemaphore( const int32 initialCount, const int32 maxCount );
      inline ~vaSemaphore();

      inline void                         Release( uint32 releaseCount = 1, uint32 * releasedCount = NULL );
      inline ThreadingWaitResponseType    WaitOne( uint32 waitTimeout = 0xFFFFFFFF );

      inline vaThreadingSyncObjectHandle  GetSyncObjectHandle( )  { return m_semaphore; }

      //this should go somewhere
      //inline static void   WaitAll( vaThreadingSyncObjectHandle[] semaphores, const int semaphoresCount );
      //inline static void   WaitAny( vaThreadingSyncObjectHandle[] semaphores, const int semaphoresCount );
   };

}

// Implementation
#include "Core/Threading/vaSemaphorePlatformSpecific.h"
