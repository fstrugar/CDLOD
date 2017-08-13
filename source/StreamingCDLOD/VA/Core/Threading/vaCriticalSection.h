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

   class vaCriticalSection
   {
      vaCriticalSectionObjectPlatformType    m_cs;
   public:
      inline vaCriticalSection();
      inline ~vaCriticalSection();

      inline void          Enter();
      inline bool          TryEnter();
      inline void          Leave();
   };

   class vaCriticalSectionRAIILock
   {
   private:
      vaCriticalSection &  m_cs;
      vaCriticalSectionRAIILock & operator=( vaCriticalSectionRAIILock & from ) { from; assert( false ); }
   public:
      vaCriticalSectionRAIILock( vaCriticalSection & cs ) : m_cs(cs) { m_cs.Enter(); }
      ~vaCriticalSectionRAIILock( ) { m_cs.Leave(); }
   };

}

// Implementation
#include "Core/Threading/vaCriticalSectionPlatformSpecific.h"