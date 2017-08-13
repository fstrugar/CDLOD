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

   class vaFileStream : public vaStream
   {
      FILE *                  m_file;
        //
   public:
      vaFileStream( );
      virtual ~vaFileStream( void );

      virtual bool            Open( const wchar_t * file_path, bool read );
      virtual bool            Open( const char * file_path, bool read );

      virtual void            Seek( __int64 position );
      virtual void            Close( );
      virtual bool            IsOpen( );
      virtual __int64         GetLength( );
      virtual __int64         GetPosition( );

      virtual int             Read( void * buffer, int count );
      virtual int             Write( const void * buffer, int count );
   };


}

