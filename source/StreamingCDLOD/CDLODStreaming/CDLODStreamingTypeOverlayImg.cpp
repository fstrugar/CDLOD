//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////


#include "CDLODStreamingTypeOverlayImg.h"

#include "CDLODStreamingStorage.h"

#include "..\TiledBitmap\TiledBitmap.h"

#include "Core/IO/vaStream.h"

#include "..\DxEventNotifier.h"


CDLODStreamingTypeOverlayImg::CDLODStreamingTypeOverlayImg( CDLODStreamingStorage * storage, int blockSize, int LODOffset, 
                                                         int32 arrayIndex, VertexAsylum::TiledBitmap * rawSourceData, 
                                                         const ExportSettings & exportSettings )
 : CDLODStreamingTypeTextureBase( storage, blockSize, LODOffset, arrayIndex, rawSourceData )
{
   if( exportSettings.OverlaymapUseDXT1Compression )
   {
      // only this format currently supported in this case
      assert( m_rawSourceData->PixelFormat() == VertexAsylum::TBPF_Format24BitRGB );
      if( m_rawSourceData->PixelFormat() != VertexAsylum::TBPF_Format24BitRGB ) 
         vaFatalError( "CDLODStreamingTypeOverlayImg: input format not supported " );

      m_standardizedTextureFormat = D3DFMT_DXT1;
   }
   else
   {
      // only these formats currently supported in this case
      assert( (m_rawSourceData->PixelFormat() == VertexAsylum::TBPF_Format24BitRGB)
         || (m_rawSourceData->PixelFormat() == VertexAsylum::TBPF_Format16BitA4R4G4B4) );
      if( !((m_rawSourceData->PixelFormat() == VertexAsylum::TBPF_Format24BitRGB)
         || (m_rawSourceData->PixelFormat() == VertexAsylum::TBPF_Format16BitA4R4G4B4)) )
         vaFatalError( "CDLODStreamingTypeOverlayImg: input format not supported " );

      if( m_rawSourceData->PixelFormat() == VertexAsylum::TBPF_Format24BitRGB )
         m_standardizedTextureFormat = D3DFMT_A8R8G8B8;
      if( m_rawSourceData->PixelFormat() == VertexAsylum::TBPF_Format16BitA4R4G4B4 )
         m_standardizedTextureFormat = D3DFMT_A4R4G4B4;
   }
}

CDLODStreamingTypeOverlayImg::CDLODStreamingTypeOverlayImg( CDLODStreamingStorage * storage, int32 arrayIndex, int texturePoolReserveMemory )
 : CDLODStreamingTypeTextureBase( storage, arrayIndex, texturePoolReserveMemory )
{
}

CDLODStreamingTypeOverlayImg::~CDLODStreamingTypeOverlayImg( )
{
}

void CDLODStreamingTypeOverlayImg::FillTexture( TextureBlock * block, int imgRasterX, int imgRasterY, int imgRasterSizeX, int imgRasterSizeY,
                                                int stepSize, int mipLevels, const ExportSettings & exportSettings )
{
   HRESULT hr;

   for( int i = 0; i < mipLevels; i++ )
   {
      // Every MIP level must have width/height divisible by 4 to comply with DXT5
      assert( (block->TextureInfo.Width >> i) % 4 == 0 );
      assert( (block->TextureInfo.Height >> i) % 4 == 0 );
   }

   if( m_rawSourceData->PixelFormat() == VertexAsylum::TBPF_Format24BitRGB )
   {
      V( DxEventNotifier::GetD3DDevice()->CreateTexture( block->TextureInfo.Width, block->TextureInfo.Height, mipLevels, 0, 
         D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &block->Texture, NULL ) );

      D3DLOCKED_RECT heightmapLockedRect; 
      V( block->Texture->LockRect( 0, &heightmapLockedRect, NULL, 0 ) );

      uint32 * imageData = (uint32 *)heightmapLockedRect.pBits;

      // Source heightmap dimensions limits
      int maxRasterX = m_rawSourceData->Width()-1;
      int maxRasterY = m_rawSourceData->Height()-1;

      for( int y = 0; y < block->TextureInfo.Height; y++ )
      {
         int imgFromY = imgRasterY + y * stepSize;
         int imgToY = imgFromY + stepSize-1;
         imgFromY = ::min( imgFromY, maxRasterY );
         imgToY = ::min( imgToY, maxRasterY );

         for( int x = 0; x < block->TextureInfo.Width; x++ )
         {
            int imgFromX = imgRasterX + x * stepSize;
            int imgToX = imgFromX + stepSize-1;
            imgFromX = ::min( imgFromX, maxRasterX );
            imgToX = ::min( imgToX, maxRasterX );

            D3DXVECTOR4 colorSum( 0.0f, 0.0f, 0.0f, 0.0f );

            for( int imgY = imgFromY; imgY <= imgToY; imgY++  )
               for( int imgX = imgFromX; imgX <= imgToX; imgX++  )
               {
                  uint32 pixel;
                  m_rawSourceData->GetPixel( imgX, imgY, &pixel );
                  
                  byte r = (byte)(pixel & 0xFF);
                  byte g = (byte)((pixel >> 8) & 0xFF);
                  byte b = (byte)((pixel >> 16) & 0xFF);
                  byte a = (byte)255;

                  D3DXVECTOR4 color( r, g, b, a );
                  colorSum += color;
               }

            D3DXVECTOR4 color = colorSum / (float)((imgToX-imgFromX+1)*(imgToY-imgFromY+1));

            byte r = (byte)clamp( color.x+0.5f, 0.0f, 255.0f );
            byte g = (byte)clamp( color.y+0.5f, 0.0f, 255.0f );
            byte b = (byte)clamp( color.z+0.5f, 0.0f, 255.0f );
            byte a = (byte)clamp( color.w+0.5f, 0.0f, 255.0f );
     
            uint32 outPixel = a << 24 | r << 16 | g << 8 | b;

            imageData[x] = outPixel;
         }
         imageData += heightmapLockedRect.Pitch / sizeof(*imageData);
      }
      block->Texture->UnlockRect( 0 );
   }

   if( m_rawSourceData->PixelFormat() == VertexAsylum::TBPF_Format16BitA4R4G4B4 )
   {
      assert( m_standardizedTextureFormat == D3DFMT_A4R4G4B4 );
      V( DxEventNotifier::GetD3DDevice()->CreateTexture( block->TextureInfo.Width, block->TextureInfo.Height, mipLevels, 0, 
         D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, &block->Texture, NULL ) );

      D3DLOCKED_RECT heightmapLockedRect; 
      V( block->Texture->LockRect( 0, &heightmapLockedRect, NULL, 0 ) );

      uint16 * imageData = (uint16 *)heightmapLockedRect.pBits;

      // Source heightmap dimensions limits
      int maxRasterX = m_rawSourceData->Width()-1;
      int maxRasterY = m_rawSourceData->Height()-1;

      for( int y = 0; y < block->TextureInfo.Height; y++ )
      {
         int imgFromY = imgRasterY + y * stepSize;
         int imgToY = imgFromY + stepSize-1;
         imgFromY = ::min( imgFromY, maxRasterY );
         imgToY = ::min( imgToY, maxRasterY );

         for( int x = 0; x < block->TextureInfo.Width; x++ )
         {
            int imgFromX = imgRasterX + x * stepSize;
            int imgToX = imgFromX + stepSize-1;
            imgFromX = ::min( imgFromX, maxRasterX );
            imgToX = ::min( imgToX, maxRasterX );

            D3DXVECTOR4 colorSum( 0.0f, 0.0f, 0.0f, 0.0f );

            for( int imgY = imgFromY; imgY <= imgToY; imgY++  )
               for( int imgX = imgFromX; imgX <= imgToX; imgX++  )
               {
                  uint16 pixel;
                  m_rawSourceData->GetPixel( imgX, imgY, &pixel );

                  byte r = (byte)(pixel & 0xF);
                  byte g = (byte)((pixel >> 4) & 0xF);
                  byte b = (byte)((pixel >> 8) & 0xF);
                  byte a = (byte)((pixel >> 12) & 0xF);

                  D3DXVECTOR4 color( r, g, b, a );
                  colorSum += color;
               }

               D3DXVECTOR4 color = colorSum / (float)((imgToX-imgFromX+1)*(imgToY-imgFromY+1));

               byte r = (byte)clamp( color.x+0.5f, 0.0f, 31.0f );
               byte g = (byte)clamp( color.y+0.5f, 0.0f, 31.0f );
               byte b = (byte)clamp( color.z+0.5f, 0.0f, 31.0f );
               byte a = (byte)clamp( color.w+0.5f, 0.0f, 31.0f );

               uint16 outPixel = a << 12 | r << 8 | g << 4 | b;

               imageData[x] = outPixel;
         }
         imageData += heightmapLockedRect.Pitch / sizeof(*imageData);
      }
      block->Texture->UnlockRect( 0 );
   }


   V( D3DXFilterTexture( block->Texture, NULL, 0, D3DX_FILTER_BOX ) );

   if( exportSettings.OverlaymapUseDXT1Compression )
   {
      assert( m_standardizedTextureFormat == D3DFMT_DXT1 );

      // Compress to DXT5 (DXT5_NM)
      IDirect3DTexture9 * compressedTexture = NULL;
      vaConvertTexture( compressedTexture, block->Texture, 0, D3DFMT_DXT1, D3DPOOL_MANAGED, true );
      SAFE_RELEASE( block->Texture );
      block->Texture = compressedTexture;
   }
}