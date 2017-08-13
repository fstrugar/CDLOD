//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#pragma once

#include "DXUT.h"

//#define MY_EXTENDED_STUFF
//#define USE_REC_O_MATIC
//#define USE_SIMPLE_PROFILER


#pragma warning ( disable: 4996 )
#pragma warning ( disable: 4995 )
#pragma warning ( disable: 4127 )   //warning C4127: conditional expression is constant


#include <string>
#include <vector>

#include "TiledBitmap/TiledBitmap.h"


typedef unsigned char      byte;
typedef char               sbyte;
typedef int                int32;
typedef unsigned int       uint32;
typedef short              int16;
typedef unsigned short     uint16;
typedef __int64            int64;
typedef unsigned __int64   uint64;

//////////////////////////////////////////////////////////////////////////
// Project specific settings


enum KeyCommands
{
   kcForward	= 0,
   kcBackward,
   kcLeft,
   kcRight,
   kcUp,
   kcDown,
   kcShiftKey,
   kcControlKey,
   kcViewRangeUp,
   kcViewRangeDown,
   kcToggleHelpText,
   
   kcToggleWireframe,
   kcToggleDetailmap,
   kcToggleShadowmap,
   kcToggleDebugView,
   kcToggleTexturingMode,

   kcToggleMovieRec,
   kcToggleCameraScriptRec,
   kcToggleCameraScriptPlay,
   kcSaveViewProjOverride,
   kcOrthoViewProjOverride,
   kcToggleViewProjOverride,

   kcReloadAll,
   kcReloadShadersOnly,

   kcSaveState,
   kcLoadState,

   kcPause,
   kcOneFrameStep,

   kcLASTVALUE
};

enum ShadowMapSupport
{
   smsNotSupported      = 0,
   smsNVidiaShadows,
   smsATIShadows,
};

struct MapDimensions
{
	float             MinX;
	float             MinY;
	float             MinZ;
	float             SizeX;
	float             SizeY;
	float             SizeZ;

	float             MaxX() const   { return MinX + SizeX; }
	float             MaxY() const   { return MinY + SizeY; }
	float             MaxZ() const   { return MinZ + SizeZ; }
};

class IniParser;
namespace VertexAsylum{ class vaStream; }
struct DxReusableTexture;
class DxReusableTexturePool;

std::string          vaGetExeDirectory();
std::string          vaGetProjectDirectory();
const IniParser &    vaGetIniParser();

const float          c_fogColor[4]           = { 0.0f, 0.5f, 0.5f, 1.0f };
const float          c_lightColorDiffuse[4]  = { 0.65f, 0.65f, 0.65f, 1.0f };
const float          c_lightColorAmbient[4]  = { 0.40f, 0.40f, 0.40f, 1.0f };

typedef void (*vaProgressReportProcPtr)(float progress);

//////////////////////////////////////////////////////////////////////////
// Demo environment specific settings

bool                 vaIsKey( enum KeyCommands );
bool                 vaIsKeyClicked( enum KeyCommands );
bool                 vaIsKeyReleased( enum KeyCommands );

std::wstring   		vaFindResource( const std::wstring & file, bool showErrorMessage = true );
std::string    		vaFindResource( const std::string & file, bool showErrorMessage = true );

void                 vaGetCursorDelta( float & x, float & y );

void                 vaFatalError( const char * messageString );


//////////////////////////////////////////////////////////////////////////
// Various utility functions and stuff

std::wstring         vaStringFormat(const wchar_t * fmtString, ...);
std::string          vaStringFormat(const char * fmtString, ...);

std::wstring         vaStringSimpleWiden( const std::string & s );
std::string          vaStringSimpleNarrow( const std::wstring & s );

std::string          vaAddProjectDir( const std::string & fileName );

void                 vaGetFrustumPlanes( D3DXPLANE * pPlanes, const D3DXMATRIX & mCameraViewProj );
float	               vaWrapAngle( float angle );

HRESULT              vaRenderQuad( int fromX, int fromY, int toX, int toY, int texWidth, int texHeight );
HRESULT              vaRenderQuad( int width, int height, int cutEdge = 0 );
HRESULT              vaSetRenderTargetTexture( int index, IDirect3DTexture9 * texture );
HRESULT              vaSetDepthStencilTexture( IDirect3DTexture9 * texture );
HRESULT              vaGetRenderTargetData( IDirect3DTexture9 * srcTexture, IDirect3DTexture9 * destTexture );
HRESULT              vaConvertTexture( IDirect3DTexture9 *& outDestTexture, IDirect3DTexture9 * srcTexture, 
                                      DWORD destUsage,D3DFORMAT destFormat,D3DPOOL destPool, bool convertMipLevels );
HRESULT              vaSaveTexture( VertexAsylum::vaStream * outStream, IDirect3DTexture9 * inTexture );
HRESULT              vaLoadTexture( VertexAsylum::vaStream * inStream, IDirect3DTexture9 *& outTexture );
HRESULT              vaLoadTexture( byte * rawBuffer, int32 rawBufferSize, IDirect3DTexture9 *& outTexture, bool processFullHeader );
bool                 vaLoadTexture( byte * rawBuffer, int32 rawBufferSize, D3DFORMAT format, int mipCount, 
                                    D3DSURFACE_DESC surfaceDescs[], D3DLOCKED_RECT lockedRects[], bool processFullHeader );

int                  vaCalcApproxTextureMemSize( D3DFORMAT format, int width, int height, int mipCount );

HRESULT              vaFillColor( D3DXVECTOR4 color, IDirect3DTexture9 * texture );

HRESULT              vaStretchRect( IDirect3DTexture9* pSourceTexture, CONST RECT* pSourceRect, IDirect3DTexture9* pDestTexture, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter);

HRESULT              vaDebugDisplayTexture( IDirect3DTexture9* sourceTexture, int x, int y, int width = -1, int height = -1 );

void                 vaCreateNormalMap( int sizeX, int sizeY, float mapSizeX, float mapSizeY, float mapSizeZ, 
                              unsigned short * heightmapData, int heightmapDataPitch, 
                              unsigned int * normalmapData, int normalmapDataPitch, const bool wrapEdges = false );
void                 vaCreateNormalMap( int sizeX, int sizeY, float mapSizeX, float mapSizeY, float mapSizeZ, 
                                       float * heightmapData, int heightmapDataPitch, 
                                       unsigned int * normalmapData, int normalmapDataPitch, const bool wrapEdges = false );

bool                 vaFileExists( const char * file );
bool                 vaFileExists( const wchar_t * file );

void                 vaSplitPath( const char * inFullPath, std::string & outDirectory, std::string & outFileName, std::string & outFileExt );

struct TransofmedTexturedVertex
{
	float    x, y, z, rhw;
	float    tu, tv;

	TransofmedTexturedVertex() {}
	TransofmedTexturedVertex( float x, float y, float z, float rhw, float tu, float tv )   : x(x), y(y), z(z), rhw(rhw), tu(tu), tv(tv)    { }

	static const DWORD     FVF = D3DFVF_XYZRHW | D3DFVF_TEX1;
};

struct TransformedVertex
{
	float    x, y, z, rhw;

	TransformedVertex() {}
	TransformedVertex( float x, float y, float z, float rhw )   : x(x), y(y), z(z), rhw(rhw)   { }

	static const DWORD     FVF = D3DFVF_XYZRHW;
};

struct TransformedColoredVertex
{
	float    x, y, z, rhw;

	DWORD    diffuse;

	TransformedColoredVertex() {}
	TransformedColoredVertex( float x, float y, float z, float rhw, DWORD diffuse )   : x(x), y(y), z(z), rhw(rhw), diffuse( diffuse )   { }

	static const DWORD     FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE;
};

struct PositionVertex
{
	float    x, y, z;

	PositionVertex() {}
	PositionVertex( float x, float y, float z )  : x(x), y(y), z(z)         { }
	PositionVertex( const D3DXVECTOR3 & v )      : x(v.x), y(v.y), z(v.z)   { }

	static const DWORD     FVF = D3DFVF_XYZ;
};

template<class T> static void erase( T & v )
{
	if( v.empty() ) return;
	v.erase(v.begin(), v.end());
}

inline unsigned short vaHalfFloatPack(float val)
{
	unsigned int num5 = *((unsigned int*) &val);
	unsigned int num3 = (unsigned int) ((num5 & 0x80000000) >> 0x10); // -2147483648
	unsigned int num = num5 & 0x7fffffff;
	if (num > 0x47ffefff)
	{
		return (unsigned short) (num3 | 0x7fff);
	}
	if (num < 0x38800000)
	{
		unsigned int num6 = (num & 0x7fffff) | 0x800000;
		int num4 = 0x71 - ((int) (num >> 0x17));
		num = (num4 > 0x1f) ? 0 : (num6 >> num4);
		return (unsigned short) (num3 | (((num + 0xfff) + ((num >> 13) & 1)) >> 13));
	}
	return (unsigned short) (num3 | ((((num + -939524096) + 0xfff) + ((num >> 13) & 1)) >> 13));
}

inline float vaHalfFloatUnpack(unsigned short val)
{
	unsigned int num3;
	if ((val & -33792) == 0)
	{
		if ((val & 0x3ff) != 0)
		{
			unsigned int num2 = 0xfffffff2;
			unsigned int num = (unsigned int) (val & 0x3ff);
			while ((num & 0x400) == 0)
			{
				num2--;
				num = num << 1;
			}
			num &= 0xfffffbff;
			num3 = ((unsigned int) (((val & 0x8000) << 0x10) | ((num2 + 0x7f) << 0x17))) | (num << 13);
		}
		else
		{
			num3 = (unsigned int) ((val & 0x8000) << 0x10);
		}
	}
	else
	{
		num3 = (unsigned int) ((((val & 0x8000) << 0x10) | (((((val >> 10) & 0x1f) - 15) + 0x7f) << 0x17)) | ((val & 0x3ff) << 13));
	}
	return *(((float*) &num3));
}

inline bool vaIsPowOf2(int val) 
{
   if( val < 1 ) return false;
   return (val & (val-1)) == 0;
}

inline int vaLog2(int val)
{
   unsigned r = 0;

   while( val >>= 1 )
   {
      r++;
   }

   return r;
}

bool              vaGetBilinearVertexTextureFilterSupport();
D3DFORMAT         vaGetVertexTextureFormat();

#ifdef DEBUG
#define DBGBREAK __asm{ int 3 }
#else
#define DBGBREAK
#endif

#ifndef PI
#define PI 3.141592653
#endif

template<class T>
inline T clamp(T const & v, T const & b, T const & c)
{
	if( v < b ) return b;
	if( v > c ) return c;
	return v;
}

// short for clamp( a, 0, 1 )
template<class T>
inline T saturate( T const & a )
{
	return ::clamp( a, (T)0.0, (T)1.0 );
}

template<class T>
inline T lerp(T const & a, T const & b, T const & f)
{
	return a + (b-a)*f;
}

template<class T>
inline void swap(T & a, T & b)
{
	T temp = b;
	b = a;
	a = temp;
}

template<class T>
inline T sq(const T & a)
{
	return a * a;
}

inline float randf( )       { return rand() / (float)RAND_MAX; }

// Get time independent lerp function. Play with different lerpRates - the bigger the rate, the faster the lerp!
inline float timeIndependentLerpF(float deltaTime, float lerpRate)
{
	return 1.0f - expf( -fabsf(deltaTime*lerpRate) );
}

inline double frac( double a )
{
	return fmod( a, 1.0 );
}

inline float frac( float a )
{
	return fmodf( a, 1.0 );
}

inline int floorLog2(unsigned int n) 
{
   int pos = 0;
   if (n >= 1<<16) { n >>= 16; pos += 16; }
   if (n >= 1<< 8) { n >>=  8; pos +=  8; }
   if (n >= 1<< 4) { n >>=  4; pos +=  4; }
   if (n >= 1<< 2) { n >>=  2; pos +=  2; }
   if (n >= 1<< 1) {           pos +=  1; }
   return ((n == 0) ? (-1) : pos);
}
