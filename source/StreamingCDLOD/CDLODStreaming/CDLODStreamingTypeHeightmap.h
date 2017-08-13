///////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "..\Common.h"

#include "CDLODStreamingTypeTextureBase.h"

class VertexAsylum::TiledBitmap;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Handles heightmap streaming and processing
// Overrides CDLODStreamingTypeTextureBase and modifies/replaces some of its functionality as 
// it has slightly non-standard requirements.
///////////////////////////////////////////////////////////////////////////////////////////////////

class CDLODStreamingTypeHeightmap : public CDLODStreamingTypeTextureBase
{
public:
   struct HeightmapBlock : public CDLODStreamingTypeTextureBase::TextureBlock
   {
      uint16 *    CornerPoints;
      int         CornerPointsSizeX;
      int         CornerPointsSizeY;

      virtual void OnAllocate()     
      { CornerPoints = NULL; CornerPointsSizeX = 0; CornerPointsSizeY = 0; CDLODStreamingTypeTextureBase::TextureBlock::OnAllocate(); }
      virtual void OnDeallocate();
   };

public:
   CDLODStreamingTypeHeightmap( CDLODStreamingStorage * storage, int blockSize, int LODOffset, 
                                 uint32 arrayIndex, VertexAsylum::TiledBitmap * rawSourceData );
   CDLODStreamingTypeHeightmap( CDLODStreamingStorage * storage, uint32 arrayIndex, int texturePoolReserveMemory );
   virtual ~CDLODStreamingTypeHeightmap( );

public:
   // completely override ReadBlockData as heightmap textures are handled in a custom way
   virtual void               ExportBlockForQuad( VertexAsylum::vaStream * outStream, CDLODStreamingTypeBase::Block * block, 
                                                   const int quadRasterX, const int quadRasterY, const int quadSize, const int quadLODLevel, 
                                                   const ExportSettings & exportSettings );

   virtual StorageTypeID      GetStorageTypeID()   { return STID_Heightmap; }

   // completely override ReadBlockData as heightmap textures are handled in a custom way
   virtual bool               StreamingReadBlockData( byte * dataBuffer, int dataBufferSize, Block * block );

   virtual void               LoadHeaderAndInitialize( VertexAsylum::vaStream * inStream );

private:
   void                       SaveHeightmapBlockInfo( VertexAsylum::vaStream * outStream, HeightmapBlock & inBlock );
   int32                      LoadHeightmapBlockInfo( byte * dataBuffer, int dataBufferSize, HeightmapBlock & outBlock );
};