///////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common.h"

class CDLODStreamingStorage;
class CDLODQuadTree;
class CDLODMinMaxMap;
class IniParser;
namespace VertexAsylum{ class vaStream; }

void vaGenerateMinMaxMap( VertexAsylum::TiledBitmap * heightmap, const int quadSize, unsigned short * outBuffer, int bufferSize );
void vaGenerateMinMaxMapLevels( VertexAsylum::TiledBitmap * heightmap, const int quadSize, int LODLevelCount, 
                                CDLODMinMaxMap minMaxMaps[], bool compressLast, 
                                vaProgressReportProcPtr progressProc, float progressMul, float progressAdd );

void vaGenerateNormalMap( VertexAsylum::TiledBitmap * heightmap, VertexAsylum::TiledBitmap * destinationNormalmap,
                         float mapSizeX, float mapSizeY, float mapSizeZ, bool showProgress = false );

bool vaDLODStreamingStorageCreateFromIni( CDLODStreamingStorage * storage, const IniParser & iniParser, const char * scdlodPath, 
                                         void * metaDataBuffer, int metaDataBufferSize );
