///////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "..\Common.h"

#include "CDLODStreamingTypeBase.h"

#include "../DxEventNotifier.h"

class VertexAsylum::TiledBitmap;

#include <queue>


///////////////////////////////////////////////////////////////////////////////////////////////////
// Handles heightmap streaming and processing
///////////////////////////////////////////////////////////////////////////////////////////////////

class CDLODStreamingTypeTextureBase : public CDLODStreamingTypeBase, protected DxEventReceiver
{
protected:
   struct ReusableTexture
   {
      CDLODStreamingTypeTextureBase *  Parent;
      IDirect3DTexture9 *              Texture;
      D3DSURFACE_DESC                  SurfaceDescs[16];
      D3DLOCKED_RECT                   LockedRects[16];
      int                              MipCount;

      ReusableTexture( CDLODStreamingTypeTextureBase * parent )    { Parent = parent; Texture = NULL; LockedRects[0].pBits = NULL; MipCount = 0; }
      ~ReusableTexture()                                          { SAFE_RELEASE( Texture ); }

      bool                       IsLocked()        { return LockedRects[0].pBits != NULL; }
      void                       LockLevels();
      void                       UnlockLevels();

      void                       Release()         { Parent->TexturePoolRelease( this ); }
   };

public:

   struct TextureBlockRenderInfo
   {
      // Texture width and height
      int                        Width;
      int                        Height;

      // World space covered by this texture.
      float                      WorldPosX;
      float                      WorldPosY;
      float                      WorldSizeX;
      float                      WorldSizeY;
   };

   struct TextureBlock : public CDLODStreamingTypeBase::Block
   {
      IDirect3DTexture9 *        Texture;
      TextureBlockRenderInfo     TextureInfo;

      ReusableTexture *          RTexture;

      virtual void OnAllocate()     { Texture = NULL; RTexture = NULL; }
      virtual void OnDeallocate();

      virtual void   GetWorldCoverage( AABB & boundingBox )    
      { 
         boundingBox.Min = D3DXVECTOR3(TextureInfo.WorldPosX, TextureInfo.WorldPosY, 0); 
         boundingBox.Max = boundingBox.Min + D3DXVECTOR3(TextureInfo.WorldSizeX, TextureInfo.WorldSizeY, 0); 
      }
   };

private:
   
   std::queue<ReusableTexture*>     m_texturePool;
   const float                      m_texturePoolMaxReserveCountRatio;
   int                              m_texturePoolMaxReserveCount;
   int                              m_texturePoolCurrentTotalAllocatedCount;
   int                              m_texturePoolCurrentReserveCount;
   int                              m_texturePoolCurrentInUseCount;

protected:

   VertexAsylum::TiledBitmap *      m_rawSourceData;

   // scale compared to heightmap raster size
   float                            m_texelScaleX;
   float                            m_texelScaleY;

   // actual size of the texture that will store the block data - standardized to one size to simplify streaming, but can also
   // be used to enforce pow-of-2 sizes if needed
   int                              m_standardizedTextureSizeX;
   int                              m_standardizedTextureSizeY;
   int                              m_standardizedTextureMipCount;
   D3DFORMAT                        m_standardizedTextureFormat;

   int                              m_standardizedTextureSizeInMemory;


public:
   CDLODStreamingTypeTextureBase( CDLODStreamingStorage * storage, int blockSize, int LODOffset, uint32 arrayIndex, 
                                 VertexAsylum::TiledBitmap * rawSourceData, BlockAllocatorInterface * allocatorPtr = NULL );
   CDLODStreamingTypeTextureBase( CDLODStreamingStorage * storage, uint32 arrayIndex, int texturePoolReserveMemory, 
                                 BlockAllocatorInterface * allocatorPtr = NULL );
   virtual ~CDLODStreamingTypeTextureBase( );

   void                       GetCurrentlyAllocatedMemory( uint32 & outDataMemorySize, uint32 & outCacheMemorySize );

protected:

   void                       SaveTextureBlockInfo( VertexAsylum::vaStream * outStream, TextureBlock & inBlock );
   int32                      LoadTextureBlockInfo( byte * dataBuffer, int dataBufferSize, TextureBlock & outBlock );

   void                       TexturePoolClear();
   ReusableTexture *          TexturePoolAllocate();
   void                       TexturePoolRelease( CDLODStreamingTypeTextureBase::ReusableTexture * rtexture );
   void                       TexturePoolRecalcMaxReserve();

   // DxEventReceiver
   void                       OnLostDevice()                         { TexturePoolClear(); }
   void                       OnDestroyDevice()                      { TexturePoolClear(); }

public:
   virtual void               ExportBlockForQuad( VertexAsylum::vaStream * outStream, CDLODStreamingTypeBase::Block * block, 
                                                   const int quadRasterX, const int quadRasterY, const int quadSize, const int quadLODLevel, 
                                                   const ExportSettings & exportSettings );
   virtual void               FillTexture( TextureBlock * block, int imgRasterX, int imgRasterY, int imgRasterSizeX, int imgRasterSizeY,
                                             int stepSize, int mipLevels, const ExportSettings & exportSettings ) { assert( false ); } ;

   virtual void               SaveHeader( VertexAsylum::vaStream * outStream, bool justAllocatePass );
   virtual void               LoadHeaderAndInitialize( VertexAsylum::vaStream * inStream );
   
   // called from the main thread when the streaming is initiated
   virtual bool               StreamingInitialize( Block * block );
   // called from the main thread when the streaming was finished
   virtual bool               StreamingFinalize( Block * block );

protected:
   // called from the streaming thread to process the buffer (after decompression, if there was any)
   virtual bool               StreamingReadBlockData( byte * dataBuffer, int dataBufferSize, Block * block );
};

