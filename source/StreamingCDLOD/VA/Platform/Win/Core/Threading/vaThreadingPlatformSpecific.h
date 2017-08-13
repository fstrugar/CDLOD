///////////////////////////////////////////////////////////////////////////////////////////////////
// VertexAsylum Codebase, Copyright (c) Filip Strugar.
// Contents of this file are distributed under Zlib license.
// (http://en.wikipedia.org/wiki/Zlib_License)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace VertexAsylum
{
   inline void vaThreading::Sleep( uint32 milliseconds )
   {
      ::Sleep( milliseconds );
   }

   inline ThreadingWaitResponseType vaThreading::WaitAll( vaThreadingSyncObjectHandle handles[], int32 handleCount, uint32 waitTimeout )
   {
      DWORD dwWaitResult = WaitForMultipleObjectsEx( handleCount, handles, true, waitTimeout, false );
      if( dwWaitResult == WAIT_FAILED )   return TWRT_Failed;
      if( dwWaitResult == WAIT_TIMEOUT )  return TWRT_Timeout;
      return (ThreadingWaitResponseType)((dwWaitResult - WAIT_OBJECT_0) + TWRT_Signaled_0);
   }

   inline ThreadingWaitResponseType vaThreading::WaitAny( vaThreadingSyncObjectHandle handles[], int32 handleCount, uint32 waitTimeout )
   {
      DWORD dwWaitResult = WaitForMultipleObjectsEx( handleCount, handles, false, waitTimeout, false );
      if( dwWaitResult == WAIT_FAILED )   return TWRT_Failed;
      if( dwWaitResult == WAIT_TIMEOUT )  return TWRT_Timeout;
      return (ThreadingWaitResponseType)((dwWaitResult - WAIT_OBJECT_0) + TWRT_Signaled_0);
   }

   //inline static void vaSemaphore::WaitAll( vaSemaphore[] semaphores, const int semaphoresCount );
   //inline static void vaSemaphore::WaitAny( vaSemaphore[] semaphores, const int semaphoresCount );

}