//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#pragma once

class ICanvas2D
{
protected:
   virtual ~ICanvas2D() {}
   //
public:
   virtual void         DrawString( int x, int y, const wchar_t * text, ... )                        = 0;
   virtual void         DrawString( int x, int y, unsigned int penColor, const wchar_t * text, ... ) = 0;
   virtual void         DrawString( int x, int y, const char * text, ... )                        = 0;
   virtual void         DrawString( int x, int y, unsigned int penColor, const char * text, ... ) = 0;
   //virtual void         GetStringWidth( const char * text );
   //virtual void         GetStringWidth( const wchar_t * text );
   virtual void         DrawLine( float x0, float y0, float x1, float y1, unsigned int penColor )    = 0;
   virtual void         DrawRectangle( float x0, float y0, float width, float height, unsigned int penColor ) = 0;
   //
   virtual int          GetWidth( )                                                                = 0;
   virtual int          GetHeight( )                                                               = 0;
   //
   virtual void         CleanQueued( )                                                             = 0;
   //
};

class ICanvas3D
{
protected:
   virtual ~ICanvas3D() {}
   //
public:
   //
   virtual void         DrawBox( const D3DXVECTOR3 & v0, const D3DXVECTOR3 & v1, unsigned int penColor, unsigned int brushColor = 0x000000, const D3DXMATRIX * transform = NULL ) = 0;
   virtual void         DrawTriangle( const D3DXVECTOR3 & v0, const D3DXVECTOR3 & v1, const D3DXVECTOR3 & v2, unsigned int penColor, unsigned int brushColor = 0x000000, const D3DXMATRIX * transform = NULL ) = 0;
   virtual void         DrawQuad( const D3DXVECTOR3 & v0, const D3DXVECTOR3 & v1, const D3DXVECTOR3 & v2, const D3DXVECTOR3 & v3, unsigned int penColor, unsigned int brushColor = 0x000000, const D3DXMATRIX * transform = NULL ) = 0;
   //
};

ICanvas2D *    GetCanvas2D();
ICanvas3D *    GetCanvas3D();

//////////////////////////////////////////////////////////////////////////
