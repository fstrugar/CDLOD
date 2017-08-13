
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
   // Typedefs
   typedef uint32 (*vaThreadProcPtr)(void * pUserData);

   class vaThread
   {
   protected:
      vaThread()  {}
      ~vaThread() {}

   public:
      enum ThreadPriority
      {
         TP_AboveNormal,
         TP_Normal,
         TP_BelowNormal
      };

   public:
      static vaThread *                      Create( uint32 stackSize, vaThreadProcPtr, void * userData );
      static void                            Destroy( vaThread * thread );

      virtual void                           Resume()                                  = 0;
      virtual bool                           WaitExit( uint32 timeout = 0xFFFFFFFF )   = 0;

      virtual void                           SetPriority( ThreadPriority priority )    = 0;
      virtual ThreadPriority                 GetPriority( )                            = 0;

      virtual vaThreadingSyncObjectHandle    GetSyncObjectHandle()                     = 0;
   };
}