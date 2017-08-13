///////////////////////////////////////////////////////////////////////////////////////////////////
// VertexAsylum Codebase, Copyright (c) Filip Strugar.
// Contents of this file are distributed under Zlib license.
// (http://en.wikipedia.org/wiki/Zlib_License)
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// Uses ZLIB compression library: see http://www.zlib.org for more info
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Core/vaCore.h"

namespace VertexAsylum
{
	class vaCompression
	{
   public:
      enum CompressionType
      {
         CT_None           = 0,
         CT_ZLIB           = 1,

         CT_Undefined      = -1,
      };

   public:

      static bool       SimplePack( CompressionType compressionType, byte * inData, int inDataSize, 
                                       byte * outData, int & outDataSize, bool useQuickLowQuality = false );

      static int32      SimpleUnpackJustGetSize( byte * inData, int inDataSize );
      static bool       SimpleUnpack( byte * inData, int inDataSize, byte * outData, int & outDataSize );
	};
}
