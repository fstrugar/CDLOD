///////////////////////////////////////////////////////////////////////////////////////////////////
// VertexAsylum Codebase, Copyright (c) Filip Strugar.
// Contents of this file are distributed under Zlib license.
// (http://en.wikipedia.org/wiki/Zlib_License)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace VertexAsylum
{

   inline vaSemaphore::vaSemaphore( const int32 initialCount, const int32 maxCount )
   {
      m_semaphore = CreateSemaphore( NULL, initialCount, maxCount, NULL );

      assert( m_semaphore != NULL ); // GetLastError()
   }

   inline vaSemaphore::~vaSemaphore()
   {
      BOOL ret = ::CloseHandle( m_semaphore );
      assert( ret ); // GetLastError()
      ret;
   }

   inline void vaSemaphore::Release( uint32 releaseCount, uint32 * releasedCount )
   {
      if( !ReleaseSemaphore( m_semaphore, releaseCount, (LPLONG)&releasedCount ) )
      {
         assert( false ); // GetLastError()
      }
   }

   inline ThreadingWaitResponseType vaSemaphore::WaitOne( uint32 waitTimeout )
   {
      DWORD dwWaitResult = WaitForSingleObject( m_semaphore, waitTimeout );          // zero-second time-out interval
      switch( dwWaitResult )
      {
         case( WAIT_FAILED )  : return TWRT_Failed;
         case( WAIT_OBJECT_0 ): return TWRT_Signaled_0;
         case( WAIT_TIMEOUT ) : return TWRT_Timeout;
      }
      return TWRT_Failed;
   }

   //inline static void vaSemaphore::WaitAll( vaSemaphore[] semaphores, const int semaphoresCount );
   //inline static void vaSemaphore::WaitAny( vaSemaphore[] semaphores, const int semaphoresCount );

}