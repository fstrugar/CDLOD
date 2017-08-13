//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////


#include "CDLODStreamingTypeTextureBase.h"

#include "CDLODStreamingStorage.h"

#include "..\TiledBitmap\TiledBitmap.h"

#include "Core/IO/vaStream.h"

#include "..\DxEventNotifier.h"

#include "..\CDLODTools.h"


static const int c_mipStopAt = 64;

#define USE_MANAGED_DIRECTX_TEXTURES (1)

CDLODStreamingTypeTextureBase::CDLODStreamingTypeTextureBase( CDLODStreamingStorage * storage, int blockSize, int LODOffset, 
                                                           uint32 arrayIndex, VertexAsylum::TiledBitmap * rawSourceData, 
                                                           BlockAllocatorInterface * allocatorPtr )
 : CDLODStreamingTypeBase( storage, (allocatorPtr != NULL)?(allocatorPtr):(new SimpleAllocator<TextureBlock>()), blockSize, LODOffset, arrayIndex ),
      m_texturePoolMaxReserveCount(0), m_texturePoolCurrentTotalAllocatedCount(0), 
      m_texturePoolCurrentReserveCount(0), m_texturePoolCurrentInUseCount(0),
      m_texturePoolMaxReserveCountRatio(0.0f), m_standardizedTextureFormat(D3DFMT_UNKNOWN)
{
   m_rawSourceData = rawSourceData;
   m_texelScaleX = 0;
   m_texelScaleY = 0;

   // Allow this for CDLODStreamingTypeHeightmap which reuses only portions of CDLODStreamingTypeTextureBase algorithms
   // and does the rest in a custom way.
   if( m_rawSourceData != NULL )
   {
      int worldTexRasterX = m_storage->GetWorldDesc().RasterSizeX-1;
      int worldTexRasterY = m_storage->GetWorldDesc().RasterSizeY-1;

      // dimensions must be integer multiple or integer divisible
      assert( ((m_rawSourceData->Width() % worldTexRasterX) == 0) || ((worldTexRasterX % m_rawSourceData->Width()) == 0) );
      assert( ((m_rawSourceData->Height() % worldTexRasterY) == 0) || ((worldTexRasterY % m_rawSourceData->Height()) == 0) );

      if( !( (((m_rawSourceData->Width() % worldTexRasterX) == 0) || ((worldTexRasterX % m_rawSourceData->Width()) == 0) ))
         && (((m_rawSourceData->Height() % worldTexRasterY) == 0) || ((worldTexRasterY % m_rawSourceData->Height()) == 0) ) )
      {
         vaFatalError("Unsupported dimensions");
      }

      m_texelScaleX = m_rawSourceData->Width() / (float)worldTexRasterX;
      m_texelScaleY = m_rawSourceData->Height() / (float)worldTexRasterY;

      const int maxTextureSizeX = (int)(m_texelScaleX * m_blockSize);
      const int maxTextureSizeY = (int)(m_texelScaleY * m_blockSize);

      // lowest mip level will have its biggest dimension of this size
      m_standardizedTextureMipCount = ::max( 1, vaLog2( ::max(maxTextureSizeX, maxTextureSizeY) ) - vaLog2( c_mipStopAt ) );

      ///////////////////////////////////////////////////////////////////////////////////////////////////
      // To eliminate seams between different texture blocks that appear due to linear filtering
      // when sampling the edges, we need to expand edges by the border determined by mipLevel count.
      // Account for DXT1/DXT5 block limitations.
      const int borderRequired = 2 << (m_standardizedTextureMipCount-1);
      ///////////////////////////////////////////////////////////////////////////////////////////////////

      m_standardizedTextureSizeX = maxTextureSizeX + borderRequired * 2;
      m_standardizedTextureSizeY = maxTextureSizeY + borderRequired * 2;
   }

   m_standardizedTextureSizeInMemory = 0;
}

CDLODStreamingTypeTextureBase::CDLODStreamingTypeTextureBase( CDLODStreamingStorage * storage, uint32 arrayIndex, int texturePoolReserveMemory, 
                                                            BlockAllocatorInterface * allocatorPtr )
 : CDLODStreamingTypeBase( storage, (allocatorPtr != NULL)?(allocatorPtr):(new SimpleAllocator<TextureBlock>()), arrayIndex ),
      m_texturePoolMaxReserveCount(0), m_texturePoolCurrentTotalAllocatedCount(0), 
      m_texturePoolCurrentReserveCount(0), m_texturePoolCurrentInUseCount(0),
      m_texturePoolMaxReserveCountRatio(0.2f)
{
   m_rawSourceData = NULL;
   m_texelScaleX = 0;
   m_texelScaleY = 0;
}


CDLODStreamingTypeTextureBase::~CDLODStreamingTypeTextureBase( )
{
   TexturePoolClear();

   delete m_allocatorPtr;
}

void CDLODStreamingTypeTextureBase::ReusableTexture::LockLevels()
{
   assert( !IsLocked() );

   HRESULT hr;

   for( int i = 0; i < MipCount; i++ )
   {
#if defined( USE_MANAGED_DIRECTX_TEXTURES )
      V( Texture->LockRect( i, &LockedRects[i], NULL, (i==0)?(0):(0) ) );
#else
      V( Texture->LockRect( i, &LockedRects[i], NULL, (i==0)?(D3DLOCK_DISCARD):(0) ) );
#endif
   }
}

void CDLODStreamingTypeTextureBase::ReusableTexture::UnlockLevels()
{
   assert( IsLocked() );

   HRESULT hr;

   for( int i = MipCount-1; i >= 0; i-- )
   {
      if( !DxEventNotifier::IsD3DDeviceLost() ) 
      {
         V( Texture->UnlockRect( i ) );
      }
      LockedRects[i].pBits = NULL;
      LockedRects[i].Pitch = 0;
   }
}

void CDLODStreamingTypeTextureBase::TextureBlock::OnDeallocate()   
{ 
   assert((Flags & BF_LoadingFinished) == 0); 
   assert((Flags & BF_Loading) == 0); 
   if( RTexture != NULL )
   {
      RTexture->Release();
      RTexture = NULL;
      Texture = NULL;
   }
   SAFE_RELEASE( Texture );
}

void CDLODStreamingTypeTextureBase::TexturePoolClear()
{   
   while( m_texturePool.size() > 0 )
   {
      ReusableTexture * texture = m_texturePool.front();
      m_texturePool.pop();
      delete texture;
      m_texturePoolCurrentTotalAllocatedCount--;
      m_texturePoolCurrentReserveCount--;
   }
}

void CDLODStreamingTypeTextureBase::TexturePoolRecalcMaxReserve()
{
   int maxReserveCountTarget        = (int)ceilf(m_texturePoolCurrentInUseCount * m_texturePoolMaxReserveCountRatio);
   int maxReserveCountTargetLimit   = (int)ceilf(m_texturePoolCurrentInUseCount * (m_texturePoolMaxReserveCountRatio*2.0f));

   m_texturePoolMaxReserveCount = ::max( m_texturePoolMaxReserveCount, maxReserveCountTarget );
   m_texturePoolMaxReserveCount = ::min( m_texturePoolMaxReserveCount, maxReserveCountTargetLimit );
}

CDLODStreamingTypeTextureBase::ReusableTexture * CDLODStreamingTypeTextureBase::TexturePoolAllocate()
{
   m_texturePoolCurrentInUseCount++;

   ReusableTexture * rtexture = NULL;

   if( m_texturePool.size() > 0 )
   {
      rtexture = m_texturePool.front();
      m_texturePool.pop();
      m_texturePoolCurrentReserveCount--;
   }
   else
   {
      rtexture = new ReusableTexture(this);
      m_texturePoolCurrentTotalAllocatedCount++;

      HRESULT hr;
#if defined( USE_MANAGED_DIRECTX_TEXTURES )
      V( DxEventNotifier::GetD3DDevice()->CreateTexture( m_standardizedTextureSizeX, m_standardizedTextureSizeY, m_standardizedTextureMipCount, 
                                                         0, m_standardizedTextureFormat, D3DPOOL_MANAGED, &rtexture->Texture, NULL ) );
#else
      V( DxEventNotifier::GetD3DDevice()->CreateTexture( m_standardizedTextureSizeX, m_standardizedTextureSizeY, m_standardizedTextureMipCount, 
                                                         D3DUSAGE_DYNAMIC, m_standardizedTextureFormat, D3DPOOL_DEFAULT, &rtexture->Texture, NULL ) );
#endif
                                                         
      rtexture->MipCount = m_standardizedTextureMipCount;
      for( int i = 0; i < rtexture->MipCount; i++ )
         rtexture->Texture->GetLevelDesc( i, &rtexture->SurfaceDescs[i] );
   }

   assert( rtexture != NULL );
   assert( m_texturePoolCurrentReserveCount == (int)m_texturePool.size() );

   return rtexture;
}

void CDLODStreamingTypeTextureBase::TexturePoolRelease( CDLODStreamingTypeTextureBase::ReusableTexture * rtexture )
{
   m_texturePoolCurrentInUseCount--;

   TexturePoolRecalcMaxReserve();

   if( (m_texturePoolCurrentReserveCount >= m_texturePoolMaxReserveCount) || DxEventNotifier::IsD3DDeviceLost() )
   {
      delete rtexture;
      m_texturePoolCurrentTotalAllocatedCount--;
   }
   else
   {
      m_texturePoolCurrentReserveCount++;
      m_texturePool.push( rtexture );
      assert( !rtexture->IsLocked() );
   }

   assert( m_texturePoolCurrentReserveCount == (int)m_texturePool.size() );
}

void CDLODStreamingTypeTextureBase::ExportBlockForQuad( VertexAsylum::vaStream * outStream, CDLODStreamingTypeBase::Block * inBlock, 
                                                      const int quadRasterX, const int quadRasterY, const int quadRasterSize, const int quadLODLevel, 
                                                      const ExportSettings & exportSettings )
{
   // blockX and blockY are indices in the 2D array of blocks of this LOD level (or they would be, if there was an array)
   int blockX, blockY, blockLODLevel;

   // CDLODStreamingStorage will use our allocator so this cast should always be correct.
   TextureBlock * block = static_cast<TextureBlock *>(inBlock);

   DecodeBlockHashID( block->HashID, blockX, blockY, blockLODLevel );

   // Block coverage in (heightmap) raster space.
   int blockRasterX, blockRasterY, blockRasterSize;
   int stepSize = 1 << blockLODLevel;

   CalcBlockRasterCoverage( blockX, blockY, blockLODLevel, blockRasterX, blockRasterY, blockRasterSize );

   // Block size (in texels of our texture representing the block)
   int correctedBlockSizeX = m_blockSize;
   int correctedBlockSizeY = m_blockSize;

   // Limit our block texture size if it runs out of the source data dimensions
   {
      int maxTextureX = (m_storage->GetWorldDesc().RasterSizeX + stepSize - 1) / stepSize;
      int maxTextureY = (m_storage->GetWorldDesc().RasterSizeY + stepSize - 1) / stepSize;
      int ourBlockMaxTextureX = (blockX+1) * m_blockSize;
      int ourBlockMaxTextureY = (blockY+1) * m_blockSize;
      correctedBlockSizeX -= ::max( 0, ourBlockMaxTextureX - maxTextureX );
      correctedBlockSizeY -= ::max( 0, ourBlockMaxTextureY - maxTextureY );
   }

   block->Texture = NULL;
   int blockRasterSizeX = correctedBlockSizeX * stepSize;
   int blockRasterSizeY = correctedBlockSizeY * stepSize;
   m_storage->RasterToWorld2D( blockRasterX, blockRasterY, block->TextureInfo.WorldPosX, block->TextureInfo.WorldPosY );
   m_storage->RasterToWorld2D( blockRasterX + blockRasterSizeX, blockRasterY + blockRasterSizeY, block->TextureInfo.WorldSizeX, block->TextureInfo.WorldSizeY );
   block->TextureInfo.WorldSizeX = block->TextureInfo.WorldSizeX - block->TextureInfo.WorldPosX;
   block->TextureInfo.WorldSizeY = block->TextureInfo.WorldSizeY - block->TextureInfo.WorldPosY;

   int imgRasterX = (int)(blockRasterX * m_texelScaleX + 0.5f);
   int imgRasterY = (int)(blockRasterY * m_texelScaleY + 0.5f);
   int imgRasterSizeX = (int)(blockRasterSizeX * m_texelScaleX + 0.5f);
   int imgRasterSizeY = (int)(blockRasterSizeY * m_texelScaleY + 0.5f);

   block->TextureInfo.Width = imgRasterSizeX / stepSize;
   block->TextureInfo.Height = imgRasterSizeY / stepSize;

   if( block->TextureInfo.Width <= 0 )
      vaFatalError( "block->TextureInfo.Width <= 0 ?" );
   if( block->TextureInfo.Height <= 0 )
      vaFatalError( "block->TextureInfo.Height <= 0 ?" );
   
   // lowest mip level will have its biggest dimension of this size
   //int mipLevels = ::max( 1, vaLog2( ::max(block->TextureInfo.Width, block->TextureInfo.Height) ) - vaLog2( c_mipStopAt ) );
   int mipLevels = m_standardizedTextureMipCount;

   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // to eliminate seams between different texture blocks that appear due to linear filtering
   // when sampling the edges, we need to expand edges by the border determined by mipLevel count.
   const int borderRequired = 1 << (mipLevels-1);
   ///////////////////////////////////////////////////////////////////////////////////////////////////

   float originalTexelSizeX = block->TextureInfo.WorldSizeX / (float)block->TextureInfo.Width;
   float originalTexelSizeY = block->TextureInfo.WorldSizeY / (float)block->TextureInfo.Height;

   // how much to expand
   int expandedLeft     = 0;
   int expandedTop      = 0;
   int expandedRight    = 0;
   int expandedBottom   = 0;

   // how much can we expand
   int luftLeft   = imgRasterX / stepSize;
   int luftTop    = imgRasterY / stepSize;
   int luftRight  = ::max( 0, ((m_rawSourceData->Width() - (imgRasterX+imgRasterSizeX)) / stepSize) );
   int luftBottom = ::max( 0, ((m_rawSourceData->Height() - (imgRasterY+imgRasterSizeY)) / stepSize) );

   // expand for edges to have perfect matching next n mip levels
   expandedLeft   = ::min( borderRequired, luftLeft );
   expandedTop    = ::min( borderRequired, luftTop );
   expandedRight  = ::min( borderRequired, luftRight );
   expandedBottom = ::min( borderRequired, luftBottom );

   const int divFix = 4;
   // expand to always be divisible by 4 (some common texture formats like DXT1 require this)
   expandedRight += (block->TextureInfo.Width + expandedLeft + expandedRight + divFix - 1) / divFix * divFix - (block->TextureInfo.Width + expandedLeft + expandedRight);
   expandedBottom += (block->TextureInfo.Height + expandedTop + expandedBottom + divFix - 1) / divFix * divFix - (block->TextureInfo.Height + expandedTop + expandedBottom);

   int expandedWidth = expandedLeft + expandedRight;
   int expandedHeight = expandedTop + expandedBottom;

   // standardize texture size: this should ideally be so that no data gets added to the extended size to help with compression
   if( ((expandedWidth + block->TextureInfo.Width) > m_standardizedTextureSizeX) ||
       ((expandedHeight + block->TextureInfo.Height) > m_standardizedTextureSizeY) )
       vaFatalError( "Incorrect standardized texture size calculation" );

   int expandToStandardWidth  = ::max(0, m_standardizedTextureSizeX - (expandedWidth + block->TextureInfo.Width) );
   int expandToStandardHeight = ::max(0, m_standardizedTextureSizeY - (expandedHeight + block->TextureInfo.Height) );

   expandedWidth  += expandToStandardWidth;
   expandedHeight += expandToStandardHeight;

   // TODO: mark expandToStandardWidth and expandToStandardHeight as fill-with-0 texels as they
   // are irrelevant and should be set to 0 for consistency, debugging and compression performance

   imgRasterX -= expandedLeft * stepSize;
   imgRasterY -= expandedTop * stepSize;
   imgRasterSizeX += expandedWidth * stepSize;
   imgRasterSizeY += expandedHeight * stepSize;

   block->TextureInfo.Width   += expandedWidth;
   block->TextureInfo.Height  += expandedHeight;
   block->TextureInfo.WorldPosX -= expandedLeft * originalTexelSizeX;
   block->TextureInfo.WorldPosY -= expandedTop * originalTexelSizeY;
   block->TextureInfo.WorldSizeX += expandedWidth * originalTexelSizeX;
   block->TextureInfo.WorldSizeY += expandedHeight * originalTexelSizeY;

   FillTexture( block, imgRasterX, imgRasterY, imgRasterSizeX, imgRasterSizeY, stepSize, mipLevels, exportSettings );

   SaveTextureBlockInfo( outStream, *block );
   if( FAILED( vaSaveTexture( outStream, block->Texture ) ) )
      vaFatalError( "Unable to save texture" );
}

void CDLODStreamingTypeTextureBase::SaveHeader( VertexAsylum::vaStream * outStream, bool justAllocatePass )
{
   // Version
   if( !outStream->WriteValue<int32>(3) )
      vaFatalError( "Unable to save CDLODStreamingTypeTextureBase header" );

   if( !outStream->WriteValue<int32>(m_rawSourceData->Width()) )
      vaFatalError( "Unable to save CDLODStreamingTypeTextureBase header" );

   if( !outStream->WriteValue<int32>(m_rawSourceData->Height()) )
      vaFatalError( "Unable to save CDLODStreamingTypeTextureBase header" );

   if( !outStream->WriteValue<float>(m_texelScaleX) )
      vaFatalError( "Unable to save CDLODStreamingTypeTextureBase header" );

   if( !outStream->WriteValue<float>(m_texelScaleY) )
      vaFatalError( "Unable to save CDLODStreamingTypeTextureBase header" );

   if( !outStream->WriteValue<int32>(m_standardizedTextureSizeX) )
      vaFatalError( "Unable to save CDLODStreamingTypeTextureBase header" );

   if( !outStream->WriteValue<int32>(m_standardizedTextureSizeY) )
      vaFatalError( "Unable to save CDLODStreamingTypeTextureBase header" );

   if( !outStream->WriteValue<int32>(m_standardizedTextureMipCount) )
      vaFatalError( "Unable to save CDLODStreamingTypeTextureBase header" );

   if( !outStream->WriteValue<uint32>((uint32)m_standardizedTextureFormat) )
      vaFatalError( "Unable to save CDLODStreamingTypeTextureBase header" );

   CDLODStreamingTypeBase::SaveHeader( outStream, justAllocatePass );
}

void CDLODStreamingTypeTextureBase::LoadHeaderAndInitialize( VertexAsylum::vaStream * inStream )
{
   int version;
   if( !inStream->ReadValue<int32>(version) )
      vaFatalError( "Unable to load CDLODStreamingTypeTextureBase header" );

   if( version != 3 )
      vaFatalError( "Unable to load CDLODStreamingTypeTextureBase header - invalid version" );

   int rawSourceDataWidth;
   if( !inStream->ReadValue<int32>(rawSourceDataWidth) )
      vaFatalError( "Unable to load CDLODStreamingTypeTextureBase header" );

   int rawSourceDataHeight;
   if( !inStream->ReadValue<int32>(rawSourceDataHeight) )
      vaFatalError( "Unable to load CDLODStreamingTypeTextureBase header" );

   if( !inStream->ReadValue<float>(m_texelScaleX) )
      vaFatalError( "Unable to load CDLODStreamingTypeTextureBase header" );

   if( !inStream->ReadValue<float>(m_texelScaleY) )
      vaFatalError( "Unable to load CDLODStreamingTypeTextureBase header" );

   if( !inStream->ReadValue<int32>(m_standardizedTextureSizeX) )
      vaFatalError( "Unable to load CDLODStreamingTypeTextureBase header" );

   if( !inStream->ReadValue<int32>(m_standardizedTextureSizeY) )
      vaFatalError( "Unable to load CDLODStreamingTypeTextureBase header" );

   if( !inStream->ReadValue<int32>(m_standardizedTextureMipCount) )
      vaFatalError( "Unable to load CDLODStreamingTypeTextureBase header" );

   uint32 fmt;
   if( !inStream->ReadValue<uint32>(fmt) )
      vaFatalError( "Unable to load CDLODStreamingTypeTextureBase header" );
   m_standardizedTextureFormat = (D3DFORMAT)fmt;

   m_standardizedTextureSizeInMemory = (m_standardizedTextureFormat==D3DFMT_UNKNOWN)?(0):
      (vaCalcApproxTextureMemSize( m_standardizedTextureFormat, 
      m_standardizedTextureSizeX, m_standardizedTextureSizeY, m_standardizedTextureMipCount ));

   CDLODStreamingTypeBase::LoadHeaderAndInitialize( inStream );
}

void CDLODStreamingTypeTextureBase::SaveTextureBlockInfo( VertexAsylum::vaStream * outStream, TextureBlock & inBlock )
{
   // version
   if( !outStream->WriteValue<int32>( 1 ) )
      vaFatalError("Unable to write texture block");
   if( !outStream->WriteValue<TextureBlockRenderInfo>( inBlock.TextureInfo ) )
      vaFatalError("Unable to write texture block");
}

int32 CDLODStreamingTypeTextureBase::LoadTextureBlockInfo( byte * dataBuffer, int dataBufferSize, TextureBlock & outBlock )
{
   static const int32 sizeToRead = (sizeof(int32)+sizeof(TextureBlockRenderInfo));
   assert( dataBufferSize >= sizeToRead );
   if( dataBufferSize < sizeToRead )
      vaFatalError("Unable to read texture block");

   int32 version = *((int32*)&dataBuffer[0]);
   if( version != 1 )
      vaFatalError("Unable to read texture block: incorrect version");

   outBlock.TextureInfo = *((TextureBlockRenderInfo*)&dataBuffer[4]);
   
   if( ((outBlock.TextureInfo.Height == 0) || (outBlock.TextureInfo.Height > 8192))
         || ((outBlock.TextureInfo.Width == 0) || (outBlock.TextureInfo.Width > 8192)) )
      vaFatalError("Unable to read texture block: incorrect TextureBlockRenderInfo values");

   return sizeToRead;
}

bool CDLODStreamingTypeTextureBase::StreamingReadBlockData( byte * dataBuffer, int dataBufferSize, Block * block )
{
   TextureBlock & textureBlock = *static_cast<TextureBlock *>(block);
   
   int32 bytesRead = LoadTextureBlockInfo( dataBuffer, dataBufferSize, textureBlock );

   dataBuffer += bytesRead;
   dataBufferSize -= bytesRead;

   assert( textureBlock.RTexture != NULL );
   if( textureBlock.RTexture == NULL )
      return false;
   assert( textureBlock.RTexture->IsLocked() );
   if( !vaLoadTexture( dataBuffer, dataBufferSize, m_standardizedTextureFormat, m_standardizedTextureMipCount,
                        textureBlock.RTexture->SurfaceDescs, textureBlock.RTexture->LockedRects, true ) )
   {
      vaFatalError( "Unable to read texture" );
      return false;
   }

   return true;
}

bool CDLODStreamingTypeTextureBase::StreamingInitialize( Block * block )
{
   TextureBlock & textureBlock = *static_cast<TextureBlock *>(block);

   assert( textureBlock.RTexture == NULL );
   
   textureBlock.RTexture = TexturePoolAllocate();
   textureBlock.RTexture->LockLevels();

   return textureBlock.RTexture != NULL;
}

bool CDLODStreamingTypeTextureBase::StreamingFinalize( Block * block )
{
   TextureBlock & textureBlock = *static_cast<TextureBlock *>(block);

   if( textureBlock.RTexture != NULL )
   {
      assert( textureBlock.Texture == NULL );

      assert( textureBlock.RTexture->IsLocked() );
      textureBlock.RTexture->UnlockLevels();
      textureBlock.Texture = textureBlock.RTexture->Texture;
   }

   return true;
}

void CDLODStreamingTypeTextureBase::GetCurrentlyAllocatedMemory( uint32 & outDataMemorySize, uint32 & outCacheMemorySize )
{
   int blockCount = GetCurrentlyAllocatedBlockCount();
   outDataMemorySize    = blockCount * m_standardizedTextureSizeInMemory;
   outCacheMemorySize   = m_texturePool.size() * m_standardizedTextureSizeInMemory;
}