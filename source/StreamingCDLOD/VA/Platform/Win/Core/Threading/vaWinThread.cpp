///////////////////////////////////////////////////////////////////////////////////////////////////
// VertexAsylum Codebase, Copyright (c) Filip Strugar.
// Contents of this file are distributed under Zlib license.
// (http://en.wikipedia.org/wiki/Zlib_License)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Core/Threading/vaThread.h"

using namespace VertexAsylum;


class vaThreadWindows : public vaThread
{
   struct Proxy
   {
      Proxy( vaThreadProcPtr threadProc, void * userData ) : threadProc(threadProc), userData(userData) {}
      vaThreadProcPtr        threadProc;
      void *               userData;
   };

   HANDLE         m_thread;

   ThreadPriority m_priority;

protected:
   friend vaThread * vaThread::Create( uint32 stackSize, vaThreadProcPtr procPtr, void * userData );
   friend void vaThread::Destroy( vaThread * thread );

   vaThreadWindows(uint32 stackSize, vaThreadProcPtr, void * userData);
   ~vaThreadWindows();

public:

   virtual void                           Resume();
   virtual bool                           WaitExit( uint32 timeout = 0xFFFFFFFF );

   virtual vaThreadingSyncObjectHandle    GetSyncObjectHandle()   { return m_thread; }

   virtual void                           SetPriority( ThreadPriority priority );
   virtual ThreadPriority                 GetPriority( )                            { return m_priority; }

   static DWORD WINAPI ThreadProcProxy( LPVOID lpThreadParameter );
};
//
vaThread * vaThread::Create( uint32 stackSize, vaThreadProcPtr procPtr, void * userData )
{
   return new vaThreadWindows( stackSize, procPtr, userData );
}
//
void vaThread::Destroy( vaThread * thread )
{
   delete static_cast<vaThreadWindows*>(thread);
}
//
void vaThreadWindows::Resume()
{
   assert( m_thread != NULL );
   ::ResumeThread(m_thread);
}
//
bool vaThreadWindows::WaitExit( uint32 timeout )
{
   assert( m_thread != NULL );
   bool retVal = ::WaitForSingleObject( m_thread, timeout ) == WAIT_OBJECT_0;

   if( retVal )
   {
      CloseHandle(m_thread);
      m_thread = NULL;
   }
   return retVal;
}
//
void vaThreadWindows::SetPriority( ThreadPriority priority )
{
   m_priority = priority;
   switch( m_priority )
   {
   case( TP_BelowNormal ): ::SetThreadPriority( m_thread, THREAD_PRIORITY_BELOW_NORMAL ); break;
   case( TP_Normal ):      ::SetThreadPriority( m_thread, THREAD_PRIORITY_NORMAL ); break;
   case( TP_AboveNormal ): ::SetThreadPriority( m_thread, THREAD_PRIORITY_ABOVE_NORMAL ); break;
   default: assert( false );
   }
}
//
vaThreadWindows::vaThreadWindows(uint32 stackSize, vaThreadProcPtr procPtr, void * userData)
{
   m_thread = ::CreateThread( NULL, stackSize, ThreadProcProxy, new Proxy(procPtr, userData), CREATE_SUSPENDED, NULL );
   m_priority = TP_Normal;
}
//
vaThreadWindows::~vaThreadWindows()
{
   if( m_thread != NULL )
      WaitExit( INFINITE );
}
//
DWORD WINAPI vaThreadWindows::ThreadProcProxy( LPVOID lpThreadParameter )
{
   Proxy * proxy = static_cast<Proxy*>(lpThreadParameter);

   uint32 retVal = proxy->threadProc(proxy->userData);
   delete proxy;
   return retVal;
}
