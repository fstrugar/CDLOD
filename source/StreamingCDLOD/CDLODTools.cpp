//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "CDLODTools.h"

#include "TiledBitmap/TiledBitmap.h"

#include "iniparser/src/IniParser.hpp"

#include "CDLODStreaming/CDLODStreamingStorage.h"

#include "Core/IO/vaStream.h"

#include "ProgressDlg.h"

using namespace std;

void vaGenerateMinMaxMap( VertexAsylum::TiledBitmap * heightmap, const int quadSize, unsigned short * outBuffer, int bufferSize )
{
   // Get min & max from all quads. Note, vertices from all quad edges are taken into account, so for a quadSize
   // of 2 that is 3x3=9 vertices!

   const int heightmapWidth = heightmap->Width();
   const int heightmapHeight = heightmap->Height();

   int buffDimX = (heightmapWidth - 1 + quadSize - 1) / quadSize;
   int buffDimY = (heightmapHeight - 1 + quadSize - 1) / quadSize;
   int buffMinSize = buffDimX * buffDimY * sizeof(unsigned short) * 2;
   assert( bufferSize >= buffMinSize );
   buffMinSize;

   unsigned short * subBlockBuffer = new unsigned short[ (quadSize+1) * (quadSize+1) ];

   for( int y = 0; y < buffDimY; y++ )
   {
      int subY = y * quadSize;
      int subSizeY = ::min( subY + quadSize + 1, heightmapHeight ) - subY;
      for( int x = 0; x < buffDimX; x++ )
      {
         int subX = x * quadSize;
         int subSizeX = ::min( subX + quadSize + 1, heightmapWidth ) - subX;

         heightmap->Read( subBlockBuffer, 2, (quadSize+1) * sizeof(unsigned short), subX, subY, subSizeX, subSizeY );

         unsigned short maxH = 0;
         unsigned short minH = 65535;
         for( int sy = 0; sy < subSizeY; sy++ )
            for( int sx = 0; sx < subSizeX; sx++ )
            {
               unsigned short h = subBlockBuffer[ sx + sy * (quadSize+1) ];
               maxH = ::max( maxH, h );
               minH = ::min( minH, h );
            }

            outBuffer[ (x + y * buffDimX)*2 + 0 ] = minH;
            outBuffer[ (x + y * buffDimX)*2 + 1 ] = maxH;
      }
   }

   delete[] subBlockBuffer;
}

void vaGenerateMinMaxMapLevels( VertexAsylum::TiledBitmap * heightmap, const int quadSize, int LODLevelCount, 
                                CDLODMinMaxMap minMaxMaps[], bool compressLast,
                                vaProgressReportProcPtr progressProc, float progressMul, float progressAdd )
{
   int baseDimX = (heightmap->Width() - 1 + quadSize - 1) / quadSize;
   int baseDimY = (heightmap->Height() - 1 + quadSize - 1) / quadSize;

   minMaxMaps[0].Initialize( baseDimX, baseDimY );

   vaGenerateMinMaxMap( heightmap, quadSize, minMaxMaps[0].GetData(), minMaxMaps[0].GetDataSizeInBytes() );

   if( progressProc != NULL ) progressProc( (1 / (float)(LODLevelCount-1)) * progressMul + progressAdd );

   for( int level = 1; level < LODLevelCount; level++ )
   {
      minMaxMaps[level].GenerateFromHigherDetailed(minMaxMaps[level-1]);
      if( progressProc != NULL ) progressProc( (level / (float)(LODLevelCount-1)) * progressMul + progressAdd );
   }

   if( compressLast && (LODLevelCount > 1) )
   {
      minMaxMaps[0].CompressBasedOnLowerDetailed( minMaxMaps[1] );
   }
}

bool vaDLODStreamingStorageCreateFromIni( CDLODStreamingStorage * storage, const IniParser & iniParser, const char * scdlodPath, void * metaDataBuffer, int metaDataBufferSize )
{
   if( storage == NULL )
   {
      vaFatalError( "storage == NULL" );
      return false;
   }

   CDLODStreamingStorage::WorldDesc       worldSettings;
   CDLODStreamingStorage::SourceDataDesc  sourceDataDesc;

   worldSettings.MapDims.MinX    = iniParser.getFloat("Main:MapDims_MinX", 0.0f);
   worldSettings.MapDims.MinY    = iniParser.getFloat("Main:MapDims_MinY", 0.0f);
   worldSettings.MapDims.MinZ    = iniParser.getFloat("Main:MapDims_MinZ", 0.0f);
   worldSettings.MapDims.SizeX   = iniParser.getFloat("Main:MapDims_SizeX", 0.0f);
   worldSettings.MapDims.SizeY   = iniParser.getFloat("Main:MapDims_SizeY", 0.0f);
   worldSettings.MapDims.SizeZ   = iniParser.getFloat("Main:MapDims_SizeZ", 0.0f);

   string heightmapPath = vaAddProjectDir( iniParser.getString( "SourceData:heightmapPath", "" ) );
   string normalmapPath = vaAddProjectDir( iniParser.getString( "SourceData:normalmapPath", "" ) );
   string overlaymapPath = vaAddProjectDir( iniParser.getString( "SourceData:overlaymapPath", "" ) );

   sourceDataDesc.Heightmap      = VertexAsylum::TiledBitmap::Open( heightmapPath.c_str(), true );
   if( sourceDataDesc.Heightmap == NULL )
   {
      vaFatalError("Unable to open heightmap source file.");
      return false;
   }

   sourceDataDesc.Normalmap      = NULL;
   if( normalmapPath != "" )
   {
      sourceDataDesc.Normalmap = VertexAsylum::TiledBitmap::Open( normalmapPath.c_str(), true );
      if( sourceDataDesc.Normalmap == NULL )
      {
         vaFatalError("Unable to open normalmap source file.");
         return false;
      }
   }
   sourceDataDesc.Overlaymap     = NULL;
   if( overlaymapPath != "" )
   {
      sourceDataDesc.Overlaymap = VertexAsylum::TiledBitmap::Open( overlaymapPath.c_str(), true );
      if( sourceDataDesc.Overlaymap == NULL )
      {
         vaFatalError("Unable to open overlaymap source file.");
         return false;
      }
   }
   sourceDataDesc.MetaDataBuffer = NULL;
   sourceDataDesc.MetaDataBufferSize = 0;
   sourceDataDesc.MetaDataBuffer       = metaDataBuffer;
   sourceDataDesc.MetaDataBufferSize   = metaDataBufferSize;

   worldSettings.RasterSizeX = sourceDataDesc.Heightmap->Width();
   worldSettings.RasterSizeY = sourceDataDesc.Heightmap->Height();

   worldSettings.LODLevelCount            = iniParser.getInt("CDLOD:LODLevelCount", 0);
   worldSettings.RenderGridResolutionMult = iniParser.getInt("CDLOD:RenderGridResolutionMult", 0);
   worldSettings.LeafQuadTreeNodeSize     = iniParser.getInt("CDLOD:LeafQuadTreeNodeSize", 0);
   worldSettings.LODLevelDistanceRatio    = iniParser.getFloat("CDLOD:LODLevelDistanceRatio", 0.0f);


   sourceDataDesc.HeightmapBlockSize   = iniParser.getInt("SourceData:HeightmapBlockSize", 0);
   sourceDataDesc.NormalmapBlockSize   = iniParser.getInt("SourceData:NormalmapBlockSize", 0);
   sourceDataDesc.OverlaymapBlockSize  = iniParser.getInt("SourceData:OverlaymapBlockSize", 0);
   sourceDataDesc.HeightmapLODOffset   = iniParser.getInt("SourceData:HeightmapLODOffset", 0);
   sourceDataDesc.NormalmapLODOffset   = iniParser.getInt("SourceData:NormalmapLODOffset", 0);
   sourceDataDesc.OverlaymapLODOffset  = iniParser.getInt("SourceData:OverlaymapLODOffset", 0);
   sourceDataDesc.NormalmapUseDXT5NMCompression = iniParser.getBool("SourceData:NormalmapUseDXT5NMCompression", true);
   sourceDataDesc.OverlaymapUseDXT1Compression  = iniParser.getBool("SourceData:OverlaymapUseDXT1Compression", true);
   sourceDataDesc.UseZLIBCompression   = iniParser.getBool("SourceData:UseZLIBCompression", true);

   CDLODStreamingStorage::ExportResults exportResults;

   wstring infoText = vaStringSimpleWiden( vaStringFormat( "Creating '%s'", scdlodPath ) );
   vaProgressWindowOpen( infoText.c_str() );
   bool retVal = storage->Create( worldSettings, sourceDataDesc, scdlodPath, vaProgressWindowUpdate, &exportResults );

   if( retVal )
   {
      wstring exportInfoStr = L"Export successful!\n\n";
      exportInfoStr += vaStringFormat( L"\nQuadtree size: %dKb", exportResults.QuadTreeStorageSize / 1024 );
      exportInfoStr += vaStringFormat( L"\n\nHeightmap size: %.2fMb", exportResults.HeightmapStorageSize / (1024.0f * 1024.0f) );
      exportInfoStr += vaStringFormat( L"\nNormalmap size: %.2fMb", exportResults.NormalmapStorageSize / (1024.0f * 1024.0f) );
      exportInfoStr += vaStringFormat( L"\nOverlaymap size: %.2fMb", exportResults.OverlaymapStorageSize / (1024.0f * 1024.0f) );
      exportInfoStr += vaStringFormat( L"\n\nTotal storage size: %.2fMb", exportResults.TotalStorageSize / (1024.0f * 1024.0f) );
      
      MessageBox( vaProgressWindowGetHWND(), exportInfoStr.c_str(), L"Exporter result", MB_OK );
   }
   else
   {
      MessageBox( vaProgressWindowGetHWND(), L"Error while exporting.", L"Exporter result", MB_OK );
   }

   vaProgressWindowClose();

   if( sourceDataDesc.Heightmap != NULL )    VertexAsylum::TiledBitmap::Close( sourceDataDesc.Heightmap );
   if( sourceDataDesc.Normalmap != NULL )    VertexAsylum::TiledBitmap::Close( sourceDataDesc.Normalmap );
   if( sourceDataDesc.Overlaymap != NULL )    VertexAsylum::TiledBitmap::Close( sourceDataDesc.Overlaymap );

   return retVal;
}

void vaGenerateNormalMap( VertexAsylum::TiledBitmap * heightmap, VertexAsylum::TiledBitmap * destinationNormalmap,
                         float mapSizeX, float mapSizeY, float mapSizeZ, bool showProgress )
{
   if( showProgress )
   {
      vaProgressWindowOpen( L"Creating normalmap" );
   }

   int processingBlockSize = 1024;

   int blockCountX = (heightmap->Width() + processingBlockSize - 1) / processingBlockSize;
   int blockCountY = (heightmap->Height() + processingBlockSize - 1) / processingBlockSize;

   const int border = 2;

   uint16 * sourceBuffer = new uint16[(processingBlockSize+border*2)*(processingBlockSize+border*2)];
   uint32 * destBuffer = new uint32[(processingBlockSize+border*2)*(processingBlockSize+border*2)];
   int srcBufferDataPitch  = (processingBlockSize+border*2) * sizeof(uint16);
   int destBufferDataPitch = (processingBlockSize+border*2) * sizeof(uint32);

   for( int by = 0; by < blockCountY; by++ )
   {
      for( int bx = 0; bx < blockCountX; bx++ )
      {
         {
            int totalProgress = blockCountX * blockCountY;
            int currentProgress = bx + by * blockCountX + 1;
            vaProgressWindowUpdate( currentProgress / (float)totalProgress );
         }

         int fromX = bx * processingBlockSize;
         int fromY = by * processingBlockSize;
         int borderLeft = fromX - ::max( 0, (fromX - border) );
         int borderTop  = fromY - ::max( 0, (fromY - border) );
         fromX -= borderLeft;
         fromY -= borderTop;

         int toX = (bx+1) * processingBlockSize - 1;
         int toY = (by+1) * processingBlockSize - 1;
         toX = ::min( heightmap->Width()-1, toX );
         toY = ::min( heightmap->Height()-1, toY );
         int borderRight   = ::min( heightmap->Width()-1, (toX + border) ) - toX;
         int borderBottom  = ::min( heightmap->Height()-1, (toY + border) ) - toY;
         toX += borderRight;
         toY += borderBottom;

         int sizeX = toX - fromX + 1;
         int sizeY = toY - fromY + 1;
      
         heightmap->Read( sourceBuffer, 2, srcBufferDataPitch, fromX, fromY, sizeX, sizeY );

         vaCreateNormalMap( sizeX, sizeY, mapSizeX * sizeX / heightmap->Width(), mapSizeY * sizeY / heightmap->Height(), mapSizeZ, 
                           sourceBuffer, srcBufferDataPitch, destBuffer, destBufferDataPitch );
         
         for( int y = fromY + borderTop; y <= toY - borderBottom; y++ )
         {
            uint32 * scanLine = &destBuffer[ (y - fromY) * destBufferDataPitch/sizeof(uint32) ];

            if( y >= destinationNormalmap->Height() )
               continue;

            int toXCorrected = toX - borderRight;
            toXCorrected = ::min( destinationNormalmap->Width()-1, toXCorrected );

            for( int x = fromX + borderLeft; x <= toXCorrected; x++ )
            {
               uint32 val = scanLine[x - fromX];

               D3DXVECTOR3 norm( ((val & 0xFFFF) / 65535.0f - 0.5f) / 0.5f, ((val >> 16) / 65535.0f - 0.5f) / 0.5f, 0 );
               norm.z = sqrtf( 1 - norm.x*norm.x - norm.y*norm.y );

               byte r = (byte)clamp( 255.0f * ( norm.x * 0.5f + 0.5f ), 0.0f, 255.0f );
               byte g = (byte)clamp( 255.0f * ( norm.y * 0.5f + 0.5f ), 0.0f, 255.0f );
               byte b = (byte)clamp( 255.0f * ( norm.z * 0.5f + 0.5f ), 0.0f, 255.0f );

               uint32 outPixel = b << 16 | g << 8 | r;
               destinationNormalmap->SetPixel( x, y, &outPixel );
            }
         }
      }
   }

   if( showProgress )
   {
      for( int i = 0; i < 10; i++ )
      {
         vaProgressWindowUpdate( 1.0f );
         Sleep( 20 );
      }
      vaProgressWindowClose();
   }
}
