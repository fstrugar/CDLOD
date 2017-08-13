//////////////////////////////////////////////////////////////////////
// Copyright (C) 2007 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#include "MipmappedTiledBitmap.h"
#include "TiledBitmap.h"

#include <stdlib.h>
#include <malloc.h>
#include <assert.h>

#pragma warning( disable : 4996 )

#define BREAKONERROR() { volatile int * pZero = 0; *pZero = 0; }

namespace VertexAsylum
{

   template<typename T>
   static T max(const T & a, const T & b)
   {
      return ((a) > (b)) ? (a) : (b);
   }

   template<typename T>
   static T min(const T & a, const T & b)
   {
      return ((a) < (b)) ? (a) : (b);
   }
/*
   static void WriteInt32( FILE * file, int val )
   {
      int count = (int)fwrite( &val, 1, sizeof(val), file );
      assert( count == sizeof(val) );
      count; // to suppress compiler warning
      //for( int i = 0; i < 4; i++ )
      //{
      //   stream.WriteByte( (byte)( val & 0xFF ) );
      //   val = val >> 8;
      //}
   }

   static int ReadInt32( FILE * file )
   {
      int ret;
      int count = (int)fread( &ret, 1, sizeof(ret), file );
      assert( count == sizeof(ret) );
      count; // to suppress compiler warning
      //for( int i = 0; i < 4; i++ )
      //   ret |= stream.ReadByte() << ( 8 * i );
      return ret;
   }
   */


}