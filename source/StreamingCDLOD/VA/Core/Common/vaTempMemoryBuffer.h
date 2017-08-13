//////////////////////////////////////////////////////////////////////////
// VertexAsylum Codebase, Copyright (c) Filip Strugar.
// Contents of this file are distributed under Zlib license.
// (http://en.wikipedia.org/wiki/Zlib_License)
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "Core/vaCore.h"

// TODO: redo types

namespace VertexAsylum
{
   class vaTempMemoryBuffer
   {
   private:
      unsigned char *   m_buffer;
      int               m_bufferSize;

   public:
      vaTempMemoryBuffer()   { m_buffer = NULL; m_bufferSize = 0; }

      ~vaTempMemoryBuffer()  { delete[] m_buffer; }

      unsigned char * GetBuffer( int size )
      {
         if( size > m_bufferSize )
         {
            if( m_buffer != NULL )
               delete[] m_buffer;
            m_bufferSize = (int)(size * 1.2);
            m_buffer = new unsigned char[m_bufferSize];
         }
         return m_buffer;
      }
   };
}