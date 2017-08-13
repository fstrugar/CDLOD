///////////////////////////////////////////////////////////////////////////////////////////////////
// VertexAsylum Codebase, Copyright (c) Filip Strugar.
// Contents of this file are distributed under Zlib license.
// (http://en.wikipedia.org/wiki/Zlib_License)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace VertexAsylum
{
   inline vaCriticalSection::vaCriticalSection()
   {
      InitializeCriticalSection( &m_cs );
   }

   inline vaCriticalSection::~vaCriticalSection()
   {
      DeleteCriticalSection( &m_cs );
   }

   inline void vaCriticalSection::Enter()
   {
      EnterCriticalSection( &m_cs );
   }

   inline bool vaCriticalSection::TryEnter()
   {
      return TryEnterCriticalSection( &m_cs ) == TRUE;
   }

   inline void vaCriticalSection::Leave()
   {
      LeaveCriticalSection( &m_cs );
   }
}