//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "Common.h"

#include "DxEventNotifier.h"

#include "DxShader.h"

#include "Core/IO/vaStream.h"

#include "Core/Common/vaTempMemoryBuffer.h"

#pragma warning ( disable : 4996 )

using namespace std;

//
void vaGetFrustumPlanes( D3DXPLANE * pPlanes, const D3DXMATRIX & mCameraViewProj )
{
   // Left clipping plane
   pPlanes[0].a = mCameraViewProj(0,3) + mCameraViewProj(0,0);
   pPlanes[0].b = mCameraViewProj(1,3) + mCameraViewProj(1,0);
   pPlanes[0].c = mCameraViewProj(2,3) + mCameraViewProj(2,0);
   pPlanes[0].d = mCameraViewProj(3,3) + mCameraViewProj(3,0);

   // Right clipping plane
   pPlanes[1].a = mCameraViewProj(0,3) - mCameraViewProj(0,0);
   pPlanes[1].b = mCameraViewProj(1,3) - mCameraViewProj(1,0);
   pPlanes[1].c = mCameraViewProj(2,3) - mCameraViewProj(2,0);
   pPlanes[1].d = mCameraViewProj(3,3) - mCameraViewProj(3,0);

   // Top clipping plane
   pPlanes[2].a = mCameraViewProj(0,3) - mCameraViewProj(0,1);
   pPlanes[2].b = mCameraViewProj(1,3) - mCameraViewProj(1,1);
   pPlanes[2].c = mCameraViewProj(2,3) - mCameraViewProj(2,1);
   pPlanes[2].d = mCameraViewProj(3,3) - mCameraViewProj(3,1);

   // Bottom clipping plane
   pPlanes[3].a = mCameraViewProj(0,3) + mCameraViewProj(0,1);
   pPlanes[3].b = mCameraViewProj(1,3) + mCameraViewProj(1,1);
   pPlanes[3].c = mCameraViewProj(2,3) + mCameraViewProj(2,1);
   pPlanes[3].d = mCameraViewProj(3,3) + mCameraViewProj(3,1);

   // Near clipping plane
   pPlanes[4].a = mCameraViewProj(0,2);
   pPlanes[4].b = mCameraViewProj(1,2);
   pPlanes[4].c = mCameraViewProj(2,2);
   pPlanes[4].d = mCameraViewProj(3,2);

   // Far clipping plane
   pPlanes[5].a = mCameraViewProj(0,3) - mCameraViewProj(0,2);
   pPlanes[5].b = mCameraViewProj(1,3) - mCameraViewProj(1,2);
   pPlanes[5].c = mCameraViewProj(2,3) - mCameraViewProj(2,2);
   pPlanes[5].d = mCameraViewProj(3,3) - mCameraViewProj(3,2);
   // Normalize the plane equations, if requested

   for (int i = 0; i < 6; i++) 
      D3DXPlaneNormalize( &pPlanes[i], &pPlanes[i] );
}
//
extern string              g_initialWorkingDir;
string GetWorkingDirectory()
{
   return g_initialWorkingDir;
}
//
extern string              g_projectDir;
string vaGetProjectDirectory()
{
   return g_projectDir;
}
//
string vaGetExeDirectory()
{
   // Get the exe name, and exe path
   WCHAR strExePath[MAX_PATH] = {0};
   WCHAR strExeName[MAX_PATH] = {0};
   WCHAR* strLastSlash = NULL;
   GetModuleFileName( NULL, strExePath, MAX_PATH );
   strExePath[MAX_PATH-1]=0;
   strLastSlash = wcsrchr( strExePath, TEXT('\\') );
   if( strLastSlash )
   {
      StringCchCopy( strExeName, MAX_PATH, &strLastSlash[1] );

      // Chop the exe name from the exe path
      *strLastSlash = 0;

      // Chop the .exe from the exe name
      strLastSlash = wcsrchr( strExeName, TEXT('.') );
      if( strLastSlash )
         *strLastSlash = 0;
   }

   wstring ret = wstring(strExePath) + L"\\";
   return vaStringSimpleNarrow( ret );
   //#endif
}
//
bool vaFileExists( const char * file )
{
   FILE * fp = fopen( file, "rb" );
   if( fp != NULL )
   {
      fclose( fp );
      return true;
   }
   return false;
}
//
bool vaFileExists( const wchar_t * file )
{
   FILE * fp = _wfopen( file, L"rb" );
   if( fp != NULL )
   {
      fclose( fp );
      return true;
   }
   return false;
}
//
std::string vaStringSimpleNarrow( const std::wstring & s ) {
   std::string ws;
   ws.resize(s.size());
   for( size_t i = 0; i < s.size(); i++ ) ws[i] = (char)s[i];
   return ws;
}
//
std::wstring vaStringSimpleWiden( const std::string & s ) {
   std::wstring ws;
   ws.resize(s.size());
   for( size_t i = 0; i < s.size(); i++ ) ws[i] = s[i];
   return ws;
}
//
string vaFindResource( const std::string & file, bool showErrorMessage )
{
   string test = GetWorkingDirectory() + file;
   if( vaFileExists( test.c_str() )  )
      return test;

   test = vaGetExeDirectory() + file;
   if( vaFileExists( test.c_str() )  )
      return test;

   if( showErrorMessage )
   {
      MessageBoxA( DXUTGetHWND(), vaStringFormat("Error trying to find '%s'!", file.c_str()).c_str(), "Error", MB_OK );
   }

   return "";
}
//
#pragma warning (disable : 4996)
#pragma warning (disable : 4995)
//
std::wstring vaStringFormat(const wchar_t * fmtString, ...)
{
	va_list args;
	va_start(args, fmtString);

	int nBuf;
	wchar_t szBuffer[512];

	nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer) / sizeof(WCHAR), fmtString, args);
	assert(nBuf < sizeof(szBuffer));//Output truncated as it was > sizeof(szBuffer)

	va_end(args);

	return std::wstring(szBuffer);
}

std::string vaStringFormat(const char * fmtString, ...)
{
	va_list args;
	va_start(args, fmtString);

	int nBuf;
	char szBuffer[512];

	nBuf = _vsnprintf(szBuffer, sizeof(szBuffer) / sizeof(char), fmtString, args);
	assert(nBuf < sizeof(szBuffer));//Output truncated as it was > sizeof(szBuffer)

	va_end(args);

	return std::string(szBuffer);
}
//
wstring vaFindResource( const wstring & file, bool showErrorMessage )
{
   return vaStringSimpleWiden( vaFindResource( vaStringSimpleNarrow( file ), showErrorMessage) );
}
//
HRESULT vaRenderQuad( int fromX, int fromY, int toX, int toY, int texWidth, int texHeight )
{
   HRESULT hr;

   IDirect3DDevice9* device = DxEventNotifier::GetD3DDevice();

   TransofmedTexturedVertex vertices[4];

   float uminx = fromX / (float)(texWidth-1);
   float uminy = fromY / (float)(texHeight-1); 
   float umaxx = (toX) / (float)(texWidth-1);
   float umaxy = (toY) / (float)(texHeight-1);

   vertices[0] = TransofmedTexturedVertex( -0.5f + fromX,      -0.5f + fromY,       0.5f, 1.0f, uminx, uminy );
   vertices[1] = TransofmedTexturedVertex( (toX) - 0.5f,       -0.5f + fromY,       0.5f, 1.0f, umaxx, uminy );
   vertices[2] = TransofmedTexturedVertex( -0.5f + fromX,      (toY) - 0.5f,        0.5f, 1.0f, uminx, umaxy );
   vertices[3] = TransofmedTexturedVertex( (toX) - 0.5f ,      (toY) - 0.5f,        0.5f, 1.0f, umaxx, umaxy );

   V_RETURN( device->SetRenderState( D3DRS_ZENABLE, FALSE ) );
   V_RETURN( device->SetFVF( TransofmedTexturedVertex::FVF ) );

   V_RETURN( device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, vertices, sizeof( TransofmedTexturedVertex ) ) );

   V_RETURN( device->SetRenderState( D3DRS_ZENABLE, TRUE ) );

   return S_OK;
}
//
HRESULT vaRenderQuad( int width, int height, int cutEdge )
{
   HRESULT hr;

   IDirect3DDevice9* device = DxEventNotifier::GetD3DDevice();

   TransofmedTexturedVertex vertices[4];

   float fCutEdge = (float)cutEdge;

   float uminx = 0 + fCutEdge / (float)(width-1);
   float uminy = 0 + fCutEdge / (float)(height-1); 
   float umaxx = 1 - fCutEdge / (float)(width-1);
   float umaxy = 1 - fCutEdge / (float)(height-1); 

   vertices[0] = TransofmedTexturedVertex( -0.5f + fCutEdge,         -0.5f + fCutEdge,         0.5f, 1.0f, uminx, uminy );
   vertices[1] = TransofmedTexturedVertex( width - 0.5f - fCutEdge,  -0.5f + fCutEdge,         0.5f, 1.0f, umaxx, uminy );
   vertices[2] = TransofmedTexturedVertex( -0.5f + fCutEdge,         height - 0.5f - fCutEdge, 0.5f, 1.0f, uminx, umaxy );
   vertices[3] = TransofmedTexturedVertex( width - 0.5f - fCutEdge,  height - 0.5f - fCutEdge, 0.5f, 1.0f, umaxx, umaxy );

   for( int i = 0; i < 4; i++ )
   {
      vertices[i].x += 0;
      vertices[i].y += 0;
   }

   V_RETURN( device->SetRenderState( D3DRS_ZENABLE, FALSE ) );
   V_RETURN( device->SetFVF( TransofmedTexturedVertex::FVF ) );

   V_RETURN( device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, vertices, sizeof( TransofmedTexturedVertex ) ) );

   V_RETURN( device->SetRenderState( D3DRS_ZENABLE, TRUE ) );

   return S_OK;
}
//
HRESULT vaSetRenderTargetTexture( int index, IDirect3DTexture9 * texture )
{
   IDirect3DSurface9 * surface = NULL;

   HRESULT hr;
   if( texture != NULL )
   {
      V_RETURN( texture->GetSurfaceLevel( 0, &surface ) );
   }

   V_RETURN( DxEventNotifier::GetD3DDevice()->SetRenderTarget( index, surface ) );

   SAFE_RELEASE( surface );

   return S_OK;
}
//
HRESULT vaSetDepthStencilTexture( IDirect3DTexture9 * texture )
{
   IDirect3DSurface9 * surface = NULL;

   HRESULT hr;
   if( texture != NULL )
   {
      V_RETURN( texture->GetSurfaceLevel( 0, &surface ) );
   }

   V_RETURN( DxEventNotifier::GetD3DDevice()->SetDepthStencilSurface( surface ) );

   SAFE_RELEASE( surface );

   return S_OK;
}
//
void vaFatalError( const char * messageString )
{
   MessageBoxA( DXUTGetHWND(), messageString, "Fatal error, program will close", MB_ICONERROR | MB_SETFOREGROUND| MB_TOPMOST );
   abort();
}
//
HRESULT vaStretchRect( IDirect3DTexture9* pSourceTexture, CONST RECT* pSourceRect, IDirect3DTexture9* pDestTexture, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter)
{
   HRESULT hr;
   IDirect3DSurface9 * sourceSurface, * destSurface;
   V( pSourceTexture->GetSurfaceLevel( 0, &sourceSurface ) );
   V( pDestTexture->GetSurfaceLevel( 0, &destSurface ) );

   hr = DxEventNotifier::GetD3DDevice()->StretchRect( sourceSurface, pSourceRect, destSurface, pDestRect, Filter );

   SAFE_RELEASE( sourceSurface );
   SAFE_RELEASE( destSurface );

   V( hr );

   return hr;
}
//
HRESULT vaDebugDisplayTexture( IDirect3DTexture9* sourceTexture, int x, int y, int width, int height )
{
   HRESULT hr;
   IDirect3DDevice9 * device = DxEventNotifier::GetD3DDevice();

   IDirect3DSurface9 * renderTargetSurf;
   V( device->GetRenderTarget(0, &renderTargetSurf) );
   SAFE_RELEASE(renderTargetSurf);

   IDirect3DSurface9 * sourceSurface;
   V( sourceTexture->GetSurfaceLevel(0, &sourceSurface) );
   D3DSURFACE_DESC srcDesc;
   sourceSurface->GetDesc(&srcDesc);
   SAFE_RELEASE(sourceSurface);

   if( width == -1 )    width    = srcDesc.Width;
   if( height == -1 )   height   = srcDesc.Height;

   float rleft   = (float)x; 
   float rtop    = (float)y; 
   float rright  = (float)x + width;
   float rbottom = (float)y + height;

   static DxPixelShader psJustCopy( "Shaders/misc.psh", "justCopy" );
   psJustCopy.SetTexture( "g_justCopySourceTexture", sourceTexture );

   device->SetPixelShader( psJustCopy );

   TransofmedTexturedVertex vertices[4];

   float uminx = 0.0f - 0.5f / (float)width;
   float uminy = 0.0f - 0.5f / (float)height;
   float umaxx = 1.0f - 0.5f / (float)width;
   float umaxy = 1.0f - 0.5f / (float)height;

   vertices[0] = TransofmedTexturedVertex( rleft,    rtop,      0.5f, 1.0f, uminx, uminy );
   vertices[1] = TransofmedTexturedVertex( rright,   rtop,      0.5f, 1.0f, umaxx, uminy );
   vertices[2] = TransofmedTexturedVertex( rleft,    rbottom,   0.5f, 1.0f, uminx, umaxy );
   vertices[3] = TransofmedTexturedVertex( rright,   rbottom,   0.5f, 1.0f, umaxx, umaxy );

   V_RETURN( device->SetRenderState( D3DRS_ZENABLE, FALSE ) );
   V_RETURN( device->SetFVF( TransofmedTexturedVertex::FVF ) );

   V_RETURN( device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, vertices, sizeof( TransofmedTexturedVertex ) ) );

   V_RETURN( device->SetRenderState( D3DRS_ZENABLE, TRUE ) );

   return S_OK;
}

void vaSplitPath( const char * inFullPath, string & outDirectory, string & outFileName, string & outFileExt )
{
   char buffDrive[32];
   char buffDir[1024];
   char buffName[1024];
   char buffExt[1024];

   _splitpath( inFullPath, buffDrive, buffDir, buffName, buffExt );

   outDirectory = string(buffDrive) + string(buffDir);
   outFileName = buffName;
   outFileExt = buffExt;
}

HRESULT vaConvertTexture( IDirect3DTexture9 *& outDestTexture, IDirect3DTexture9 * srcTexture, 
                                      DWORD destUsage,D3DFORMAT destFormat,D3DPOOL destPool, bool convertMipLevels )
{
   HRESULT hr;
   IDirect3DDevice9 * device = DxEventNotifier::GetD3DDevice();

   const int mipLevels = (convertMipLevels)?(srcTexture->GetLevelCount()):(1);

   D3DSURFACE_DESC surfDesc;
   V( srcTexture->GetLevelDesc( 0, &surfDesc ) );

   V( device->CreateTexture( surfDesc.Width, surfDesc.Height, mipLevels, destUsage, destFormat, destPool, &outDestTexture, NULL ) );

   for( int level = 0; level < mipLevels; level++ )
   {
      D3DLOCKED_RECT srcLockedRect; 
      V( srcTexture->LockRect( level, &srcLockedRect, NULL, 0 ) );
      V( srcTexture->GetLevelDesc( level, &surfDesc ) );

      IDirect3DSurface9 * destSurface = NULL;
      V( outDestTexture->GetSurfaceLevel( level, &destSurface ) );

      RECT srcRect;
      ::SetRect( &srcRect, 0, 0, surfDesc.Width, surfDesc.Height );
      V( D3DXLoadSurfaceFromMemory( destSurface, NULL, NULL, srcLockedRect.pBits, surfDesc.Format, srcLockedRect.Pitch, 
                                    NULL, &srcRect, D3DX_DEFAULT, 0 ) );

      SAFE_RELEASE( destSurface );
      V( srcTexture->UnlockRect( level ) );
   }

   return S_OK;
}
//
static inline D3DXVECTOR3 AvgNormalFromQuad( float ha, float hb, float hc, float hd, float sizex, float sizey, float scalez )
{
   D3DXVECTOR3 n0, n1;

   n0.x = - (hb - ha) * scalez * sizey;
   n0.y = - sizex * (hc - ha) * scalez;
   n0.z = sizex * sizey;

   //D3DXVec3Normalize( &n0, &n0 );

   n1.x = -sizey * (hd-hc) * scalez;
   n1.y = ((hb-hc) * sizex - sizex * (hd-hc)) * scalez;
   n1.z = sizey * sizex;


   //D3DXVec3Normalize( &n1, &n1 );

   n0 += n1;
   //D3DXVec3Normalize( &n0, &n0 );

   return n0;
}
//
static inline int CoordClamp( int val, int limit )
{
   if( val < 0 )        return 0;
   if( val > limit - 1 )  return limit - 1;
   return val;
}
//
static inline int CoordWrap( int val, int limit )
{
   if( val < 0 )        return limit + val;
   if( val > limit - 1 )  return val - limit;
   return val;
}
//
static inline int CoordFix( int val, int limit, bool wrap )
{
   if( wrap )
      return CoordWrap( val, limit );
   else
      return CoordClamp( val, limit );
}
//
static inline float SampleHeight( unsigned short data )
{
   return data / 65535.0f;
}
//
static inline float SampleHeight( float data )
{
   return data;
}
//
template <typename HeightmapType>
static void CreateNormalMap( int sizeX, int sizeY, float mapSizeX, float mapSizeY, float mapSizeZ, 
                            HeightmapType * heightmapData, int heightmapDataPitch, 
                            unsigned int * normalmapData, int normalmapDataPitch, const bool wrapEdges )
{
   float stepx = 1.0f / (sizeX-1) * mapSizeX;
   float stepy = 1.0f / (sizeY-1) * mapSizeY;

   const int smoothSteps = 1; // can be 0, 1, 2, ... more steps == slower algorithm
   for( int dist = 1; dist < 2+smoothSteps; dist++ )
   {
      for( int y = 0; y < sizeY; y++ )
      {
         HeightmapType * hmScanLine1 = &heightmapData[ CoordFix(y+0-dist+1, sizeY, wrapEdges) * (heightmapDataPitch/sizeof(HeightmapType)) ];
         HeightmapType * hmScanLine2 = &heightmapData[ CoordFix(y+dist, sizeY, wrapEdges) * (heightmapDataPitch/sizeof(HeightmapType)) ];

         unsigned int * nmScanLine = &normalmapData[ y * (normalmapDataPitch/sizeof(unsigned int)) ];

         for( int x = 0; x < sizeX; x++ )
         {
            int xcoord    = CoordFix( x-dist+1, sizeX, wrapEdges );
            int xcoordp   = CoordFix( x+dist, sizeX, wrapEdges );

            float he = SampleHeight( hmScanLine1[xcoord] );
            float hf = SampleHeight( hmScanLine2[xcoord] );
            float hh = SampleHeight( hmScanLine1[xcoordp] );
            float hi = SampleHeight( hmScanLine2[xcoordp] );

            D3DXVECTOR3 norm = AvgNormalFromQuad( he, hf, hh, hi, stepx * (dist*2-1), stepy * (dist*2-1), mapSizeZ );

            D3DXVec3Normalize( &norm, &norm );

            if( dist > 1 )
            {
               D3DXVECTOR3 oldNorm( ((nmScanLine[x] >> 16) / 65535.0f - 0.5f) / 0.5f, ((nmScanLine[x] & 0xFFFF ) / 65535.0f - 0.5f) / 0.5f, 0 );
               oldNorm.z = sqrtf( 1 - oldNorm.x*oldNorm.x - oldNorm.y*oldNorm.y );

               norm += oldNorm * 5.0f; // use bigger const to add more weight to normals calculated from smaller quads
               D3DXVec3Normalize( &norm, &norm );
            }

            unsigned short a = (unsigned short)clamp( 65535.0f * ( norm.x * 0.5f + 0.5f ), 0.0f, 65535.0f );
            unsigned short b = (unsigned short)clamp( 65535.0f * ( norm.y * 0.5f + 0.5f ), 0.0f, 65535.0f );

            nmScanLine[x] = (a << 16) | b;
         }
      }
   }
}
//
void vaCreateNormalMap( int sizeX, int sizeY, float mapSizeX, float mapSizeY, float mapSizeZ, unsigned short * heightmapData, 
                        int heightmapDataPitch, unsigned int * normalmapData, int normalmapDataPitch, const bool wrapEdges )
{
   return CreateNormalMap( sizeX, sizeY, mapSizeX, mapSizeY, mapSizeZ, heightmapData, heightmapDataPitch, 
                           normalmapData, normalmapDataPitch, wrapEdges );
}

void vaCreateNormalMap( int sizeX, int sizeY, float mapSizeX, float mapSizeY, float mapSizeZ, float * heightmapData, 
                        int heightmapDataPitch, unsigned int * normalmapData, int normalmapDataPitch, const bool wrapEdges )
{
   return CreateNormalMap( sizeX, sizeY, mapSizeX, mapSizeY, mapSizeZ, heightmapData, heightmapDataPitch, 
      normalmapData, normalmapDataPitch, wrapEdges );
}
//
HRESULT vaSaveTexture( VertexAsylum::vaStream * outStream, IDirect3DTexture9 * inTexture )
{
   static VertexAsylum::vaTempMemoryBuffer tempBuffer;
   HRESULT hr;

   
   if( !outStream->WriteValue<int32>(2) )   { return E_FAIL; }

   {
      //replacement for 
      //V_RETURN( D3DXSaveTextureToFileInMemory( &pBuffer, D3DXIFF_DDS, inTexture, NULL ) );

      D3DSURFACE_DESC surfDesc;
      inTexture->GetLevelDesc(0, &surfDesc);

      // room for data size
      int64 dataSizeStreamPos = outStream->GetPosition();
      if( !outStream->WriteValue<uint32>((uint32)0) )   { return E_FAIL; }

      if( !outStream->WriteValue<uint32>((uint32)surfDesc.Format) )     { return E_FAIL; }
      if( !outStream->WriteValue<uint32>((uint32)surfDesc.Width) )      { return E_FAIL; }
      if( !outStream->WriteValue<uint32>((uint32)surfDesc.Height) )     { return E_FAIL; }
      
      uint32 mipLevelCount = inTexture->GetLevelCount();
      
      if( !outStream->WriteValue<uint32>((uint32)mipLevelCount) )   { return E_FAIL; }

      int32 correctWidth = surfDesc.Width;
      int32 correctHeight = surfDesc.Height;

      for( int i = 0; i < (int)mipLevelCount; i++ )
      {
         inTexture->GetLevelDesc(i, &surfDesc);

         if( (correctWidth != (int32)surfDesc.Width) || (correctHeight != (int32)surfDesc.Height) )
            return E_FAIL;

         D3DLOCKED_RECT srcLockedRect; 
         V( inTexture->LockRect( i, &srcLockedRect, NULL, D3DLOCK_READONLY ) );

         int mipLevelBufferSize = srcLockedRect.Pitch * correctHeight;
         if( surfDesc.Format == D3DFMT_DXT1 )   mipLevelBufferSize /= 4;
         if( surfDesc.Format == D3DFMT_DXT5 )   mipLevelBufferSize /= 4;

         if( !outStream->WriteValue<uint32>((uint32)mipLevelBufferSize) )   { return E_FAIL; }

         if( outStream->Write(srcLockedRect.pBits, mipLevelBufferSize) != mipLevelBufferSize )   { return E_FAIL; }

         V( inTexture->UnlockRect( i ) );

         correctWidth   = ::max( 1, correctWidth / 2 );
         correctHeight  = ::max( 1, correctHeight / 2 );
      }

      // Now go back and store the whole buffer size info
      int64 currentStreamPos = outStream->GetPosition();
      outStream->Seek( dataSizeStreamPos );
      if( !outStream->WriteValue<uint32>( (uint32)(currentStreamPos - dataSizeStreamPos - sizeof(uint32) ) ) )   { return E_FAIL; }
      outStream->Seek( currentStreamPos );
   }

   return S_OK;
}
//
HRESULT vaLoadTexture( VertexAsylum::vaStream * inStream, IDirect3DTexture9 *& outTexture )
{
   static VertexAsylum::vaTempMemoryBuffer tempBuffer;

   int version;
   if( !inStream->ReadValue<int32>( version ) )
      return E_FAIL;

   // D3DXCreateTextureFromFileInMemoryEx version not supported anymore
   if( version == 1 )
      return E_FAIL;

   if( version != 2 )
      return E_FAIL;

   {
      int bufferSize;
      if( !inStream->ReadValue<int32>( bufferSize ) )
         return E_FAIL;
      if( bufferSize <= 0 )
         return E_FAIL;

      byte * bufferPtr = tempBuffer.GetBuffer(bufferSize);
      if( inStream->Read(bufferPtr, bufferSize) != bufferSize )
         return E_FAIL;

      return vaLoadTexture( (byte*)bufferPtr, (int32)bufferSize, outTexture, false );
   }
}
//
HRESULT vaLoadTexture( byte * rawBuffer, int32 rawBufferSize, IDirect3DTexture9 *& outTexture, bool processFullHeader )
{
   HRESULT hr;

   if( rawBufferSize < 48 )
      return E_FAIL;

   if( processFullHeader )
   {
      int version = *((int32*)(&rawBuffer[0]));

      // D3DXCreateTextureFromFileInMemoryEx version not supported anymore
      if( version == 1 )
         return E_FAIL;

      if( version != 2 )
         return E_FAIL;


      int bufferSize = *((int32*)(&rawBuffer[4]));
      if( bufferSize > (rawBufferSize-8) )
      {
         assert( false );
         return E_FAIL;
      }

      rawBuffer += 8;
      rawBufferSize = bufferSize;
   }

   {
      unsigned char * bufferPtr = rawBuffer;
      int bufferSize = rawBufferSize;

      if( bufferSize <= 32 )
         return E_FAIL;


      unsigned char * currentBufferPtr = bufferPtr;
      D3DFORMAT format = (D3DFORMAT)*( (uint32*)currentBufferPtr );
      currentBufferPtr += sizeof(uint32);

      uint32 width = *( (uint32*)currentBufferPtr );
      currentBufferPtr += sizeof(uint32);

      uint32 height = *( (uint32*)currentBufferPtr );
      currentBufferPtr += sizeof(uint32);

      uint32 mipLevelCount = *( (uint32*)currentBufferPtr );
      currentBufferPtr += sizeof(uint32);

      V_RETURN( DxEventNotifier::GetD3DDevice()->CreateTexture( width, height, mipLevelCount, D3DUSAGE_DYNAMIC, 
                                                                  format, D3DPOOL_DEFAULT, &outTexture, NULL ) );

      D3DSURFACE_DESC surfDesc;
      outTexture->GetLevelDesc(0, &surfDesc);

      int32 correctWidth = surfDesc.Width;
      int32 correctHeight = surfDesc.Height;

      for( int i = 0; i < (int)mipLevelCount; i++ )
      {
         outTexture->GetLevelDesc(i, &surfDesc);

         if( (correctWidth != (int32)surfDesc.Width) || (correctHeight != (int32)surfDesc.Height) )
         {
            SAFE_RELEASE( outTexture );
            return E_FAIL;
         }

         D3DLOCKED_RECT dstLockedRect; 
         V( outTexture->LockRect( i, &dstLockedRect, NULL, 0 ) );

         uint32 dstPitch = (uint32)dstLockedRect.Pitch;
         if( surfDesc.Format == D3DFMT_DXT1 )   dstPitch /= 4;
         if( surfDesc.Format == D3DFMT_DXT5 )   dstPitch /= 4;

         uint32 mipLevelBufferSize = *( (uint32*)currentBufferPtr );
         currentBufferPtr += sizeof(uint32);

         assert( (mipLevelBufferSize % correctHeight) == 0 );
         uint32 srcPitch = mipLevelBufferSize / correctHeight;
         assert( srcPitch <= dstPitch );

         if( ((mipLevelBufferSize % correctHeight) != 0) && (srcPitch > dstPitch) )
         {
            outTexture->UnlockRect( i );
            SAFE_RELEASE( outTexture );
            return E_FAIL;
         }

         byte * destBuffer = (byte *)dstLockedRect.pBits;
         for( int32 sl = 0; sl < correctHeight; sl++ )
         {
            memcpy( destBuffer, currentBufferPtr, srcPitch );
            destBuffer += dstPitch;
            currentBufferPtr += srcPitch;
         }

         V( outTexture->UnlockRect( i ) );

         correctWidth   = ::max( 1, correctWidth / 2 );
         correctHeight  = ::max( 1, correctHeight / 2 );
      }

      assert( currentBufferPtr == (bufferPtr + bufferSize) );
   }

   return S_OK;
}

bool vaLoadTexture( byte * rawBuffer, int32 rawBufferSize, D3DFORMAT format, int mipCount, 
                     D3DSURFACE_DESC surfaceDescs[], D3DLOCKED_RECT lockedRects[], bool processFullHeader )
{
   if( rawBufferSize < 48 )
      return false;

   if( processFullHeader )
   {
      int version = *((int32*)(&rawBuffer[0]));

      if( version == 1 )
      {
         assert( false );
         return NULL;
      }

      if( version != 2 )
      {
         assert( false );
         return NULL;
      }


      int bufferSize = *((int32*)(&rawBuffer[4]));
      if( bufferSize > (rawBufferSize-8) )
      {
         assert( false );
         return NULL;
      }

      rawBuffer += 8;
      rawBufferSize = bufferSize;
   }

   {
      unsigned char * bufferPtr = rawBuffer;
      int bufferSize = rawBufferSize;

      if( bufferSize <= 32 )
      {
         assert( false );
         return false;
      }


      unsigned char * currentBufferPtr = bufferPtr;
      D3DFORMAT dataFormat = (D3DFORMAT)*( (uint32*)currentBufferPtr );
      currentBufferPtr += sizeof(uint32);

      uint32 width = *( (uint32*)currentBufferPtr );
      currentBufferPtr += sizeof(uint32);

      uint32 height = *( (uint32*)currentBufferPtr );
      currentBufferPtr += sizeof(uint32);

      uint32 mipLevelCount = *( (uint32*)currentBufferPtr );
      currentBufferPtr += sizeof(uint32);

      if( (int)mipLevelCount != mipCount )
      {
         assert( false );
         return false;
      }

      if( dataFormat != format )
      {
         assert( false );
         return false;
      }

      D3DSURFACE_DESC surfDesc;
      surfDesc = surfaceDescs[0];

      int32 correctWidth = width;
      int32 correctHeight = height;

      for( int i = 0; i < (int)mipLevelCount; i++ )
      {
         surfDesc = surfaceDescs[i];

         if( (correctWidth > (int32)surfDesc.Width) || (correctHeight > (int32)surfDesc.Height) )
         {
            assert( false );
            vaFatalError( "Fatal error in vaLoadTexture()" );
            return false;
         }

         D3DLOCKED_RECT dstLockedRect = lockedRects[i];

         uint32 dstPitch = (uint32)dstLockedRect.Pitch;
         if( surfDesc.Format == D3DFMT_DXT1 )   dstPitch /= 4;
         if( surfDesc.Format == D3DFMT_DXT5 )   dstPitch /= 4;

         uint32 mipLevelBufferSize = *( (uint32*)currentBufferPtr );
         currentBufferPtr += sizeof(uint32);

         assert( (mipLevelBufferSize % correctHeight) == 0 );
         uint32 srcPitch = mipLevelBufferSize / correctHeight;
         assert( srcPitch <= dstPitch );

         if( ((mipLevelBufferSize % correctHeight) != 0) && (srcPitch > dstPitch) )
         {
            assert( false );
            vaFatalError( "Fatal error in vaLoadTexture()" );
            return false;
         }

         int copyPitch = ::min( srcPitch, dstPitch );
         int reminderPitch = dstPitch - copyPitch;

         byte * destBuffer = (byte *)dstLockedRect.pBits;
         for( int32 sl = 0; sl < correctHeight; sl++ )
         {
            memcpy( destBuffer, currentBufferPtr, copyPitch );
            
            if( reminderPitch > 0 )
               memset( destBuffer + copyPitch, 0, reminderPitch );

            destBuffer += dstPitch;
            currentBufferPtr += srcPitch;
         }

         correctWidth   = ::max( 1, correctWidth / 2 );
         correctHeight  = ::max( 1, correctHeight / 2 );
      }

      assert( currentBufferPtr == (bufferPtr + bufferSize) );
   }

   return true;
}

int vaCalcApproxTextureMemSize( D3DFORMAT format, int width, int height, int mipCount )
{
   int pixels = 0;
   int tw = width, th = height;
   for( int i = 0; i < mipCount; i++ )
   {
      pixels += tw * th;
      tw /= 2; th /= 2;
   }

   switch(format) 
   {
   case( D3DFMT_DXT1 ):       return pixels / 2;

   case( D3DFMT_DXT2 ):
   case( D3DFMT_DXT3 ):
   case( D3DFMT_DXT4 ):
   case( D3DFMT_DXT5 ):       return pixels;

   case( D3DFMT_L16 ):
   case( D3DFMT_R16F ):
   case( D3DFMT_A4R4G4B4 ):   return pixels * 2;
   case( D3DFMT_L8 ):         return pixels;
   case( D3DFMT_R32F ):       return pixels * 4;
   case( D3DFMT_A8R8G8B8 ):   
   case( D3DFMT_A8B8G8R8 ):   
   case( D3DFMT_X8R8G8B8 ):   
   case( D3DFMT_X8B8G8R8 ):   return pixels * 4;
   default:
      assert(false);
   }
   return 0;
}