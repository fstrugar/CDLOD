//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////


#include "CDLODStreamingTypeHeightmap.h"

#include "CDLODStreamingStorage.h"

#include "..\TiledBitmap\TiledBitmap.h"

#include "Core/IO/vaStream.h"

#include "..\DxEventNotifier.h"

#include "Core/Common/vaTempMemoryBuffer.h"


CDLODStreamingTypeHeightmap::CDLODStreamingTypeHeightmap( CDLODStreamingStorage * storage, const int blockSize, const int LODOffset,
                                                       uint32 arrayIndex, VertexAsylum::TiledBitmap * rawSourceData )
 : CDLODStreamingTypeTextureBase( storage, blockSize, LODOffset, arrayIndex, NULL, new SimpleAllocator<HeightmapBlock>() )
{
   m_rawSourceData = rawSourceData;

   assert( m_rawSourceData->Width() == m_storage->GetWorldDesc().RasterSizeX );
   assert( m_rawSourceData->Height() == m_storage->GetWorldDesc().RasterSizeY );

   if( !( (m_rawSourceData->Width() == m_storage->GetWorldDesc().RasterSizeX)
      && (m_rawSourceData->Height() == m_storage->GetWorldDesc().RasterSizeY ) ) )
   {
      vaFatalError("Heightmap source data incorrect");
   }

   m_texelScaleX = 1;
   m_texelScaleY = 1;

   m_standardizedTextureSizeX = (int)(m_texelScaleX * m_blockSize) + 1;
   m_standardizedTextureSizeY = (int)(m_texelScaleY * m_blockSize) + 1;
   m_standardizedTextureFormat = D3DFMT_UNKNOWN;
   m_standardizedTextureMipCount = 1;
}

CDLODStreamingTypeHeightmap::CDLODStreamingTypeHeightmap( CDLODStreamingStorage * storage, uint32 arrayIndex, int texturePoolReserveMemory )
 : CDLODStreamingTypeTextureBase( storage, arrayIndex, texturePoolReserveMemory, new SimpleAllocator<HeightmapBlock>() )
{

}

CDLODStreamingTypeHeightmap::~CDLODStreamingTypeHeightmap( )
{
}

void CDLODStreamingTypeHeightmap::HeightmapBlock::OnDeallocate()
{ 
   delete[] CornerPoints;
   CDLODStreamingTypeTextureBase::TextureBlock::OnDeallocate(); 
}

void CDLODStreamingTypeHeightmap::ExportBlockForQuad( VertexAsylum::vaStream * outStream, CDLODStreamingTypeBase::Block* inBlock, 
                                                    const int quadRasterX, const int quadRasterY, const int quadRasterSize, const int quadLODLevel, 
                                                    const ExportSettings & exportSettings )
{
   // blockX and blockY are indices in the 2D array of blocks of this LOD level (or they would be, if there was an array)
   int blockX, blockY, blockLODLevel;

   // CDLODStreamingStorage will use our allocator so this cast should always be correct.
   HeightmapBlock * block = static_cast<HeightmapBlock *>(inBlock);

   DecodeBlockHashID( block->HashID, blockX, blockY, blockLODLevel );

   // Block coverage in (heightmap) raster space.
   int blockRasterX, blockRasterY, blockRasterSize;
   int stepSize = 1 << blockLODLevel;


   CalcBlockRasterCoverage( blockX, blockY, blockLODLevel, blockRasterX, blockRasterY, blockRasterSize );

   // Block size (in texels of our texture representing the block)
   block->TextureInfo.Width   = m_blockSize+1;
   block->TextureInfo.Height  = m_blockSize+1;

   // Limit our block texture size if it runs out of the source data dimensions
   {
      int maxTextureX = (m_rawSourceData->Width() + stepSize - 1) / stepSize;
      int maxTextureY = (m_rawSourceData->Height() + stepSize - 1) / stepSize;
      int ourBlockMaxTextureX = (blockX+1) * m_blockSize + 1;
      int ourBlockMaxTextureY = (blockY+1) * m_blockSize + 1;
      block->TextureInfo.Width -= ::max( 0, ourBlockMaxTextureX - maxTextureX );
      block->TextureInfo.Height -= ::max( 0, ourBlockMaxTextureY - maxTextureY );
   }

   block->Texture = NULL;
   
   int blockRasterSizeX = (block->TextureInfo.Width-1) * stepSize;
   int blockRasterSizeY = (block->TextureInfo.Height-1) * stepSize;

   m_storage->RasterToWorld2D( blockRasterX, blockRasterY, block->TextureInfo.WorldPosX, block->TextureInfo.WorldPosY );
   m_storage->RasterToWorld2D( blockRasterX + blockRasterSizeX, blockRasterY + blockRasterSizeY, block->TextureInfo.WorldSizeX, block->TextureInfo.WorldSizeY );
   block->TextureInfo.WorldSizeX = block->TextureInfo.WorldSizeX - block->TextureInfo.WorldPosX;
   block->TextureInfo.WorldSizeY = block->TextureInfo.WorldSizeY - block->TextureInfo.WorldPosY;

   // Source heightmap dimensions limits
   int maxRasterX = m_rawSourceData->Width()-1;
   int maxRasterY = m_rawSourceData->Height()-1;

   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // Extract edge points
   block->CornerPointsSizeX = (block->TextureInfo.Width + exportSettings.LeafQuadTreeNodeSize - 1) / exportSettings.LeafQuadTreeNodeSize;
   block->CornerPointsSizeY = (block->TextureInfo.Height + exportSettings.LeafQuadTreeNodeSize - 1) / exportSettings.LeafQuadTreeNodeSize;
   block->CornerPoints = new uint16[block->CornerPointsSizeX * block->CornerPointsSizeY];
   for( int y = 0; y < block->CornerPointsSizeY; y++ )
   {
      int hmY = blockRasterY + y * stepSize * exportSettings.LeafQuadTreeNodeSize;

      hmY = ::min( hmY, maxRasterY );
      uint16 * edgePtsScanLine = &block->CornerPoints[block->CornerPointsSizeX * y];

      for( int x = 0; x < block->CornerPointsSizeX; x++ )
      {
         int hmX = blockRasterX + x * stepSize * exportSettings.LeafQuadTreeNodeSize;
         hmX = ::min( hmX, maxRasterX );

         uint16 pixel;
         m_rawSourceData->GetPixel( hmX, hmY, &pixel );

         edgePtsScanLine[x] = pixel;
      }
   }
   ///////////////////////////////////////////////////////////////////////////////////////////////////

   SaveHeightmapBlockInfo( outStream, *block );

   int32 bufferSize = block->TextureInfo.Width * block->TextureInfo.Height * sizeof(uint16);
   uint16 * buffer = new uint16[block->TextureInfo.Width * block->TextureInfo.Height];

   for( int y = 0; y < block->TextureInfo.Height; y++ )
   {
      int hmY = blockRasterY + y * stepSize;

      hmY = ::min( hmY, maxRasterY );

      uint16 * bufferScanLine = &buffer[block->TextureInfo.Width * y];

      for( int x = 0; x < block->TextureInfo.Width; x++ )
      {
         int hmX = blockRasterX + x * stepSize;
         hmX = ::min( hmX, maxRasterX );

         uint16 pixel;
         m_rawSourceData->GetPixel( hmX, hmY, &pixel );

         bufferScanLine[x] = pixel;
      }
   }

   assert( bufferSize != 0 );

   if( !outStream->WriteValue<int32>(bufferSize) )             vaFatalError( "Unable to save heightmap block" );

   if( outStream->Write( buffer, bufferSize ) != bufferSize )  vaFatalError( "Unable to save heightmap block" );

   delete[] buffer;
}

void CDLODStreamingTypeHeightmap::LoadHeaderAndInitialize( VertexAsylum::vaStream * inStream )
{
   CDLODStreamingTypeTextureBase::LoadHeaderAndInitialize( inStream );

   m_standardizedTextureFormat = vaGetVertexTextureFormat();
   m_standardizedTextureSizeInMemory = (m_standardizedTextureFormat==D3DFMT_UNKNOWN)?(0):
      (vaCalcApproxTextureMemSize( m_standardizedTextureFormat, 
      m_standardizedTextureSizeX, m_standardizedTextureSizeY, m_standardizedTextureMipCount ));
}

void CDLODStreamingTypeHeightmap::SaveHeightmapBlockInfo( VertexAsylum::vaStream * outStream, HeightmapBlock & inBlock )
{
   // version
   if( !outStream->WriteValue<int32>( 1 ) )
      vaFatalError("Unable to write heightmap block");

   if( !outStream->WriteValue<int32>( inBlock.CornerPointsSizeX ) )       vaFatalError("Unable to write heightmap block");
   if( !outStream->WriteValue<int32>( inBlock.CornerPointsSizeY ) )       vaFatalError("Unable to write heightmap block");
   if( inBlock.CornerPointsSizeX * inBlock.CornerPointsSizeY > 0 )
   {
      assert( inBlock.CornerPoints != NULL );
      int size = inBlock.CornerPointsSizeX * inBlock.CornerPointsSizeY * 2;
      if( outStream->Write( inBlock.CornerPoints, size ) != size )        vaFatalError("Unable to write heightmap block");
   }

   SaveTextureBlockInfo( outStream, *((TextureBlock*)&inBlock) );
}

int32 CDLODStreamingTypeHeightmap::LoadHeightmapBlockInfo( byte * dataBuffer, int dataBufferSize, HeightmapBlock & outBlock )
{
   int32 sizeToRead = 0;

   int32 version = *((int32*)&dataBuffer[0]); dataBuffer += 4; sizeToRead += 4;
   if( version != 1 )
      vaFatalError("Unable to read texture block: incorrect version");

   outBlock.CornerPointsSizeX = *((int32*)&dataBuffer[0]); dataBuffer += 4; sizeToRead += 4;
   outBlock.CornerPointsSizeY = *((int32*)&dataBuffer[0]); dataBuffer += 4; sizeToRead += 4;
   assert( sizeToRead <= dataBufferSize );
   if( outBlock.CornerPointsSizeX * outBlock.CornerPointsSizeY > 0 )
   {
      int size = outBlock.CornerPointsSizeX * outBlock.CornerPointsSizeY * 2;
      outBlock.CornerPoints = new uint16[outBlock.CornerPointsSizeX * outBlock.CornerPointsSizeY];
      sizeToRead += size;
      assert( sizeToRead <= dataBufferSize );
      memcpy( outBlock.CornerPoints, dataBuffer, size );
      dataBuffer += size;
   }

   sizeToRead += LoadTextureBlockInfo( dataBuffer, dataBufferSize, *((TextureBlock*)&outBlock) );

   return sizeToRead;
}

bool CDLODStreamingTypeHeightmap::StreamingReadBlockData( byte * dataBuffer, int dataBufferSize, Block * block )
{
   HeightmapBlock & heightmapBlock = *static_cast<HeightmapBlock *>(block);

   int32 bytesRead = LoadHeightmapBlockInfo( dataBuffer, dataBufferSize, heightmapBlock );

   dataBuffer += bytesRead;
   dataBufferSize -= bytesRead;

   int32 subBufferSize = *((int32*)&dataBuffer[0]);

   if( (dataBufferSize-4) < subBufferSize )             vaFatalError( "Unable to read heightmap block" );

   uint16 * buffer = (uint16*)&dataBuffer[4];

   // standardize texture size: this should ideally be so that no data gets added to the extended size to help with compression
   if( ((heightmapBlock.TextureInfo.Width) > m_standardizedTextureSizeX) || ((heightmapBlock.TextureInfo.Height) > m_standardizedTextureSizeY) )
      vaFatalError( "Incorrect standardized texture size calculation" );

   assert( (m_standardizedTextureFormat == D3DFMT_R32F) || (m_standardizedTextureFormat == D3DFMT_L16) );

   D3DLOCKED_RECT heightmapLockedRect; 

   if( heightmapBlock.RTexture == NULL )
      return false;

   assert( heightmapBlock.RTexture->IsLocked() );
   
   heightmapLockedRect = heightmapBlock.RTexture->LockedRects[0];

   if( m_standardizedTextureFormat == D3DFMT_L16 )
   {
      uint16 * heightmapTextureData = (uint16 *)heightmapLockedRect.pBits;

      for( int y = 0; y < m_standardizedTextureSizeY; y++ )
      {
         int srcY = ::min( y, heightmapBlock.TextureInfo.Height-1 );
         uint16 * bufferScanLine = &buffer[heightmapBlock.TextureInfo.Width * srcY];

         for( int x = 0; x < m_standardizedTextureSizeX; x++ )
         {
            int srcX = ::min( x, heightmapBlock.TextureInfo.Width-1 );
            heightmapTextureData[x] = bufferScanLine[srcX];
         }
         heightmapTextureData += heightmapLockedRect.Pitch / sizeof(*heightmapTextureData);
      }
   }
   else if( m_standardizedTextureFormat == D3DFMT_R32F )
   {
      float * heightmapTextureData = (float *)heightmapLockedRect.pBits;

      for( int y = 0; y < m_standardizedTextureSizeY; y++ )
      {
         int srcY = ::min( y, heightmapBlock.TextureInfo.Height-1 );
         uint16 * bufferScanLine = &buffer[heightmapBlock.TextureInfo.Width * srcY];

         for( int x = 0; x < m_standardizedTextureSizeX; x++ )
         {
            int srcX = ::min( x, heightmapBlock.TextureInfo.Width-1 );
            heightmapTextureData[x] = bufferScanLine[srcX] / 65535.0f;
         }
         heightmapTextureData += heightmapLockedRect.Pitch / sizeof(*heightmapTextureData);
      }
   }
   else
   {
      assert( false );
   }

   heightmapBlock.TextureInfo.WorldSizeX *= (float)(m_standardizedTextureSizeX-1) / (float)(heightmapBlock.TextureInfo.Width-1);
   heightmapBlock.TextureInfo.WorldSizeY *= (float)(m_standardizedTextureSizeY-1) / (float)(heightmapBlock.TextureInfo.Height-1);
   heightmapBlock.TextureInfo.Width   = m_standardizedTextureSizeX;
   heightmapBlock.TextureInfo.Height  = m_standardizedTextureSizeY;
   return true;
}
