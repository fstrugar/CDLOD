//////////////////////////////////////////////////////////////////////////
// VertexAsylum Codebase, Copyright (c) Filip Strugar.
// Contents of this file are distributed under Zlib license.
// (http://en.wikipedia.org/wiki/Zlib_License)
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "Core\vaCore.h"

namespace VertexAsylum
{
   class vaStream
   {
   public:
      virtual ~vaStream( void )  {}

      virtual void      Seek( __int64 position )                     = 0;
      virtual void      Close( )                                     = 0;
      virtual bool      IsOpen( )                                    = 0;
      virtual __int64   GetLength( )                                 = 0;
      virtual __int64   GetPosition( )                               = 0;

      virtual int       Read( void * buffer, int count )             = 0;
      virtual int       Write( const void * buffer, int count )      = 0;

      template<typename T>
      inline bool       WriteValue( const T & val );
      template<typename T>
      inline bool       ReadValue( T & val );
      template<typename T>
      inline bool       ReadValue( T & val, const T & def );
   };


   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // inline

   template<typename T>
   inline bool vaStream::WriteValue( const T & val )
   {
      int writtenCount = Write( &val, sizeof(T) );
      return writtenCount == sizeof(T);
   }

   template<typename T>
   inline bool vaStream::ReadValue( T & val )
   {
      int readCount = Read( &val, sizeof(T) );
      return readCount == sizeof(T);
   }
   template<typename T>
   inline bool vaStream::ReadValue( T & val, const T & deft )
   {
      if( ReadValue( val ) )
         return true;
      val = deft;
      return false;
   }
}