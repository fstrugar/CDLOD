//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////


#include "DXUT.h"
#include "SDKmisc.h"

#include "Common.h"

#include <stdio.h>
#include <stdarg.h>

#include "DxCanvas.h"
#include "DemoCamera.h"
#include "DxEventNotifier.h"

using namespace std;

extern bool                      g_bHelpText;

extern CDXUTTextHelper*          g_pTxtHelper;

static D3DCOLORVALUE U32_TO_D3DCOLORVALUE( unsigned int c )
{
   D3DCOLORVALUE cv = { ( ( c >> 16 ) & 0xFF ) / 255.0f,
      ( ( c >> 8 ) & 0xFF ) / 255.0f,
      ( c & 0xFF ) / 255.0f,
      ( ( c >> 24 ) & 0xFF ) / 255.0f };
   return cv;
}

class Canvas2DDX9 : public ICanvas2D, protected DxEventReceiver
{
   struct DrawStringItem
   {
      int         x, y;
      unsigned int penColor;
      wstring      text;
      DrawStringItem( int x, int y, unsigned int penColor, const wstring & text ) : x(x), y(y), penColor(penColor), text(text) {}
   };
   //
   struct DrawLineItem
   {
      float x0;
      float y0;
      float x1;
      float y1;
      unsigned int penColor;

      DrawLineItem( float x0, float y0, float x1, float y1, unsigned int penColor )
         : x0(x0), y0(y0), x1(x1), y1(y1), penColor(penColor) { }
   };
   //
   vector<DrawStringItem>     m_DrawStringLines;
   vector<DrawLineItem>       m_drawLines;
   //
   D3DSURFACE_DESC            m_backBufferSurfaceDesc;
   //
public:
   virtual void         DrawString( int x, int y, const wchar_t * text, ... )
   {
	   va_list args;
	   va_start(args, text);

	   int nBuf;
	   wchar_t szBuffer[1024];

	   nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer) / sizeof(wchar_t), text, args);
	   assert(nBuf < sizeof(szBuffer));//Output truncated as it was > sizeof(szBuffer)

	   va_end(args);

      m_DrawStringLines.push_back( DrawStringItem(x, y, 0xFFFFFF00, wstring(szBuffer) ) );      
   }

   virtual void         DrawString( int x, int y, unsigned int penColor, const wchar_t * text, ... )
   {
	   va_list args;
	   va_start(args, text);

	   int nBuf;
	   wchar_t szBuffer[1024];

	   nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer) / sizeof(wchar_t), text, args);
	   assert(nBuf < sizeof(szBuffer));//Output truncated as it was > sizeof(szBuffer)

	   va_end(args);

      m_DrawStringLines.push_back( DrawStringItem(x, y, penColor, wstring(szBuffer) ) );
   }
   //
   virtual void         DrawString( int x, int y, const char * text, ... )
   {
      va_list args;
      va_start(args, text);

      int nBuf;
      char szBuffer[1024];

      nBuf = _vsnprintf(szBuffer, sizeof(szBuffer) / sizeof(char), text, args);
      assert(nBuf < sizeof(szBuffer));//Output truncated as it was > sizeof(szBuffer)

      va_end(args);

      m_DrawStringLines.push_back( DrawStringItem(x, y, 0xFFFFFF00, vaStringSimpleWiden( string(szBuffer) ) ) );      
   }

   virtual void         DrawString( int x, int y, unsigned int penColor, const char * text, ... )
   {
      va_list args;
      va_start(args, text);

      int nBuf;
      char szBuffer[1024];

      nBuf = _vsnprintf(szBuffer, sizeof(szBuffer) / sizeof(char), text, args);
      assert(nBuf < sizeof(szBuffer));//Output truncated as it was > sizeof(szBuffer)

      va_end(args);

      m_DrawStringLines.push_back( DrawStringItem(x, y, penColor, vaStringSimpleWiden( string(szBuffer) ) ) );
   }
   //
   virtual void DrawLine( float x0, float y0, float x1, float y1, unsigned int penColor )
   {
      m_drawLines.push_back( DrawLineItem( x0, y0, x1, y1, penColor ) );
   }
   //
   virtual void DrawRectangle( float x0, float y0, float width, float height, unsigned int penColor )
   {
      DrawLine( x0, y0, x0 + width, y0, penColor );
      DrawLine( x0 + width, y0, x0 + width, y0 + height, penColor );
      DrawLine( x0 + width, y0 + height, x0, y0 + height, penColor );
      DrawLine( x0, y0 + height, x0, y0, penColor );
   }
   //
protected:
   virtual HRESULT   OnResetDevice(const D3DSURFACE_DESC* pBackBufferSurfaceDesc)
   { 
      m_backBufferSurfaceDesc = *pBackBufferSurfaceDesc;
      return S_OK; 
   }
public:
   //
   void           CleanQueued()
   {
      ::erase( m_drawLines );
      ::erase( m_DrawStringLines );
   }
   //
   void              Initialize()
   {
   }
   //
   void              Deinitialize()
   {
   }
   //
   void              Draw()
   {
      g_pTxtHelper->Begin();
      g_pTxtHelper->SetInsertionPos( 5, 5 );
      g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
      g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
      g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

      g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 0.0f, 0.0f, 0.0f, 0.6f ) );

      for( size_t i = 0; i < m_DrawStringLines.size(); i++ )
      {
         g_pTxtHelper->SetInsertionPos( m_DrawStringLines[i].x+1, m_DrawStringLines[i].y+1 );
         g_pTxtHelper->DrawTextLine( m_DrawStringLines[i].text.c_str() );
         //m_glfont.DrawString( m_DrawStringLines[i].text, 0.5f, (float)m_DrawStringLines[i].x, height - (float)m_DrawStringLines[i].y, top_color, bottom_color );
      }

      for( size_t i = 0; i < m_DrawStringLines.size(); i++ )
      {
         g_pTxtHelper->SetInsertionPos( m_DrawStringLines[i].x, m_DrawStringLines[i].y );
         g_pTxtHelper->SetForegroundColor( D3DXCOLOR( m_DrawStringLines[i].penColor ) );
         g_pTxtHelper->DrawTextLine( m_DrawStringLines[i].text.c_str() );
         //m_glfont.DrawString( m_DrawStringLines[i].text, 0.5f, (float)m_DrawStringLines[i].x, height - (float)m_DrawStringLines[i].y, top_color, bottom_color );
      }

      g_pTxtHelper->End();

      ::erase( m_DrawStringLines );

      HRESULT hr;
      IDirect3DDevice9* device = DxEventNotifier::GetD3DDevice();

      device->SetTexture( 0, NULL );

      device->SetVertexShader( NULL );
      device->SetPixelShader( NULL );
      device->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
      device->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );

      //device->SetRenderState( D3DRS_LIGHTING, TRUE );

      device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
      device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
      device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );

      device->SetRenderState( D3DRS_ZENABLE, FALSE );

      device->SetFVF( TransformedColoredVertex::FVF );

      {
         TransformedColoredVertex vertices[2];

         for( size_t i = 0; i < m_drawLines.size(); i++ )
         {
            vertices[0] = TransformedColoredVertex( m_drawLines[i].x0, m_drawLines[i].y0, 0.0f, 0.5f, m_drawLines[i].penColor );
            vertices[1] = TransformedColoredVertex( m_drawLines[i].x1, m_drawLines[i].y1, 0.0f, 0.5f, m_drawLines[i].penColor );

            //mat.Diffuse = U32_TO_D3DCOLORVALUE( m_drawLines[i].penColor );
            //mat.Emissive = U32_TO_D3DCOLORVALUE( m_drawLines[i].penColor );
            //V( device->SetMaterial( &mat ) );

            V( device->DrawPrimitiveUP( D3DPT_LINELIST, 2, vertices, sizeof( TransformedColoredVertex ) ) );
         }
      }

      device->SetRenderState( D3DRS_ZENABLE, TRUE );

      device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
      device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
      device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

      device->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );

      device->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );

      ::erase( m_drawLines );
   }
   //
   virtual int          GetWidth( )
   {
      return m_backBufferSurfaceDesc.Width;
   }
   //
   virtual int          GetHeight( )
   {
      return m_backBufferSurfaceDesc.Height;
   }
   //
};

class Canvas3DDX9 : public ICanvas3D
{
   enum DrawItemType
   {
      Triangle,
      Box,
   };
   //
   struct DrawItem
   {
      D3DXVECTOR3       v0;
      D3DXVECTOR3       v1;
      D3DXVECTOR3       v2;
      unsigned int      penColor;
      unsigned int      brushColor;
      DrawItemType   type;
      DrawItem( const D3DXVECTOR3 & v0, const D3DXVECTOR3 & v1, const D3DXVECTOR3 & v2, unsigned int penColor, unsigned int brushColor, DrawItemType type ) 
         : v0(v0), v1(v1), v2(v2), penColor(penColor), brushColor(brushColor), type(type) { }
   };
   //
   vector<DrawItem>  m_drawItems;
   //
public:
   //
   void           Initialize()
   {
   }
   //
   void           Deinitialize()
   {
   }
   //
   void           Render( DemoCamera * camera )
   {
      //glEnable(GL_COLOR_MATERIAL);

      IDirect3DDevice9 * device = DxEventNotifier::GetD3DDevice();

      const D3DXMATRIX & view = camera->GetViewMatrix();
      const D3DXMATRIX & proj = camera->GetProjMatrix();

      //D3DXMATRIX proj;
      //D3DXMatrixPerspectiveFovLH( &proj, camera->GetFOV(), camera->GetAspect(), 10.0f, 2e8 );

      //D3DXMATRIX viewProj = view * proj;

      D3DXMATRIXA16 matWorld;
      //D3DXMatrixTranslation( &matWorld, m_directionalLightDir.x * -1e8f, m_directionalLightDir.y * -1e8f, m_directionalLightDir.z * -1e8f );
      D3DXMatrixIdentity( &matWorld );
      device->SetTransform( D3DTS_WORLD, &matWorld );
      device->SetTransform( D3DTS_VIEW, &view );
      device->SetTransform( D3DTS_PROJECTION, &proj );

      D3DMATERIAL9 mat;
      mat.Ambient = U32_TO_D3DCOLORVALUE( 0 );
      mat.Diffuse = U32_TO_D3DCOLORVALUE( 0 );
      mat.Emissive = U32_TO_D3DCOLORVALUE( 0 );
      mat.Specular = U32_TO_D3DCOLORVALUE( 0 );
      mat.Power = 1.0f;

      device->SetFVF( PositionVertex::FVF );

      device->SetTexture( 0, NULL );

      device->SetVertexShader( NULL );
      device->SetPixelShader( NULL );
      device->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
      device->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );

      device->SetRenderState( D3DRS_LIGHTING, TRUE );

      device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
      device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
      device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );

      device->SetRenderState( D3DRS_ZENABLE, TRUE );

      //device->SetRenderState( D3DRS_COLORVERTEX, TRUE );
      //device->SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL );

      for( size_t i = 0; i < m_drawItems.size(); i++ )
      {
         DrawItem & item = m_drawItems[i];

         if( item.type == Triangle )
         {
            D3DXVECTOR3 a0(item.v0);
            D3DXVECTOR3 a1(item.v1);
            D3DXVECTOR3 a2(item.v2);

            if( (item.brushColor & 0xFF000000) != 0 )
            {
               device->SetRenderState( D3DRS_ZWRITEENABLE, (item.brushColor >> 24) == 0xFF );

               mat.Diffuse = U32_TO_D3DCOLORVALUE( item.brushColor );
               mat.Emissive = U32_TO_D3DCOLORVALUE( item.brushColor );
               device->SetMaterial( &mat );

               PositionVertex vertices[3];

               int index = 0;

               vertices[index++] = PositionVertex( a0 );
               vertices[index++] = PositionVertex( a1 );
               vertices[index++] = PositionVertex( a2 );


               device->DrawPrimitiveUP( D3DPT_TRIANGLELIST, index/3, vertices, sizeof( PositionVertex ) );
            }

            if( (item.penColor & 0xFF000000) != 0 )
            {
               device->SetRenderState( D3DRS_ZWRITEENABLE, (item.penColor >> 24) == 0xFF );

               mat.Diffuse = U32_TO_D3DCOLORVALUE( item.penColor );
               mat.Emissive = U32_TO_D3DCOLORVALUE( item.penColor );
               device->SetMaterial( &mat );

               PositionVertex vertices[4];

               int index = 0;

               vertices[index++] = PositionVertex( a0 );
               vertices[index++] = PositionVertex( a1 );
               vertices[index++] = PositionVertex( a2 );
               vertices[index++] = PositionVertex( a0 );

               device->DrawPrimitiveUP( D3DPT_LINESTRIP, 3, vertices, sizeof( PositionVertex ) );
            }

         }

         if( item.type == Box )
         {

            const D3DXVECTOR3 & boxMin = item.v0;
            const D3DXVECTOR3 & boxMax = item.v1;

            D3DXVECTOR3 a0(boxMin.x, boxMin.y, boxMin.z);
            D3DXVECTOR3 a1(boxMax.x, boxMin.y, boxMin.z);
            D3DXVECTOR3 a2(boxMax.x, boxMax.y, boxMin.z);
            D3DXVECTOR3 a3(boxMin.x, boxMax.y, boxMin.z);
            D3DXVECTOR3 b0(boxMin.x, boxMin.y, boxMax.z);
            D3DXVECTOR3 b1(boxMax.x, boxMin.y, boxMax.z);
            D3DXVECTOR3 b2(boxMax.x, boxMax.y, boxMax.z);
            D3DXVECTOR3 b3(boxMin.x, boxMax.y, boxMax.z);

            if( (item.brushColor & 0xFF000000) != 0 )
            {
               device->SetRenderState( D3DRS_ZWRITEENABLE, (item.brushColor >> 24) == 0xFF );

               mat.Diffuse = U32_TO_D3DCOLORVALUE( item.brushColor );
               mat.Emissive = U32_TO_D3DCOLORVALUE( item.brushColor );
               device->SetMaterial( &mat );

               PositionVertex vertices[36];

               int index = 0;

               vertices[index++] = PositionVertex( a0 );
               vertices[index++] = PositionVertex( a1 );
               vertices[index++] = PositionVertex( a2 );
               vertices[index++] = PositionVertex( a2 );
               vertices[index++] = PositionVertex( a3 );
               vertices[index++] = PositionVertex( a0 );

               vertices[index++] = PositionVertex( b0 );
               vertices[index++] = PositionVertex( b1 );
               vertices[index++] = PositionVertex( b2 );
               vertices[index++] = PositionVertex( b2 );
               vertices[index++] = PositionVertex( b3 );
               vertices[index++] = PositionVertex( b0 );

               vertices[index++] = PositionVertex( a0 );
               vertices[index++] = PositionVertex( a1 );
               vertices[index++] = PositionVertex( b1 );
               vertices[index++] = PositionVertex( b1 );
               vertices[index++] = PositionVertex( b0 );
               vertices[index++] = PositionVertex( a0 );

               vertices[index++] = PositionVertex( a1 );
               vertices[index++] = PositionVertex( a2 );
               vertices[index++] = PositionVertex( b2 );
               vertices[index++] = PositionVertex( b1 );
               vertices[index++] = PositionVertex( b2 );
               vertices[index++] = PositionVertex( a1 );

               vertices[index++] = PositionVertex( a2 );
               vertices[index++] = PositionVertex( a3 );
               vertices[index++] = PositionVertex( b3 );
               vertices[index++] = PositionVertex( b3 );
               vertices[index++] = PositionVertex( b2 );
               vertices[index++] = PositionVertex( a2 );

               vertices[index++] = PositionVertex( a3 );
               vertices[index++] = PositionVertex( a0 );
               vertices[index++] = PositionVertex( b0 );
               vertices[index++] = PositionVertex( b0 );
               vertices[index++] = PositionVertex( b3 );
               vertices[index++] = PositionVertex( a3 );

               device->DrawPrimitiveUP( D3DPT_TRIANGLELIST, index/3, vertices, sizeof( PositionVertex ) );
            }

            if( (item.penColor & 0xFF000000) != 0 )
            {
               device->SetRenderState( D3DRS_ZWRITEENABLE, (item.penColor >> 24) == 0xFF );

               mat.Diffuse = U32_TO_D3DCOLORVALUE( item.penColor );
               mat.Emissive = U32_TO_D3DCOLORVALUE( item.penColor );
               device->SetMaterial( &mat );

               PositionVertex vertices[24];

               int index = 0;

               vertices[index++] = PositionVertex( a0 );
               vertices[index++] = PositionVertex( a1 );
               vertices[index++] = PositionVertex( a1 );
               vertices[index++] = PositionVertex( a2 );
               vertices[index++] = PositionVertex( a2 );
               vertices[index++] = PositionVertex( a3 );
               vertices[index++] = PositionVertex( a3 );
               vertices[index++] = PositionVertex( a0 );
               vertices[index++] = PositionVertex( a0 );
               vertices[index++] = PositionVertex( b0 );
               vertices[index++] = PositionVertex( a1 );
               vertices[index++] = PositionVertex( b1 );
               vertices[index++] = PositionVertex( a2 );
               vertices[index++] = PositionVertex( b2 );
               vertices[index++] = PositionVertex( a3 );
               vertices[index++] = PositionVertex( b3 );
               vertices[index++] = PositionVertex( b0 );
               vertices[index++] = PositionVertex( b1 );
               vertices[index++] = PositionVertex( b1 );
               vertices[index++] = PositionVertex( b2 );
               vertices[index++] = PositionVertex( b2 );
               vertices[index++] = PositionVertex( b3 );
               vertices[index++] = PositionVertex( b3 );
               vertices[index++] = PositionVertex( b0 );

               device->DrawPrimitiveUP( D3DPT_LINELIST, index/2, vertices, sizeof( PositionVertex ) );
            }
         }
      }

      //device->SetRenderState( D3DRS_COLORVERTEX, TRUE );
      //device->SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1 );

      device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
      device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
      device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

      device->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );

      device->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );

      m_drawItems.erase(m_drawItems.begin(), m_drawItems.end());
      //glDisable(GL_BLEND);
   }
   //
   virtual void   DrawBox( const D3DXVECTOR3 & v0, const D3DXVECTOR3 & v1, unsigned int penColor, unsigned int brushColor, const D3DXMATRIX * transform )
   {
      assert( transform == NULL );
      m_drawItems.push_back( DrawItem( v0, v1, D3DXVECTOR3(0,0,0), penColor, brushColor, Box ) );
   }
   virtual void   DrawTriangle( const D3DXVECTOR3 & v0, const D3DXVECTOR3 & v1, const D3DXVECTOR3 & v2, unsigned int penColor, unsigned int brushColor, const D3DXMATRIX * transform )
   {
      assert( transform == NULL );
      m_drawItems.push_back( DrawItem( v0, v1, v2, penColor, brushColor, Triangle ) );
   }
   //
   virtual void   DrawQuad( const D3DXVECTOR3 & v0, const D3DXVECTOR3 & v1, const D3DXVECTOR3 & v2, const D3DXVECTOR3 & v3, unsigned int penColor, unsigned int brushColor, const D3DXMATRIX * transform )
   {
      DrawTriangle(v0, v1, v2, penColor, brushColor, transform);
      DrawTriangle(v2, v1, v3, penColor, brushColor, transform);
   }
   //
};

Canvas2DDX9           g_Canvas2D;
Canvas3DDX9           g_Canvas3D;

ICanvas2D *    GetCanvas2D()
{
   return &g_Canvas2D;
}

ICanvas3D *    GetCanvas3D()
{
   return &g_Canvas3D;
}

void vaDrawCanvas2D()
{
   g_Canvas2D.Draw();
}

void vaDrawCanvas3D( DemoCamera * camera )
{
   g_Canvas3D.Render( camera );
}

/*
void CanvasInitialize()
{
   g_Canvas2D.Initialize();
   g_Canvas3D.Initialize();
}

void CanvasDeinitialize()
{
   g_Canvas2D.Deinitialize();
   g_Canvas3D.Deinitialize();
}

void DrawCanvas2D()
{
   g_Canvas2D.DrawGL();
}

void DrawCanvas3D(aCameraGL * camera)
{
   g_Canvas3D.DrawGL(camera);
}
*/
