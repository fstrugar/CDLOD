//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "Common.h"

#include "DxEventNotifier.h"

#include "DxShader.h"

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

   //device->SetRenderState( D3DRS_ZENABLE, FALSE );
   //device->SetRenderState( D3DRS_LIGHTING, FALSE );

   V_RETURN( device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, vertices, sizeof( TransofmedTexturedVertex ) ) );

   //device->SetRenderState( D3DRS_LIGHTING, FALSE );
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

   //device->SetRenderState( D3DRS_ZENABLE, FALSE );
   //device->SetRenderState( D3DRS_LIGHTING, FALSE );

   V_RETURN( device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, vertices, sizeof( TransofmedTexturedVertex ) ) );

   //device->SetRenderState( D3DRS_LIGHTING, FALSE );
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
