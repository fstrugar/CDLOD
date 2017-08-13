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
// Overrides CDLODStreamingTypeTextureBase and replaces some of its functionality.
///////////////////////////////////////////////////////////////////////////////////////////////////

class CDLODStreamingTypeOverlayImg : public CDLODStreamingTypeTextureBase
{
public:
   CDLODStreamingTypeOverlayImg( CDLODStreamingStorage * storage, int blockSize, int LODOffset, int32 arrayIndex, 
                                 VertexAsylum::TiledBitmap * rawSourceData, const ExportSettings & exportSettings );
   CDLODStreamingTypeOverlayImg( CDLODStreamingStorage * storage, int32 arrayIndex, int texturePoolReserveMemory );
   virtual ~CDLODStreamingTypeOverlayImg( );

protected:
   virtual void               FillTexture( TextureBlock * block, int imgRasterX, int imgRasterY, int imgRasterSizeX, int imgRasterSizeY,
                                             int stepSize, int mipLevels, const ExportSettings & exportSettings );

   virtual StorageTypeID      GetStorageTypeID()   { return STID_Overlaymap; }
};