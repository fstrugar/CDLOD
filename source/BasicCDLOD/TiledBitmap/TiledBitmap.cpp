//////////////////////////////////////////////////////////////////////
// Copyright (C) 2007 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

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

   int TiledBitmap::GetPixelFormatBPP( TiledBitmapPixelFormat pixelFormat )
   {
      switch( pixelFormat )
      {
         case ( TBPF_Format8BitGrayScale ):     return 1;
         case ( TBPF_Format16BitGrayScale ):    return 2;
         case ( TBPF_Format24BitRGB ):          return 3;
         case ( TBPF_Format32BitARGB ):         return 4;
         case ( TBPF_Format16BitA4R4G4B4 ):     return 2;
         case ( TBPF_Format64BitD32VY16VX16):   return 8;
         default: return -1;
      }
   }


   TiledBitmap::TiledBitmap( FILE * file, TiledBitmapPixelFormat pixelFormat, int width, int height, int blockDim, bool readOnly, __int64 fileOffset )
   {
      m_File            = file;
      m_PixelFormat     = pixelFormat;
      m_Width           = width;
      m_Height          = height;
      m_BlockDim        = blockDim;
      m_ReadOnly        = readOnly;
      m_BytesPerPixel   = TiledBitmap::GetPixelFormatBPP( pixelFormat );
      m_UsedMemory      = 0;

      m_FileOffset      = fileOffset;

      m_BlocksX         = ( width - 1 ) / blockDim + 1;
      m_BlocksY         = ( height - 1 ) / blockDim + 1;
      m_EdgeBlockWidth  = width - ( m_BlocksX - 1 ) * blockDim;
      m_EdgeBlockHeight = height - ( m_BlocksY - 1 ) * blockDim;

      if( ( ( blockDim - 1 ) & blockDim ) != 0 ) 
      {
         assert( false ); // "blockDim must be power of 2"
         BREAKONERROR();
      }

      m_BlockDimBits = 0; int a = blockDim;
      while( a > 1 ) { m_BlockDimBits++; a /= 2; }

      m_DataBlocks = new DataBlock*[m_BlocksX];
      for( int x = 0; x < m_BlocksX; x++ )
      {
         m_DataBlocks[x] = new DataBlock[m_BlocksY];
         for( int y = 0; y < m_BlocksY; y++ )
         {
            m_DataBlocks[x][y].pData = 0;
            m_DataBlocks[x][y].Width = (unsigned short)( ( x == ( m_BlocksX - 1 ) ) ? ( m_EdgeBlockWidth ) : ( blockDim ) );
            m_DataBlocks[x][y].Height = (unsigned short)( ( y == ( m_BlocksY - 1 ) ) ? ( m_EdgeBlockHeight ) : ( blockDim ) );
            m_DataBlocks[x][y].Modified = false;
         }
      }

      _fseeki64( m_File, c_TotalHeaderSize + m_FileOffset, SEEK_SET );
   }

   TiledBitmap::~TiledBitmap()
   {
      Close();
   }

   TiledBitmap * TiledBitmap::Create( const char * pPath, TiledBitmapPixelFormat pixelFormat, int width, int height )
   {
      int bytesPerPixel = GetPixelFormatBPP( pixelFormat );
      if( bytesPerPixel < 0 || bytesPerPixel > 8 ) 
      {
         assert( false ); // "bytesPerPixel < 0 || bytesPerPixel > 4" // could work for bytesPerPixel > 4, but is not tested
         BREAKONERROR();
      }

      FILE * file = fopen(pPath, "w+b"); // File.Open( path, FileMode.Create, FileAccess.ReadWrite, FileShare.None );

      // set file size..
      int64 fileSize = (int64)bytesPerPixel * width * height + c_TotalHeaderSize;
      _fseeki64( file, fileSize-1, SEEK_SET );
      char dummy = 0;
      fwrite( &dummy, 1, 1, file );

      _fseeki64( file, 0, SEEK_SET );

      WriteInt32( file, (int)pixelFormat );
      WriteInt32( file, width );
      WriteInt32( file, height );

      int blockDim = 256;

      int version = c_FormatVersion;
      WriteInt32( file, version );
      WriteInt32( file, blockDim );

      int64 pos = _ftelli64(file);
      for( ; pos < c_TotalHeaderSize; pos++ )
         fwrite( &dummy, 1, 1, file );

      return new TiledBitmap( file, pixelFormat, width, height, blockDim, false );
   }

   TiledBitmap * TiledBitmap::Open( const char * pPath, bool readOnly )
   {
      FILE * file;
      if( readOnly )
         file = fopen(pPath, "rb"); // File.Open( path, FileMode.Open, FileAccess.Read, FileShare.Read );
      else
         file = fopen(pPath, "r+b"); // File.Open( path, FileMode.Open, FileAccess.ReadWrite, FileShare.None );

      if( file == 0 )
      {
         return 0;
      }

      TiledBitmapPixelFormat pixelFormat = (TiledBitmapPixelFormat)ReadInt32( file );
      int bytesPerPixel =  GetPixelFormatBPP( pixelFormat );
      bytesPerPixel;
      int width         = ReadInt32( file );
      int height        = ReadInt32( file );
      int version       = ReadInt32( file );

      int blockDim = 128;
      if( version > 0 )
         blockDim = ReadInt32( file );

      //{
      //   int64 current = _ftelli64(file);
      //   _fseeki64(file, 0, SEEK_END);
      //   int64 filelength = _ftelli64(file);
      //   _fseeki64(file, current, SEEK_SET);
   
      //   if( ( (int64)bytesPerPixel * width * height + c_TotalHeaderSize ) != filelength )
      //   {
      //      assert( false ); // file is probably corrupt
      //   }
      //}

      return new TiledBitmap( file, pixelFormat, width, height, blockDim, readOnly );
   }

   void TiledBitmap::Close( TiledBitmap * pTiledBitmap )
   {
      delete pTiledBitmap;
   }

   void TiledBitmap::Close()
   {
      if( m_File == 0 ) return;

      if( m_DataBlocks )
      {
         for( int x = 0; x < m_BlocksX; x++ )
         {
            for( int y = 0; y < m_BlocksY; y++ )
            { 
               if( m_DataBlocks[x][y].pData != 0 ) 
                  ReleaseBlock( x, y ); 
            }
            delete[] m_DataBlocks[x];
         }
         delete[] m_DataBlocks;
         m_DataBlocks = 0;
      }

      fclose( m_File );
      m_File = 0;
   }

   void TiledBitmap::ReleaseBlock( int bx, int by )
   {
      DataBlock & db = m_DataBlocks[bx][by];
      if( db.pData == 0 ) 
      {
         assert( false ); // "block not loaded"
         BREAKONERROR();
      }

      if( db.Modified ) 
         SaveBlock( bx, by );

      int blockSize = db.Width * db.Height * m_BytesPerPixel;
      m_UsedMemory -= blockSize;

      free( db.pData );
      db.Modified = false;
      db.pData = 0;
   }

   void TiledBitmap::LoadBlock( int bx, int by )
   {
      DataBlock & db = m_DataBlocks[bx][by];
      if( db.pData != 0 ) 
      {
         assert( false ); // "Block already loaded"
         BREAKONERROR();
      }

      //load
      while( m_UsedMemory > c_MemoryLimit )
      {
         DataBlockID dbid = (DataBlockID)m_UsedBlocks.front();
         m_UsedBlocks.pop();
         ReleaseBlock( dbid.Bx, dbid.By );
      }

      int blockSize = db.Width * db.Height * m_BytesPerPixel;

      db.pData = (char*)malloc( blockSize );

      _fseeki64( m_File, GetBlockStartPos( bx, by ) + m_FileOffset, SEEK_SET );
      if( (int)fread( db.pData, 1, blockSize, m_File ) != blockSize )
      {
         assert( false );
      }
      db.Modified = false;

      m_UsedBlocks.push( DataBlockID( bx, by ) );
      m_UsedMemory += blockSize;
   }

   void TiledBitmap::SaveBlock( int bx, int by )
   {
      DataBlock & db = m_DataBlocks[bx][by];
      if( db.pData == 0 )
      {
         assert( false ); // "block not loaded"
         BREAKONERROR();
      }

      int blockSize = db.Width * db.Height * m_BytesPerPixel;
      
      _fseeki64( m_File, GetBlockStartPos( bx, by ) + m_FileOffset, SEEK_SET );
      fwrite( db.pData, 1, blockSize, m_File );
      db.Modified = false;
   }

   int64 TiledBitmap::GetBlockStartPos( int bx, int by )
   {
      int64 pos = c_TotalHeaderSize;

      pos += (int64)by * ( m_BlocksX - 1 ) * ( (int64)m_BlockDim * m_BlockDim * m_BytesPerPixel );
      pos += (int64)by * ( (int64)m_BlockDim * m_EdgeBlockWidth * m_BytesPerPixel );

      if( by == ( m_BlocksY - 1 ) )
         pos += ( (int64)bx ) * ( (int64)m_BlockDim * m_EdgeBlockHeight * m_BytesPerPixel );
      else
         pos += ( (int64)bx ) * ( (int64)m_BlockDim * m_BlockDim * m_BytesPerPixel );

      return pos;
   }

   void TiledBitmap::GetPixel( int x, int y, void* pPixel )
   {
      assert( x >= 0 && x <= m_Width && y >= 0 && y <= m_Height );
      int bx = x >> m_BlockDimBits;
      int by = y >> m_BlockDimBits;
      x -= bx << m_BlockDimBits;
      y -= by << m_BlockDimBits;
      DataBlock & db = m_DataBlocks[bx][by];
      if( db.pData == 0 )
         LoadBlock( bx, by );
      memcpy( pPixel, db.pData + ( ( db.Width * y + x ) * m_BytesPerPixel ), m_BytesPerPixel );
   }

   void TiledBitmap::SetPixel( int x, int y, void* pPixel )
   {
      assert( !m_ReadOnly );
      assert( x >= 0 && x <= m_Width && y >= 0 && y <= m_Height );

      int bx = x >> m_BlockDimBits;
      int by = y >> m_BlockDimBits;
      x -= bx << m_BlockDimBits;
      y -= by << m_BlockDimBits;
      DataBlock & db = m_DataBlocks[bx][by];
      if( db.pData == 0 )
         LoadBlock( bx, by );

      char* pTo = db.pData + ( ( db.Width * y + x ) * m_BytesPerPixel );
      char* pFrom = (char*)pPixel;
      for( int b = 0; b < m_BytesPerPixel; b++ )
      {
         *pTo = *pFrom;
         pTo++; pFrom++;
      }

      db.Modified = true;
   }

   int TiledBitmap::ReadHeader(void * pDestBuffer, int size)
   {
      pDestBuffer;
      size;
      return -1; // not implemented, I was lazy 
      //file.Position = ( TotalHeaderSize - UserHeaderSize );
      //byte[] buffer = new byte[UserHeaderSize];
      //file.Read( buffer, 0, UserHeaderSize );
      //return buffer;
   }

   int TiledBitmap::WriteHeader(void * pDestBuffer, int size)
   {
      pDestBuffer;
      size;
      return -1; // not implemented, I was lazy 
      //file.Position = ( TotalHeaderSize - UserHeaderSize );
      //file.Write( buffer, 0, buffer.Length );
   }

   bool TiledBitmap::Read( void * destBuffer, int destBytesPerPixel, int destBufferStride, int posX, int posY, int destSizeX, int destSizeY )
   {
      if( (destBuffer == 0) || (destBytesPerPixel != m_BytesPerPixel) )
      {
         assert( false );
         return false;
      }

      if( ( posX < 0 ) || ( ( posX + destSizeX ) > m_Width ) || ( posY < 0 ) || ( ( posY + destSizeY ) > m_Height ) || ( destSizeX < 0 ) || ( destSizeY < 0 ) )
      {
         assert( false ); // "Invalid lock region"
         return false;
      }

      char* pBuffer = (char*)destBuffer;

      int blockX0 = posX / m_BlockDim;
      int blockY0 = posY / m_BlockDim;
      int blockX1 = ( posX + destSizeX - 1 ) / m_BlockDim;
      int blockY1 = ( posY + destSizeY - 1 ) / m_BlockDim;

      for( int by = blockY0; by <= blockY1; by++ )
         for( int bx = blockX0; bx <= blockX1; bx++ )
         {

            int bw = ( bx == ( m_BlocksX - 1 ) ) ? ( m_EdgeBlockWidth ) : ( m_BlockDim );
            int bh = ( by == ( m_BlocksY - 1 ) ) ? ( m_EdgeBlockHeight ) : ( m_BlockDim );

            assert( bx >= 0 && bx < m_BlocksX );
            assert( by >= 0 && by < m_BlocksY );


            DataBlock & db = m_DataBlocks[bx][by];
            if( db.pData == 0 )
               LoadBlock( bx, by );
            char* tempBlockBuffer = (char*)db.pData;

            int fromX = max( bx * m_BlockDim, posX );
            int fromY = max( by * m_BlockDim, posY );
            int toX = min( bx * m_BlockDim + bw, posX + destSizeX );
            int toY = min( by * m_BlockDim + bh, posY + destSizeY );

            for( int y = fromY; y < toY; y++ )
            {
               int lly = y - posY;
               int bly = y - ( by * m_BlockDim );
               for( int x = fromX; x < toX; x++ )
               {
                  int llx = x - posX;
                  int blx = x - ( bx * m_BlockDim );
                  int indexl = llx * m_BytesPerPixel + lly * destBufferStride;
                  int indexb = ( blx + bly * bw ) * m_BytesPerPixel;
                  for( int b = 0; b < m_BytesPerPixel; b++ )
                     pBuffer[indexl + b] = tempBlockBuffer[indexb + b];
               }
            }
         }

      return true;
   }

   /*
   bool TiledBitmap::Write( TiledBitmapBlock & source, int posX, int posY )
   {
      if( m_ReadOnly )
      {
         assert( false );
         return false;
      }

      int tileWidth = source.Width();
      int tileHeight = source.Height();

      if( (source.BufferPtr() == 0) || (source.BytesPerPixel() != m_BytesPerPixel) )
      {
         assert( false );
         return false;
      }

      if( ( posX < 0 ) || ( ( posX + tileWidth ) > m_Width ) || ( posY < 0 ) || ( ( posY + tileHeight ) > m_Height ) || ( tileWidth < 0 ) || ( tileHeight < 0 ) )
      {
         assert( false ); // "Invalid lock region"
         return false;
      }

      char* tempBlockBuffer = (char*)malloc(m_BytesPerPixel * m_BlockDim * m_BlockDim);

      char* pBuffer = (char*) source.BufferPtr();
      //int bufferSize = tileWidth * tileHeight * m_BytesPerPixel;

      int blockX0 = posX / m_BlockDim;
      int blockY0 = posY / m_BlockDim;
      int blockX1 = ( posX + tileWidth ) / m_BlockDim;
      int blockY1 = ( posY + tileHeight ) / m_BlockDim;

      for( int by = blockY0; by < blockY1; by++ )
         for( int bx = blockX0; bx < blockX1; bx++ )
         {
            int bw = ( bx == ( m_BlocksX - 1 ) ) ? ( m_EdgeBlockWidth ) : ( m_BlockDim );
            int bh = ( by == ( m_BlocksY - 1 ) ) ? ( m_EdgeBlockWidth ) : ( m_BlockDim );

            int64 blockStartPos = GetBlockStartPos( bx, by );

            int fromX = max( bx * m_BlockDim, posX );
            int fromY = max( by * m_BlockDim, posY );
            int toX = min( bx * m_BlockDim + bw, posX + tileWidth );
            int toY = min( by * m_BlockDim + bh, posY + tileHeight );

            if( ( ( fromX != bx * m_BlockDim ) || ( fromY != by * m_BlockDim ) ||
                ( toX != ( bx * m_BlockDim + bw ) ) || ( toY != ( by * m_BlockDim + bh ) ) ) )
            {
               _fseeki64( m_File, blockStartPos + m_FileOffset, SEEK_SET );
               if( (int)fread( tempBlockBuffer, 1, bw * bh * m_BytesPerPixel, m_File ) != bw * bh * m_BytesPerPixel )
               {
                  assert( false );
               }
            }

            for( int y = fromY; y < toY; y++ )
               for( int x = fromX; x < toX; x++ )
               {
                  int llx = x - posX;
                  int lly = y - posY;
                  int blx = x - ( bx * m_BlockDim );
                  int bly = y - ( by * m_BlockDim );
                  int indexl = ( llx + lly * tileWidth ) * m_BytesPerPixel;
                  int indexb = ( blx + bly * bw ) * m_BytesPerPixel;
                  for( int b = 0; b < m_BytesPerPixel; b++ )
                     tempBlockBuffer[indexb + b] = pBuffer[indexl + b];
               }

            _fseeki64( m_File, blockStartPos + m_FileOffset, SEEK_SET );
            if( (int)fwrite( tempBlockBuffer, 1, bw * bh * m_BytesPerPixel, m_File ) != bw * bh * m_BytesPerPixel )
            {
               assert( false );
            }

         }
      free( tempBlockBuffer );

      return true;
   }
   */

}