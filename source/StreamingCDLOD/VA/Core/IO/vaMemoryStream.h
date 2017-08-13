//////////////////////////////////////////////////////////////////////////
// VertexAsylum Codebase, Copyright (c) Filip Strugar.
// Contents of this file are distributed under Zlib license.
// (http://en.wikipedia.org/wiki/Zlib_License)
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "Core\IO\vaStream.h"

struct _iobuf;
typedef struct _iobuf FILE;

namespace VertexAsylum
{

   class vaMemoryStream : public vaStream
   {
      byte *                  m_buffer;
      int                     m_bufferSize;
      int                     m_pos;
        //
   public:
      vaMemoryStream( byte * buffer, int bufferSize );
      virtual ~vaMemoryStream( void );

      virtual void            Seek( __int64 position )            { assert(position <= 2147483647 ); m_pos = (int32)position; }
      virtual void            Close( )                            { assert(false); }
      virtual bool            IsOpen( )                           { return m_buffer != NULL; }
      virtual __int64         GetLength( )                        { return m_bufferSize; }
      virtual __int64         GetPosition( )                      { return m_pos; }

      virtual int             Read( void * buffer, int count );
      virtual int             Write( const void * buffer, int count );

      byte *                  GetBuffer()                         { return m_buffer; }
   };


}

