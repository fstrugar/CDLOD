//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////


#include "CDLODStreamingTypeNormalmap.h"

#include "CDLODStreamingStorage.h"

#include "..\TiledBitmap\TiledBitmap.h"

#include "Core/IO/vaStream.h"

#include "..\DxEventNotifier.h"


CDLODStreamingTypeNormalmap::CDLODStreamingTypeNormalmap( CDLODStreamingStorage * storage, const int blockSize, const int LODOffset, 
                                                       uint32 arrayIndex, VertexAsylum::TiledBitmap * rawSourceData,
                                                       const ExportSettings & exportSettings)
 : CDLODStreamingTypeTextureBase( storage, blockSize, LODOffset, arrayIndex, rawSourceData )
{
   // only this format currently supported
   assert( m_rawSourceData->PixelFormat() == VertexAsylum::TBPF_Format24BitRGB );
   if( m_rawSourceData->PixelFormat() != VertexAsylum::TBPF_Format24BitRGB ) 
      vaFatalError( "CDLODStreamingTypeNormalmap: unsupported format" );

   if( exportSettings.NormalmapUseDXT5NMCompression )
      m_standardizedTextureFormat = D3DFMT_DXT5;
   else
      m_standardizedTextureFormat = D3DFMT_A8R8G8B8;
}

CDLODStreamingTypeNormalmap::CDLODStreamingTypeNormalmap( CDLODStreamingStorage * storage, uint32 arrayIndex, int texturePoolReserveMemory )
 : CDLODStreamingTypeTextureBase( storage, arrayIndex, texturePoolReserveMemory )
{
}

CDLODStreamingTypeNormalmap::~CDLODStreamingTypeNormalmap( )
{
}

uint32 PackToDXT5NM( const D3DXVECTOR3 & norm )
{
   byte x = (byte)clamp( 255.0f * ( norm.x * 0.5f + 0.5f ), 0.0f, 255.0f );
   byte y = (byte)clamp( 255.0f * ( norm.y * 0.5f + 0.5f ), 0.0f, 255.0f );

   // DXT5_NM-style packing: x in alpha, y in red
   return x << 24 | y << 16;
}

D3DXVECTOR3 UnpackFromDXT5NM( uint32 normU32 )
{
   byte x = (byte)((normU32>>24) & 0xFF);
   byte y = (byte)((normU32>>16) & 0xFF);

   D3DXVECTOR3 norm( x / 255.0f * 2.0f - 1.0f, y / 255.0f * 2.0f - 1.0f, 0.0f );
   norm.z = sqrtf( 1 - norm.x * norm.x - norm.y * norm.y );
   return norm;
}

void CDLODStreamingTypeNormalmap::FillTexture( TextureBlock * block, int imgRasterX, int imgRasterY, int imgRasterSizeX, int imgRasterSizeY,
                                                int stepSize, int mipLevels, const ExportSettings & exportSettings )
{
   HRESULT hr;

   // only this format currently supported
   assert( m_rawSourceData->PixelFormat() == VertexAsylum::TBPF_Format24BitRGB );
   if( m_rawSourceData->PixelFormat() != VertexAsylum::TBPF_Format24BitRGB ) return;

   V( DxEventNotifier::GetD3DDevice()->CreateTexture( block->TextureInfo.Width, block->TextureInfo.Height, mipLevels, 0, 
                                                      D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &block->Texture, NULL ) );

   for( int i = 0; i < mipLevels; i++ )
   {
      // Every MIP level must have width/height divisible by 4 to comply with DXT5
      assert( (block->TextureInfo.Width >> i) % 4 == 0 );
      assert( (block->TextureInfo.Height >> i) % 4 == 0 );
   }

   D3DLOCKED_RECT lockedRect; 
   V( block->Texture->LockRect( 0, &lockedRect, NULL, 0 ) );

   uint32 * imageData = (uint32 *)lockedRect.pBits;

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

         D3DXVECTOR3 normSum( 0.0f, 0.0f, 0.0f );
         for( int imgY = imgFromY; imgY <= imgToY; imgY++  )
            for( int imgX = imgFromX; imgX <= imgToX; imgX++  )
            {
               uint32 pixel;
               m_rawSourceData->GetPixel( imgX, imgY, &pixel );
               
               byte r = (byte)(pixel & 0xFF);
               byte g = (byte)((pixel >> 8) & 0xFF);
               byte b = (byte)((pixel >> 16) & 0xFF);

               D3DXVECTOR3 norm( (r / 255.0f - 0.5f) / 0.5f, (g / 255.0f - 0.5f) / 0.5f, (b / 255.0f - 0.5f) / 0.5f );
               normSum += norm;
            }

         D3DXVECTOR3 norm = normSum / (float)((imgToX-imgFromX+1)*(imgToY-imgFromY+1));

         D3DXVec3Normalize( &norm, &norm );
         imageData[x] = PackToDXT5NM( norm );
      }
      imageData += lockedRect.Pitch / sizeof(*imageData);
   }
   block->Texture->UnlockRect( 0 );

   // Proper mipmap filtere here as D3DXFilterTexture() doesn't know how to handle DXT5_NM
   for( int level = 1; level < mipLevels; level++ )
   {
      D3DLOCKED_RECT srcLockedRect; 
      V( block->Texture->LockRect( level-1, &srcLockedRect, NULL, 0 ) );
      uint32 * srcData = (uint32 *)srcLockedRect.pBits;

      D3DLOCKED_RECT destLockedRect; 
      V( block->Texture->LockRect( level, &destLockedRect, NULL, 0 ) );
      uint32 * destData = (uint32 *)destLockedRect.pBits;

      D3DSURFACE_DESC surfDesc;
      block->Texture->GetLevelDesc( level, &surfDesc );

      for( uint32 y = 0; y < surfDesc.Height; y++ )
      {
         uint32 * srcDataScanLineA = &srcData[(y*2) * srcLockedRect.Pitch / sizeof(*srcData)];
         uint32 * srcDataScanLineB = &srcData[(y*2+1) * srcLockedRect.Pitch / sizeof(*srcData)];
         uint32 * destDataScanLine = &destData[y * destLockedRect.Pitch / sizeof(*destData)];

         for( uint32 x = 0; x < surfDesc.Width; x++ )
         {
            D3DXVECTOR3 norm = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
            norm += UnpackFromDXT5NM( srcDataScanLineA[x*2] );
            norm += UnpackFromDXT5NM( srcDataScanLineA[x*2+1] );
            norm += UnpackFromDXT5NM( srcDataScanLineB[x*2] );
            norm += UnpackFromDXT5NM( srcDataScanLineB[x*2+1] );

            D3DXVec3Normalize( &norm, &norm );
            destDataScanLine[x] = PackToDXT5NM( norm );
         }
      }

      block->Texture->UnlockRect( level );
      block->Texture->UnlockRect( level-1 );
   }

   if( exportSettings.NormalmapUseDXT5NMCompression )
   {
      // Compress to DXT5 (DXT5_NM)
      IDirect3DTexture9 * compressedTexture = NULL;
      vaConvertTexture( compressedTexture, block->Texture, 0, D3DFMT_DXT5, D3DPOOL_MANAGED, true );
      SAFE_RELEASE( block->Texture );
      block->Texture = compressedTexture;
   }
}