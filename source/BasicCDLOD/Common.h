//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#pragma once

//#define MY_EXTENDED_STUFF

#include "DXUT.h"

#pragma warning ( disable: 4996 )
#pragma warning ( disable: 4995 )

#include <string>
#include <vector>

#include "TiledBitmap/TiledBitmap.h"


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

   kcToggleMovieRec,
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
	float                MinX;
	float                MinY;
	float                MinZ;
	float                SizeX;
	float                SizeY;
	float                SizeZ;

	float                MaxX() const   { return MinX + SizeX; }
	float                MaxY() const   { return MinY + SizeY; }
	float                MaxZ() const   { return MinZ + SizeZ; }
};

const char *            vaGetIniPath();
const MapDimensions &   vaGetMapDimensions();
VertexAsylum::TiledBitmap* vaGetHeightmap();
std::string             vaGetExeDirectory();
std::string             vaGetProjectDirectory();
void                    vaGetProjectDefaultCameraParams( D3DXVECTOR3 & pos, const D3DXVECTOR3 & dir, float & viewRange );

const float             c_fogColor[4]           = { 0.0f, 0.5f, 0.5f, 1.0f };
const float             c_lightColorDiffuse[4]  = { 0.65f, 0.65f, 0.65f, 1.0f };
const float             c_lightColorAmbient[4]  = { 0.35f, 0.35f, 0.35f, 1.0f };


//////////////////////////////////////////////////////////////////////////
// Demo environment specific settings

bool                    vaIsKey( enum KeyCommands );
bool                    vaIsKeyClicked( enum KeyCommands );
bool                    vaIsKeyReleased( enum KeyCommands );

std::wstring   			vaFindResource( const std::wstring & file, bool showErrorMessage = true );
std::string    			vaFindResource( const std::string & file, bool showErrorMessage = true );

void                    vaGetCursorDelta( float & x, float & y );

void                    vaFatalError( const char * messageString );


//////////////////////////////////////////////////////////////////////////
// Various utility functions and stuff

std::wstring   			vaStringFormat(const wchar_t * fmtString, ...);
std::string    			vaStringFormat(const char * fmtString, ...);

std::wstring   			vaStringSimpleWiden( const std::string & s );
std::string    			vaStringSimpleNarrow( const std::wstring & s );

std::string             vaAddProjectDir( const std::string & fileName );

void                    vaGetFrustumPlanes( D3DXPLANE * pPlanes, const D3DXMATRIX & mCameraViewProj );
float	                  vaWrapAngle( float angle );

HRESULT                 vaRenderQuad( int fromX, int fromY, int toX, int toY, int texWidth, int texHeight );
HRESULT                 vaRenderQuad( int width, int height, int cutEdge = 0 );
HRESULT                 vaSetRenderTargetTexture( int index, IDirect3DTexture9 * texture );
HRESULT                 vaSetDepthStencilTexture( IDirect3DTexture9 * texture );
HRESULT                 vaGetRenderTargetData( IDirect3DTexture9 * srcTexture, IDirect3DTexture9 * destTexture );

HRESULT                 vaFillColor( D3DXVECTOR4 color, IDirect3DTexture9 * texture );

HRESULT                 vaStretchRect( IDirect3DTexture9* pSourceTexture, CONST RECT* pSourceRect, IDirect3DTexture9* pDestTexture, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter);

HRESULT                 vaDebugDisplayTexture( IDirect3DTexture9* sourceTexture, int x, int y, int width = -1, int height = -1 );

bool                    vaFileExists( const char * file );
bool                    vaFileExists( const wchar_t * file );

template<class StringType>
void                    vaSplitFilePath( const StringType & inFullPath, StringType & outDirectory, StringType & outFileName )
{
	// should be the same as stdlib.h _splitpath

   size_t np = StringType::npos;
   size_t np1 = inFullPath.find_last_of('\\');
   size_t np2 = inFullPath.find_last_of('/');
   if( np1 != StringType::npos && np2 != StringType::npos )
      np = (np1>np2)?(np1):(np2);
   else if ( np1 != StringType::npos )  np = np1;
   else if ( np2 != StringType::npos )  np = np2;

   if( np != StringType::npos )
   {
      outDirectory   = inFullPath.substr( 0, np+1 );
      outFileName    = inFullPath.substr( np+1 );
   }
   else
   {
      outFileName = inFullPath;
      outDirectory.erase();
   }

}

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

template<class T> static void erase( std::vector<T> & v )
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

ShadowMapSupport  vaGetShadowMapSupport();
bool              vaGet16BitUintVertexTextureSupport();
bool              vaGetBilinearVertexTextureFilterSupport();

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
inline T sqr(T & a)
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

// ATI Fetch4 support

// GET4 is used both as a format for exposing Fetch4 as well as the enable value
// GET1 is used only as the disable value
#define FOURCC_GET4  MAKEFOURCC('G','E','T','4')
#define FOURCC_GET1  MAKEFOURCC('G','E','T','1')

