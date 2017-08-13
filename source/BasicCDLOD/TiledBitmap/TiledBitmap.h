//////////////////////////////////////////////////////////////////////
// Copyright (C) 2007 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#ifndef __TiledBitmap_H__
#define __TiledBitmap_H__

#include <stdio.h>

#include <queue>

namespace VertexAsylum
{

#ifndef int64
   typedef __int64            int64;
#endif

   enum TiledBitmapLockMode
   {
      TBLM_Read,
      TBLM_Write,
      TBLM_ReadWrite
   };

   enum TiledBitmapPixelFormat
   {
      TBPF_Format16BitGrayScale        = 0,
      TBPF_Format8BitGrayScale         = 1,
      TBPF_Format24BitRGB              = 2,
      TBPF_Format32BitARGB             = 3,
      TBPF_Format16BitA4R4G4B4         = 4,
      TBPF_Format64BitD32VY16VX16      = 5,  // Watermap
      //TBPF_Format32BitX1R10G10B10,
      //TBPF_Format16BitRGB,
      //TBPF_Format16BitInt,
      //TBPF_Format16BitUInt,
      //TBPF_Format32BitInt,
      //TBPF_Format32BituInt,
      //TBPF_Format32BitFloat,
      //TBPF_Format64BitFloat,
   };


   /// <summary>
   /// BFB is a simple bitmap format that supports unlimited image sizes and
   /// where data is organized into tiles to enable fast reads/writes of random
   /// image sub-regions.
   /// 
   /// Current file format version is 1 (specified in FormatVersion field): supports 
   /// reading and writing of versions 0, 1.
   /// </summary>
   class TiledBitmap
   {
   public:
      static int                    GetPixelFormatBPP( TiledBitmapPixelFormat pixelFormat );

      static const int              c_FormatVersion = 1;
      static const int              c_MemoryLimit = 64 * 1024 * 1024; // allow 256Mb of memory usage
      static const int              c_UserHeaderSize = 224;
      static const int              c_TotalHeaderSize = 256;

   private:
    
      struct DataBlock
      {
         char *              pData;
         unsigned short      Width;
         unsigned short      Height;
         bool                Modified;
      };

      struct DataBlockID
      {
         int                 Bx;
         int                 By;

         DataBlockID( int bx, int by ) { this->Bx = bx; this->By = by; }
      };

      int                           m_UsedMemory;
      std::queue<DataBlockID>       m_UsedBlocks;

      FILE *                        m_File;
      __int64                       m_FileOffset;

      bool                          m_ReadOnly;

      int                           m_BlockDimBits;
      int                           m_BlocksX;
      int                           m_BlocksY;
      int                           m_EdgeBlockWidth;
      int                           m_EdgeBlockHeight;
      DataBlock **                  m_DataBlocks;

      TiledBitmapPixelFormat        m_PixelFormat;
      int                           m_Width;
      int                           m_Height;
      int                           m_BlockDim;
      int                           m_BytesPerPixel;

   public:
      TiledBitmapPixelFormat        PixelFormat() const     { return m_PixelFormat; }
      int                           BytesPerPixel() const   { return m_BytesPerPixel; }
      int                           Width() const           { return m_Width; }
      int                           Height() const          { return m_Height; }
      //const char * string                    FileName       { get { return file.Name; } }
      bool                          IsOpen() const          { return m_File != 0; }

   protected:
      TiledBitmap( FILE * file, TiledBitmapPixelFormat pixelFormat, int width, int height, int blockDim, bool readOnly, __int64 fileOffset = 0 );

   public:
      ~TiledBitmap();

   public:
      static TiledBitmap * Create( const char * pPath, TiledBitmapPixelFormat pixelFormat, int width, int height );
      static TiledBitmap * Open( const char * pPath, bool readOnly );
      static void          Close( TiledBitmap * pTiledBitmap );

      void                 Close();

   private:
      void           ReleaseBlock( int bx, int by );
      void           LoadBlock( int bx, int by );
      void           SaveBlock( int bx, int by );
      int64          GetBlockStartPos( int bx, int by );

   public:
      void           GetPixel( int x, int y, void* pPixel );
      void           SetPixel( int x, int y, void* pPixel );
      
      int            ReadHeader(void * pDestBuffer, int size);
      int            WriteHeader(void * pDestBuffer, int size);

      bool           Read( void * destBuffer, int destBytesPerPixel, int destBufferStride, int posX, int posY, int sizeX, int sizeY );
      //bool           Write( TiledBitmapBlock & source, int posX, int posY );
   };

}


#endif __TiledBitmap_H__
