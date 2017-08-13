
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

   class vaThreading
   {
   public:
      inline static void                        Sleep( uint32 milliseconds );
      inline static ThreadingWaitResponseType   WaitAll( vaThreadingSyncObjectHandle handles[], int32 handleCount, uint32 waitTimeout = 0xFFFFFFFF );
      inline static ThreadingWaitResponseType   WaitAny( vaThreadingSyncObjectHandle handles[], int32 handleCount, uint32 waitTimeout = 0xFFFFFFFF );
   };

}

// Implementation
#include "Core/Threading/vaThreadingPlatformSpecific.h"
