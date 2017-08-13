///////////////////////////////////////////////////////////////////////////////////////////////////
// VertexAsylum Codebase, Copyright (c) Filip Strugar.
// Contents of this file are distributed under Zlib license.
// (http://en.wikipedia.org/wiki/Zlib_License)
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// Uses ZLIB compression library: see http://www.zlib.org for more info
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "Core/Compression/vaCompression.h"

#include "Core/Common/vaTempMemoryBuffer.h"

extern "C"
{
#include "Core/Compression/zlib/zlib.h"
};

#define A_CHECK_ERR(err, msg) { if (err != Z_OK) { throw "Most horrible error occurred!"; } }

using namespace VertexAsylum;

static byte * WriteInt32( byte * data, int32 val )
{
   data[0] = (byte)(val & 0x000000FF);
   data[1] = (byte)((val & 0x0000FF00) >> 8);
   data[2] = (byte)((val & 0x00FF0000) >> 16);
   data[3] = (byte)((val & 0xFF000000) >> 24);
   return data + 4;
}
static const byte * ReadInt32( const byte * data, int32 & val )
{
   val = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | (data[0] << 0);
   return &data[4];
}

bool vaCompression::SimplePack( CompressionType compressionType, byte * inData, int inDataSize, 
                                 byte * outData, int & outDataSize, bool useQuickLowQuality )
{
   if( (compressionType != CT_ZLIB) && (compressionType != CT_None) )
   {
      assert(false); // unsupported mode
      return false;
   }

   if( outDataSize < 16 )
   {
      assert(false); // not enough room even for the header
      return false;
   }

   byte * outDataCurrent = outData;

   // Write custom header
   
   // Version
   outDataCurrent = WriteInt32( outDataCurrent, 1 );
   
   // Uncompressed data size
   outDataCurrent = WriteInt32( outDataCurrent, inDataSize );

   // Compression type
   outDataCurrent = WriteInt32( outDataCurrent, (int32)compressionType );

   if( compressionType == CT_None )
   {
      if( inDataSize > (outDataSize+16) )
      {
         assert(false); // output buffer not big enough
         return false;
      }

      outDataSize = inDataSize+16;
      // Compressed data size
      outDataCurrent = WriteInt32( outDataCurrent, (int32)outDataSize );

      memcpy( outData+16, inData, outDataSize );
   }
   else if( compressionType == CT_ZLIB )
   {
      // Compressed data size
      byte * compressedDataSizePos = outDataCurrent;
      outDataCurrent = WriteInt32( outDataCurrent, (int32)0 );

      int quality = (useQuickLowQuality)?(Z_BEST_SPEED):(Z_BEST_COMPRESSION);

      uint32 compressedDataSize = outDataSize - 16;
      int res = compress2(  outDataCurrent, (uLongf*)&compressedDataSize, inData, inDataSize, quality );
      if( res == Z_OK )
      {
         outDataSize = compressedDataSize + 16;
         WriteInt32( compressedDataSizePos, compressedDataSize );

         return true;
      }
      else
      {
         assert( false );
         return false;
      }
   }

   return true;
}

int32 vaCompression::SimpleUnpackJustGetSize( byte * inData, int inDataSize )
{
   const byte * inDataCurrent = inData;

   if( inDataSize < 16 )
   {
      assert( false );
      return false;
   }

   int version;

   inDataCurrent = ReadInt32( inDataCurrent, version );

   if( version != 1 )
   {
      assert( false );  // version not supported
      return -1;
   }

   int uncompressedDataSize;

   // Uncompressed data size
   inDataCurrent = ReadInt32( inDataCurrent, uncompressedDataSize );

   return uncompressedDataSize;

}

bool vaCompression::SimpleUnpack( byte * inData, int inDataSize, byte * outData, int & outDataSize )
{
   const byte * inDataCurrent = inData;

   int version;

   inDataCurrent = ReadInt32( inDataCurrent, version );

   if( version != 1 )
   {
      assert( false );  // version not supported
      return false;
   }

   int uncompressedDataSize;

   // Uncompressed data size
   inDataCurrent = ReadInt32( inDataCurrent, uncompressedDataSize );

   int compressionTypeIntVal;
   inDataCurrent = ReadInt32( inDataCurrent, compressionTypeIntVal );
   CompressionType compressionType = (CompressionType)compressionTypeIntVal;

   if( (compressionType != CT_ZLIB) && (compressionType != CT_None) )
   {
      assert(false); // unsupported mode
      return false;
   }

   if( outDataSize < uncompressedDataSize )
   {
      assert(false); // not enough space in the buffer
      return false;
   }
   outDataSize = uncompressedDataSize;

   if( compressionType == CT_None )
   {
      // Compressed data size
      int compressedDataSize;
      inDataCurrent = ReadInt32( inDataCurrent, compressedDataSize );
      memcpy( outData, inDataCurrent, outDataSize );
      inDataSize;

      return true;
   }
   else if( compressionType == CT_ZLIB )
   {
      // Compressed data size
      int compressedDataSize;
      inDataCurrent = ReadInt32( inDataCurrent, compressedDataSize );

      uLongf destSizeArg = outDataSize;
      uLongf srcSizeArg = compressedDataSize;
      int res = uncompress( outData, &destSizeArg, inDataCurrent, srcSizeArg );
      if( res == Z_OK )
      {
         return true;
      }
      else
      {
         assert( false );
         return false;
      }
   }

   return false;
}
