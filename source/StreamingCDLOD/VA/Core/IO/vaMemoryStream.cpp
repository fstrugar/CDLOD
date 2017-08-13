//////////////////////////////////////////////////////////////////////////
// VertexAsylum Codebase, Copyright (c) Filip Strugar.
// Contents of this file are distributed under Zlib license.
// (http://en.wikipedia.org/wiki/Zlib_License)
//////////////////////////////////////////////////////////////////////////

#include "Core\IO\vaMemoryStream.h"

#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////
// vaMemoryStream
//////////////////////////////////////////////////////////////////////////////
using namespace VertexAsylum;
//
vaMemoryStream::vaMemoryStream( byte * buffer, int bufferSize )
{ 
   m_buffer = buffer;
   m_bufferSize = bufferSize;
   m_pos = 0;
}
//
vaMemoryStream::~vaMemoryStream(void) 
{
}
//
int vaMemoryStream::Read( void * buffer, int count )
{ 
   if( count + m_pos > m_bufferSize )
   {
      assert( false ); // buffer overrun
      count = m_bufferSize - m_pos;
   }

   memcpy(buffer, m_buffer + m_pos, count); 

   m_pos += count;

   return count; 
}
int vaMemoryStream::Write( const void * buffer, int count )
{ 
   if( count + m_pos > m_bufferSize )
   {
      assert( false ); // buffer overrun
      count = m_bufferSize - m_pos;
   }

   memcpy(m_buffer + m_pos, buffer, count); 
   
   m_pos += count;

   return count; 
}
//
///////////////////////////////////////////////////////////////////////////////////////////////////