
///////////////////////////////////////////////////////////////////////////////////////////////////
// VertexAsylum Codebase, Copyright (c) Filip Strugar.
// Contents of this file are distributed under Zlib license.
// (http://en.wikipedia.org/wiki/Zlib_License)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace VertexAsylum
{
   // Enums
   enum ThreadingWaitResponseType
   {
      TWRT_Failed       = 0xFFFFFFFF,
      TWRT_Signaled_0   = 0,
      TWRT_Timeout      = 258L,
   };
}


// Platform specific
#include "Core/Threading/vaThreadingPlatformSpecificTypes.h"
