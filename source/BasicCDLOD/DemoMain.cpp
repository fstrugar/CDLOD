//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"
#include "DXUTgui.h"

#include "DemoMain.h"

#include "DemoRenderer.h"
#include "DemoCamera.h"
#include "DemoSky.h"

#include "DxCanvas.h"

#include "TiledBitmap\TiledBitmap.h"

#include "iniparser\src\IniParser.hpp"

#include "DxShader.h"

#include <time.h>

using namespace std;

#ifdef MY_EXTENDED_STUFF
#include "RecOMatic\DxRecOMatic.h"
#include "IProf/prof.h"
#endif

bool                       g_bHelpText             = false;
bool                       g_bPause                = false;

DemoCamera *               g_pDemoCamera           = NULL;
DemoRenderer *             g_pDemoRenderer         = NULL;
DemoSky *                  g_pDemoSky              = NULL;

VertexAsylum::TiledBitmap *   g_heightmap             = NULL;

bool                       g_deviceCreated         = false;
bool                       g_deviceReset           = false;
D3DSURFACE_DESC            g_backBufferSrufaceDesc;

ShadowMapSupport           g_shadowMapSupport      = smsNotSupported;
bool                       g_BilinearVTFilter      = false;


POINT                      g_oldCursorPos;
POINT                      g_cursorCenter;
POINT                      g_prevCursorDelta0      = { 0 };
POINT                      g_prevCursorDelta1      = { 0 };
POINT                      g_cursorDelta           = { 0 };

bool                       g_Keys[kcLASTVALUE];
bool                       g_KeyUps[kcLASTVALUE];
bool                       g_KeyDowns[kcLASTVALUE];

bool                       g_hasCursorCapture   = false;
HWND                       g_hwndMain           = NULL;

MapDimensions              g_mapDims;

string                     g_cmdLine;

wstring                    g_sWindowTitle       = L"BasicCDLOD";

string                     g_sRevision          = "$Rev: 147 $";

string                     g_iniFilePath        = "";
string                     g_initialWorkingDir  = "";
string                     g_projectDir         = "";

extern CDXUTDialog         g_HUD;
extern CDXUTDialogResourceManager g_DialogResourceManager;
CDXUTDialog                g_DemoSettingsHUD;
//
CDXUTCheckBox *            g_HUDWireframe       = NULL;
CDXUTComboBox *            g_HUDShadowQuality   = NULL;
CDXUTCheckBox *            g_HUDDetailmap       = NULL;
CDXUTCheckBox *            g_HUDDebugView       = NULL;
CDXUTSlider *              g_HUDCameraRange     = NULL;
CDXUTComboBox *            g_HUDRenderGridSize  = NULL;
CDXUTSlider *              g_HUDSunHorizontal   = NULL;
CDXUTSlider *              g_HUDSunVertical     = NULL;

const float                g_HUDCameraRangeMinToMaxRatio = 6.0f;

#ifdef MY_EXTENDED_STUFF
DxRecOMatic *              g_recOMatic          = NULL;
#endif

bool                       g_skipOneRenderFrame = false;


enum DemoHUDControlID
{
   dhid_Wireframe       = 0,
   dhid_ShadowQuality   = 1,
   dhid_Detailmap       = 2,
   dhid_DebugView       = 3,
   dhid_CameraRange     = 4,
   dhid_RenderGridSize  = 5,
   dhid_SunHorizontal   = 6,
   dhid_SunVertical     = 7
};
//

void                       ReleaseCursorCapture( );
void                       vaUpdateHUD();

//

const char *               vaGetIniPath()          { return g_iniFilePath.c_str(); }
const MapDimensions &      vaGetMapDimensions()    { return g_mapDims; }
VertexAsylum::TiledBitmap *   vaGetHeightmap()        { return g_heightmap; }

void vaGetCursorDelta( float & x, float & y )
{
   x = g_prevCursorDelta1.x * 0.25f + g_prevCursorDelta0.x * 0.35f + g_cursorDelta.x * 0.4f;
   y = g_prevCursorDelta1.y * 0.25f + g_prevCursorDelta0.y * 0.35f + g_cursorDelta.y * 0.4f;
}

float	vaWrapAngle( float angle )
{
   while( angle > PI )  angle -= (float)PI*2;
   while( angle < -PI ) angle += (float)PI*2;
   return angle;
}

string vaAddProjectDir( const string & fileName )
{
   if( fileName.size() == 0 )
      return fileName;
   if( fileName.size() == 1 )
      return g_projectDir + fileName;
   if( fileName[1] == ':' || (fileName[0]=='\\' && fileName[1]=='\\') || (fileName[0]=='/' && fileName[1]=='/') )
      return fileName;
   return g_projectDir + fileName;
}

void vaInitializeHUD();

bool  vaInitialize()
{
   TCHAR szDirectory[MAX_PATH] = L"";

   if(!::GetCurrentDirectory(sizeof(szDirectory) - 1, szDirectory))
   {
      MessageBox(NULL, L"Error trying to get working directory", L"Error", MB_OK );
      return false;
   }

   string heightmapPath;

   srand( (unsigned)time(NULL) );

   g_initialWorkingDir = vaStringSimpleNarrow( wstring(szDirectory) );
   g_initialWorkingDir += "\\";

   g_iniFilePath = "";
   if( g_cmdLine != "" )
   {
      if( vaFileExists((g_initialWorkingDir + g_cmdLine).c_str()) )
      {
         g_iniFilePath = g_initialWorkingDir + g_cmdLine;
      }
      else if( vaFileExists(g_cmdLine.c_str()) )
      {
         g_iniFilePath = g_cmdLine;
      }
   }

   if( g_iniFilePath == "" )
   {
      OPENFILENAME ofn;			ZeroMemory( &ofn, sizeof(ofn) );
      wchar_t sFile[MAX_PATH];	ZeroMemory( &sFile, sizeof(sFile) );
      ofn.lStructSize = sizeof(ofn);
      ofn.lpstrFilter = L"demo ini file (.ini)\0*.ini\0\0";
      ofn.lpstrFile = sFile;
      ofn.nMaxFile = sizeof(sFile);
      ofn.lpstrTitle = L"Select map file";
      ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
      ofn.hwndOwner = DXUTGetHWND();

      if ( GetOpenFileName( &ofn ) )
      {
         g_iniFilePath = vaStringSimpleNarrow( wstring(ofn.lpstrFile) );
      }
      else
      {
         return false;
      }
   }

   IniParser   iniParser;
   if( !iniParser.Open( g_iniFilePath.c_str() ) )
   {
      MessageBoxA( NULL, vaStringFormat( "Error trying to open main config file '%s'.", g_iniFilePath.c_str() ).c_str(), "Error", MB_OK );
      goto wsInitializeError;
   }

   {
      string nameOnly;
      vaSplitFilePath<string>( g_iniFilePath, g_projectDir, nameOnly );
   }

   heightmapPath       = vaAddProjectDir( iniParser.getString( "Main:heightmapPath", "" ) );

   g_mapDims.MinX    = iniParser.getFloat("Main:MapDims_MinX", 0.0f);
   g_mapDims.MinY    = iniParser.getFloat("Main:MapDims_MinY", 0.0f);
   g_mapDims.MinZ    = iniParser.getFloat("Main:MapDims_MinZ", 0.0f);
   g_mapDims.SizeX   = iniParser.getFloat("Main:MapDims_SizeX", 0.0f);
   g_mapDims.SizeY   = iniParser.getFloat("Main:MapDims_SizeY", 0.0f);
   g_mapDims.SizeZ   = iniParser.getFloat("Main:MapDims_SizeZ", 0.0f);

   g_heightmap = VertexAsylum::TiledBitmap::Open( heightmapPath.c_str(), true );

   g_pDemoCamera     = new DemoCamera();
   g_pDemoSky        = new DemoSky();
   g_pDemoRenderer   = new DemoRenderer();

   g_pDemoCamera->Initialize();

   {
      float viewRange = iniParser.getFloat( "Camera:ViewRange", FLT_MIN );
      D3DXVECTOR3 camPos;
      camPos.x    = iniParser.getFloat( "Camera:PositionX", FLT_MIN );
      camPos.y    = iniParser.getFloat( "Camera:PositionY", FLT_MIN );
      camPos.z    = iniParser.getFloat( "Camera:PositionZ", FLT_MIN );
      float yaw   = iniParser.getFloat( "Camera:Yaw", FLT_MIN );
      float pitch = iniParser.getFloat( "Camera:Pitch", FLT_MIN );
      float speed = iniParser.getFloat( "Camera:Speed", FLT_MIN );

      if( viewRange != FLT_MIN )
         g_pDemoCamera->SetViewRange( viewRange );

      if( camPos.x != FLT_MIN && camPos.y != FLT_MIN && camPos.z != FLT_MIN )
         g_pDemoCamera->SetPosition( camPos );

      if( yaw != FLT_MIN && pitch != FLT_MIN )
         g_pDemoCamera->SetOrientation( yaw / 180.0f * (float)PI, pitch / 180.0f * (float)PI );

      if( speed != FLT_MIN )
         g_pDemoCamera->SetSpeed( speed );
   }

   g_pDemoRenderer->Start( g_heightmap, g_mapDims );

   vaInitializeHUD();

#ifdef MY_EXTENDED_STUFF
   g_recOMatic = new DxRecOMatic();
#endif

   return true;

wsInitializeError:

   SAFE_DELETE( g_heightmap );

   return false;
}

void     vaGetProjectDefaultCameraParams( D3DXVECTOR3 & pos, const D3DXVECTOR3 & dir, float & viewRange )
{
   D3DXVECTOR3 bmin( g_mapDims.MinX, g_mapDims.MinY, g_mapDims.MinZ );
   D3DXVECTOR3 bmax( g_mapDims.MaxX(), g_mapDims.MaxY(), g_mapDims.MaxZ() );

   D3DXVECTOR3 bcent = (bmin + bmax) * 0.5f;
   D3DXVECTOR3 bsize = bmax - bmin;
   
   float size = D3DXVec3Length( &bsize );

   pos = bcent - dir * (size * 0.7f);

   viewRange = 0.0f;//g_initialCameraViewRange;
}

void     vaDeinitialize()
{
   if( g_pDemoCamera != NULL )
   {
      delete g_pDemoCamera;
      g_pDemoCamera = NULL;
   }

   if( g_pDemoSky != NULL )
   {
      delete g_pDemoSky;
      g_pDemoSky = NULL;
   }

   if( g_pDemoRenderer != NULL )
   {
      g_pDemoRenderer->Stop();
      delete g_pDemoRenderer;
      g_pDemoRenderer = NULL;
   }

   SAFE_DELETE( g_heightmap );

#ifdef MY_EXTENDED_STUFF
   SAFE_DELETE( g_recOMatic );
#endif
}

//
KeyCommands	vaGetKeyBind( WPARAM vKey, LPARAM lParam )
{
   switch( vKey )
   {
   case 'W': 	         return kcForward;
   case 'S': 	         return kcBackward;
   case 'A': 	         return kcLeft;
   case 'D': 	         return kcRight;
   case 'Q': 	         return kcUp;
   case 'Z': 	         return kcDown;
   case VK_SHIFT:
   case VK_LSHIFT:
   case VK_RSHIFT:      return kcShiftKey;
   case VK_LCONTROL:
   case VK_RCONTROL:
   case VK_CONTROL:     return kcControlKey;
   case 'C': 	         return kcViewRangeDown;
   case 'V': 	         return kcViewRangeUp;
   case VK_F1:          return kcToggleHelpText;
   case VK_F6:          return kcToggleWireframe;
   case VK_F7:          return kcToggleShadowmap;
   case VK_F8:          return kcToggleDetailmap;
   case VK_F9:          return kcToggleDebugView;
   case 'P':            return kcPause;
   case 'O':            return kcOneFrameStep;
   case VK_SPACE:       return kcToggleDetailmap;

#ifdef MY_EXTENDED_STUFF
   case 'R':
      {
         if( vaIsKey( kcControlKey ) )
            return kcToggleMovieRec;
         break;
      }
   case VK_F11:         
      {
         if( vaIsKey( kcControlKey ) )
            return kcReloadAll;
         else
            return kcReloadShadersOnly;
      }
   case '8':            return kcSaveViewProjOverride;
   case '9':            return kcOrthoViewProjOverride;
   case '0':            return kcToggleViewProjOverride;
#endif
   }
   
   return kcLASTVALUE;
}
//
bool vaIsKey( KeyCommands key )
{
   return g_Keys[key];
}
//
bool vaIsKeyClicked( KeyCommands key )
{
   return g_KeyDowns[key];
}
//
bool vaIsKeyReleased( KeyCommands key )
{
   return g_KeyUps[key];
}
//
void SetCursorCapture( )
{
   if( g_hasCursorCapture ) return;
   g_hasCursorCapture = true;

   g_HUD.EnableMouseInput( false );

   GetCursorPos( &g_oldCursorPos );
   RECT dwr;
   //GetWindowRect( GetDesktopWindow(), &dwr );
   GetWindowRect( g_hwndMain, &dwr );
   ShowCursor( FALSE );
   g_cursorCenter.x = (dwr.left + dwr.right) / 2; 
   g_cursorCenter.y = (dwr.top + dwr.bottom) / 2;
   SetCursorPos( g_cursorCenter.x, g_cursorCenter.y );
   SetCapture( g_hwndMain );

   //GetWindowRect( hWnd, &dwr );
   //ClipCursor( &dwr );
}
//
void ReleaseCursorCapture( )
{
   if( !g_hasCursorCapture ) return;
   g_hasCursorCapture = false;

   SetCursorPos( g_oldCursorPos.x, g_oldCursorPos.y );
   ShowCursor( TRUE );
   //ClipCursor( NULL );
   ReleaseCapture();

   g_HUD.EnableMouseInput( true );
}
//
static void ResetAllKeys()
{
   for( int i = 0; i < kcLASTVALUE; i++ )
   {
      g_Keys[i] = false;
      g_KeyUps[i]		= false;
      g_KeyDowns[i]	= false;
   }
}
//
static void OnGotFocus( HWND hWnd )
{
   ResetAllKeys();
}
//
static void OnLostFocus( HWND hWnd )
{
   ResetAllKeys();
   ReleaseCursorCapture();
}
//
void vaMsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing )
{
   if( g_bHelpText )
   {
      // Give the dialogs a chance to handle the message first
      *pbNoFurtherProcessing = g_DemoSettingsHUD.MsgProc( hWnd, msg, wParam, lParam );
      if( *pbNoFurtherProcessing )
         return;
   }


   if( g_pDemoRenderer != NULL )
      g_pDemoRenderer->MsgProc( hWnd, msg, wParam, lParam, pbNoFurtherProcessing );

   if( *pbNoFurtherProcessing )
      return;

   KeyCommands key;

   switch( msg )
   {
   case WM_MOUSEWHEEL:
      {
         if( g_pDemoCamera == NULL ) break;

         float zDelta = GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
         float speed = g_pDemoCamera->GetSpeed();
         speed *= pow( ((zDelta>0.0)?(1.1f):(0.9f)), fabsf(zDelta) );
         g_pDemoCamera->SetSpeed(speed);
      }
      break;
   case WM_SETFOCUS:
      *pbNoFurtherProcessing = true;
      OnGotFocus( hWnd );
      break;
   case WM_KILLFOCUS:
      *pbNoFurtherProcessing = true;
      OnLostFocus( hWnd );
      break;
   case WM_KEYDOWN:
      if( wParam == VK_ESCAPE )
      {
         PostQuitMessage( 0 );
         //return 0;
      }
      key = vaGetKeyBind( wParam, lParam ) ;
      if( key != kcLASTVALUE ) 
      { 
         g_Keys[key]		= true;
         g_KeyDowns[key] = true;
      }
      break;
   case WM_KEYUP:
      key = vaGetKeyBind( wParam, lParam ) ;
      if( key != kcLASTVALUE ) 
      { 
         g_Keys[key]		= false;
         g_KeyUps[key]	= true;
      }
      break;
   case WM_RBUTTONDOWN:
      *pbNoFurtherProcessing = true;
      if( g_hasCursorCapture )
         ReleaseCursorCapture( );
      else
         SetCursorCapture( );
      break;
   case WM_MBUTTONDOWN:
      *pbNoFurtherProcessing = true;
      if( g_pDemoRenderer != NULL ) g_pDemoRenderer->SetWireframe( !g_pDemoRenderer->GetWireframe() );
      break;

   }
}

#ifdef MY_EXTENDED_STUFF
static wstring SaveFileDialog( const wchar_t * filter, const wchar_t * title  )
{
   //Select map
   OPENFILENAME ofn;			ZeroMemory( &ofn, sizeof(ofn) );
   wchar_t sFile[MAX_PATH];	ZeroMemory( &sFile, sizeof(sFile) );
   ofn.lStructSize = sizeof(ofn);
   ofn.lpstrFilter = filter;
   ofn.lpstrFile = new wchar_t[MAX_PATH];
   ofn.lpstrFile = sFile;
   ofn.nMaxFile = sizeof(sFile);
   ofn.lpstrTitle = title;
   ofn.Flags = OFN_PATHMUSTEXIST;// | OFN_FILEMUSTEXIST;

   if ( GetSaveFileName( &ofn ) )
   {
      return wstring( ofn.lpstrFile );
   }
   return L"";

}
#endif

void DrawInfoText();

void vaTick( float deltaTime )
{
#ifdef MY_EXTENDED_STUFF
   Prof_update(1);
#endif
   {
#ifdef MY_EXTENDED_STUFF
      Prof(vaTick);
#endif

#ifdef MY_EXTENDED_STUFF
      if( vaIsKeyClicked( kcToggleMovieRec ) )
      {
         if( !g_recOMatic->IsRecording() )
         {
            wstring aviPath = SaveFileDialog( L"Avi video file (.avi)\0*.*\0\0", L"Select output avi file" );
            if( aviPath != L"" )
            {
               g_recOMatic->StartRecording( vaStringSimpleNarrow( aviPath ).c_str() );
               g_skipOneRenderFrame = true;
               DXUTSetConstantFrameTime( true, g_recOMatic->GetFrameDeltaTime() );
               return;
            }
         }
         else
            g_recOMatic->StopRecording();
      }

      if( !g_recOMatic->IsRecording() )
      {
         DXUTSetConstantFrameTime( false );
      }
#endif

      //if( vaIsKeyClicked( kcToggleHelpText ) )
      //   g_bHelpText = !g_bHelpText;

      // don't need pause
      //if( vaIsKeyClicked( kcPause ) )
      //   g_bPause = !g_bPause;

      if( vaIsKeyClicked( kcOneFrameStep ) )
         g_bPause = true;

      if( vaIsKeyClicked( kcReloadShadersOnly ) || vaIsKeyClicked( kcReloadAll ) )
      {
         DxShader::ReloadAllShaders();
      }

      g_prevCursorDelta1 = g_prevCursorDelta0;
      g_prevCursorDelta0 = g_cursorDelta;
      
      g_cursorDelta.x = 0;
      g_cursorDelta.y = 0;

      if( g_hasCursorCapture )
      {
         POINT pt;
         GetCursorPos( &pt );
         SetCursorPos( g_cursorCenter.x, g_cursorCenter.y );

         g_cursorDelta.x = pt.x - g_cursorCenter.x;
         g_cursorDelta.y = pt.y - g_cursorCenter.y;
      }

      if( g_bPause )
      {
         if( vaIsKeyClicked( kcOneFrameStep ) )
            deltaTime = 1 / 60.0f;
         else
            deltaTime = 0.0f;
      }

      g_pDemoCamera->Tick( deltaTime );
      g_pDemoSky->Tick( deltaTime );
      g_pDemoRenderer->Tick( g_pDemoCamera, deltaTime );

      DrawInfoText();

      for( int i = 0; i < kcLASTVALUE; i++ )
      {
         g_KeyUps[i]		= false;
         g_KeyDowns[i]	= false;
      }

      vaUpdateHUD();
   }
}

extern void vaDrawCanvas3D( DemoCamera * camera );
extern void vaDrawCanvas2D( );

HRESULT vaRender( float deltaTime )
{
   if( g_skipOneRenderFrame )
   {
      g_skipOneRenderFrame = false;
      return S_OK;
   }

   {
#ifdef MY_EXTENDED_STUFF
      Prof(vaRender);
#endif

      HRESULT hr;

      V( g_pDemoSky->Render( g_pDemoCamera ) );
      V( g_pDemoRenderer->Render( g_pDemoCamera, g_pDemoSky, deltaTime ) );

      vaDrawCanvas3D( g_pDemoCamera );

      if( g_bHelpText )
      {
         g_DemoSettingsHUD.OnRender( deltaTime );
      }

#ifdef MY_EXTENDED_STUFF
      if( g_recOMatic->IsRecording() )
      {
         g_recOMatic->GrabBackbuffer();
         g_recOMatic->Render();
         //vaDrawCanvas2D(); // just render "we're recording" text from previous line
      }
#endif
   }
   
#ifdef MY_EXTENDED_STUFF
   Prof_draw( g_backBufferSrufaceDesc.Width - 510.0f, g_backBufferSrufaceDesc.Height - 250.0f, 500.0f, 240.0f );
#endif
   return S_OK;
}

static wstring FormatArrayToString( const int intArr[], int count )
{
   wstring outStr = L"";

   if( count == 0 )
      return outStr;

   for( int i = 0; i < count-1; i++ )
   {
      outStr += vaStringFormat(L"%03d, ", intArr[i]);
   }
   outStr += vaStringFormat(L"%03d", intArr[count-1]);
   
   return outStr;
}

void DrawInfoText()
{
   int lineHeight = 12;
   int lineYPos = 26;

   const int lineXPos = 6;

   if( g_bPause )                      GetCanvas2D()->DrawString( g_backBufferSrufaceDesc.Width/2 - 44, lineYPos, L"PAUSED" );
   lineYPos += (int)(lineHeight * 2.0);

   if( !g_bHelpText )
   {
      lineYPos += lineHeight;
   }
   else
   {
      lineYPos += lineHeight;
      lineYPos += 6;

      GetCanvas2D()->DrawString( lineXPos, lineYPos, L"Visibility range: %.1f", g_pDemoCamera->GetViewRange() );
      lineYPos += lineHeight+1;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, L"Camera: speed %.1f, yaw %.1f, pitch %.1f,", g_pDemoCamera->GetSpeed(), g_pDemoCamera->GetYaw() / (float)PI * 180, g_pDemoCamera->GetPitch() / (float)PI * 180 );
      lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, L"             pos %.1f, %.1f, %.1f", g_pDemoCamera->GetPosition().x, g_pDemoCamera->GetPosition().y, g_pDemoCamera->GetPosition().z );
      lineYPos += lineHeight;
      lineYPos += 6;

      GetCanvas2D()->DrawString( lineXPos, lineYPos, L"commands:" );   lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, L"  W,S,A,D,Q,Z: camera movement" ); lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, L"  Shift/Ctrl: boost/slow camera movement speed"); lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, L"  Right mouse btn: toggle mouse mode"); lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, L"  Mouse wheel: change camera speed"); lineYPos += lineHeight;

      const DemoRenderer::LastFrameRenderStats & renderStats = g_pDemoRenderer->GetRenderStats();

      
      lineYPos = g_backBufferSrufaceDesc.Height - 6 * lineHeight - 4;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, L"Statistics" );   lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, L"  Terrain:" );   lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, L"    total LOD levels: (%d)", renderStats.LODLevelCount ); lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, L"    quads per level: (%s)", FormatArrayToString(renderStats.TerrainStats.RenderedQuads, renderStats.LODLevelCount).c_str() ); lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, L"    quads total:        (%d)", renderStats.TerrainStats.TotalRenderedQuads ); lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, L"    triangles total: (%d)", renderStats.TerrainStats.TotalRenderedTriangles ); lineYPos += lineHeight;
      //GetCanvas2D()->DrawString( lineXPos, lineYPos, L"  Terrain shadow map:" ); lineYPos += lineHeight;
      //GetCanvas2D()->DrawString( lineXPos, lineYPos, L"    quads per level: (%s)", FormatArrayToString(renderStats.TerrainShadowStats.RenderedQuads, renderStats.LODLevelCount).c_str() ); lineYPos += lineHeight;
      //GetCanvas2D()->DrawString( lineXPos, lineYPos, L"    quads total:        (%d)", renderStats.TerrainShadowStats.TotalRenderedQuads ); lineYPos += lineHeight;
      //GetCanvas2D()->DrawString( lineXPos, lineYPos, L"    triangles total: (%d)", renderStats.TerrainShadowStats.TotalRenderedTriangles ); lineYPos += lineHeight;

   }

}

ShadowMapSupport vaGetShadowMapSupport()
{
   return g_shadowMapSupport;
}

bool vaGetBilinearVertexTextureFilterSupport()
{
   return g_BilinearVTFilter;
}

bool vaIsD3D9DeviceAcceptable( D3DCAPS9* pCaps, UINT AdapterOrdinal, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
   //// used for shadow maps
   //if( FAILED( DXUTGetD3D9Object()->CheckDeviceFormat( AdapterOrdinal, DeviceType, AdapterFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, D3DFMT_D24S8 ) )
   //   return false;

   // vertex texture support
   if( FAILED( DXUTGetD3D9Object()->CheckDeviceFormat( AdapterOrdinal, DeviceType, AdapterFormat, D3DUSAGE_QUERY_VERTEXTEXTURE, D3DRTYPE_TEXTURE, D3DFMT_R32F ) ) )
      return false;

   // used for normals (used to be sampled in vertex shader for testing purposes, thus the strange 16bit format)
   if( FAILED( DXUTGetD3D9Object()->CheckDeviceFormat( AdapterOrdinal, DeviceType, AdapterFormat, D3DUSAGE_QUERY_FILTER, D3DRTYPE_TEXTURE, D3DFMT_G16R16 ) ) )
      return false;

   // vertex shader support
   if( pCaps->VertexShaderVersion < D3DVS_VERSION(3,0) )
      return false;

   // pixel shader support
   if( pCaps->PixelShaderVersion < D3DPS_VERSION(3,0) )
      return false;

   if( !(pCaps->RasterCaps & D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS) )
      return false;

   return true;
}

HRESULT CheckResourceFormatSupport(IDirect3DDevice9* pd3dDevice, D3DFORMAT fmt, D3DRESOURCETYPE resType, DWORD dwUsage)
{
   HRESULT hr = S_OK;
   IDirect3D9* tempD3D = NULL;
   pd3dDevice->GetDirect3D(&tempD3D);
   D3DCAPS9 devCaps;
   pd3dDevice->GetDeviceCaps(&devCaps);

   D3DDISPLAYMODE displayMode;
   tempD3D->GetAdapterDisplayMode(devCaps.AdapterOrdinal, &displayMode);

   hr = tempD3D->CheckDeviceFormat(devCaps.AdapterOrdinal, devCaps.DeviceType, displayMode.Format, dwUsage, resType, fmt);

   tempD3D->Release(), tempD3D = NULL;

   return hr;
}

void vaOnCreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc )
{
   HRESULT hr;
   D3DDEVICE_CREATION_PARAMETERS cp;
   pd3dDevice->GetCreationParameters(&cp);

   IDirect3D9 * pD3D9 = NULL;
   V( pd3dDevice->GetDirect3D(&pD3D9) );

   D3DCAPS9 devCaps;
   pd3dDevice->GetDeviceCaps(&devCaps);

   D3DDISPLAYMODE displayMode;
   DXUTGetD3D9Object()->GetAdapterDisplayMode(devCaps.AdapterOrdinal, &displayMode);

   if( SUCCEEDED( pD3D9->CheckDeviceFormat( cp.AdapterOrdinal, cp.DeviceType, pBackBufferSurfaceDesc->Format, 
      D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, (D3DFORMAT)(MAKEFOURCC('D','F','2','4')) ) ) )
   {
      g_shadowMapSupport = smsATIShadows;
   }
   else if( SUCCEEDED( DXUTGetD3D9Object()->CheckDeviceFormat(devCaps.AdapterOrdinal, devCaps.DeviceType, displayMode.Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, D3DFMT_D24S8 ) ) )
   {
      g_shadowMapSupport = smsNVidiaShadows;
   }
   else
   {
      g_shadowMapSupport = smsNotSupported;
   }

   SAFE_RELEASE( pD3D9 );

   //g_16BitUintVTSupport = SUCCEEDED( DXUTGetD3D9Object()->CheckDeviceFormat( devCaps.AdapterOrdinal, devCaps.DeviceType, displayMode.Format, D3DUSAGE_QUERY_VERTEXTEXTURE, D3DRTYPE_TEXTURE, D3DFMT_L16 ) );

   g_BilinearVTFilter = (devCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR) != 0 && (devCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR);
}

HRESULT vaOnResetDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc )
{

   pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
   pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

   g_DemoSettingsHUD.SetLocation( pBackBufferSurfaceDesc->Width - 200, 100 );
   g_DemoSettingsHUD.SetSize( 194, 300 );

   return S_OK;
}

void CALLBACK vaOnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
   if( g_pDemoRenderer == NULL )
      return;

   switch( nControlID )
   {
   case dhid_Wireframe:
      g_pDemoRenderer->SetWireframe( g_HUDWireframe->GetChecked() );
      break;
   case dhid_ShadowQuality:
      g_pDemoRenderer->SetShadowQuality( g_HUDShadowQuality->GetSelectedIndex() );
      break;
   case dhid_Detailmap:
      g_pDemoRenderer->SetDetailmap( g_HUDDetailmap->GetChecked() );
      break;
   case dhid_DebugView:
      g_pDemoRenderer->SetDebugView( g_HUDDebugView->GetChecked() );
      break;
   case dhid_CameraRange:
      {
         float minValue = g_pDemoRenderer->GetSettings().MinViewRange;
         float maxValue = g_pDemoRenderer->GetSettings().MaxViewRange;
         float val  = (g_HUDCameraRange->GetValue() / 1000.0f) * (maxValue - minValue) + minValue;
         g_pDemoCamera->SetViewRange( val );
      }
      break;
   case dhid_RenderGridSize:
      {
         g_pDemoRenderer->SetRenderGridResolutionMult( (int)g_HUDRenderGridSize->GetSelectedData() );
      }
      break;
   case dhid_SunHorizontal:
   case dhid_SunVertical:
      {
         float sunAzimuth  = g_HUDSunHorizontal->GetValue() / 1000.0f * (2.0f * (float)PI);
         float sunElevation= 0.06f + g_HUDSunVertical->GetValue() / 1000.0f * (0.45f * (float)PI);
         g_pDemoSky->SetSunPosition( sunAzimuth, sunElevation );
      }
   default:
      break;
   }
}

void vaInitializeHUD()
{
   g_DemoSettingsHUD.Init( &g_DialogResourceManager );

   g_DemoSettingsHUD.SetCallback( vaOnGUIEvent ); int iY = 10;
   g_DemoSettingsHUD.SetBackgroundColors( 0x80808080 );

   int iX = 10;
   int lineHeight = 24;
   int lineWidth = 173;

   g_DemoSettingsHUD.AddCheckBox( dhid_Wireframe, L"Wireframe (F6)", iX, iY, lineWidth, 22, false, 0, false, &g_HUDWireframe );
   iY += lineHeight + 4;
   g_DemoSettingsHUD.AddStatic( -1, L"Cascaded shadow maps (F7):", iX, iY, lineWidth, 22 );
   iY += lineHeight;
   g_DemoSettingsHUD.AddComboBox( dhid_ShadowQuality, iX, iY, lineWidth, 22, 0, false, &g_HUDShadowQuality );
   iY += lineHeight + 4;
   
   if( vaGetShadowMapSupport() != smsNotSupported )
   {
      g_HUDShadowQuality->AddItem( L"Disabled", NULL );
      g_HUDShadowQuality->AddItem( L"Low quality", NULL );
      g_HUDShadowQuality->AddItem( L"High quality", NULL );
   }
   else
   {
      g_HUDShadowQuality->AddItem( L"Disabled (not supported)", NULL );
      g_HUDShadowQuality->SetEnabled( false );
   }

   g_DemoSettingsHUD.AddCheckBox( dhid_Detailmap, L"Detail map (F8)", iX, iY, lineWidth, 22, false, 0, false, &g_HUDDetailmap );
   if( !g_pDemoRenderer->GetDetailmapExists() ) g_HUDDetailmap->SetEnabled( false );
   iY += lineHeight + 4;

   g_DemoSettingsHUD.AddCheckBox( dhid_DebugView, L"Debug view (F9)", iX, iY, lineWidth, 22, false, 0, false, &g_HUDDebugView );
   iY += lineHeight + 4;


   float minValue = g_pDemoRenderer->GetSettings().MinViewRange;
   float maxValue = g_pDemoRenderer->GetSettings().MaxViewRange;
   int initialValue = (int)(((g_pDemoCamera->GetViewRange() - minValue) / (maxValue - minValue)) * 1000.0f+0.5);
   g_DemoSettingsHUD.AddStatic( -1, L"View range & LOD detail level:", iX, iY, lineWidth, 22 );
   iY += lineHeight;
   g_DemoSettingsHUD.AddSlider( dhid_CameraRange, iX, iY, lineWidth, 22, 0, 1000, initialValue, false, &g_HUDCameraRange );
   iY += lineHeight + 4;

   g_DemoSettingsHUD.AddStatic( -1, L"Render grid size:", iX, iY, lineWidth, 22 );
   iY += lineHeight;
   g_DemoSettingsHUD.AddComboBox( dhid_RenderGridSize, iX, iY, lineWidth, 22, 0, false, &g_HUDRenderGridSize );
   iY += lineHeight + 4;

   const DemoRenderer::Settings & settings = g_pDemoRenderer->GetSettings();
   int leafSize = settings.LeafQuadTreeNodeSize;
   int maxGridMult = g_pDemoRenderer->GetMaxRenderGridResolutionMult();
   for( int i = 0, m = 1; m <= maxGridMult; i++, m *= 2 )
   {
      int gridSize = leafSize * m;
      g_HUDRenderGridSize->AddItem( vaStringFormat(L"%d x %d (%d mult)", gridSize, gridSize, m ).c_str(), (void*)m );
      if( settings.RenderGridResolutionMult == m )
         g_HUDRenderGridSize->SetSelectedByIndex( i );
   }

   g_DemoSettingsHUD.AddStatic( -1, L"Sun azimuth/elevation:", iX, iY, lineWidth, 22 );
   iY += lineHeight;
   g_DemoSettingsHUD.AddSlider( dhid_SunHorizontal, iX+1, iY, lineWidth/2, 22, 0, 1000, initialValue, false, &g_HUDSunHorizontal );
   //iY += lineHeight;
   g_DemoSettingsHUD.AddSlider( dhid_SunVertical, iX+lineWidth/2 + 23, iY, lineWidth/2-24, 22, 0, 1000, initialValue, false, &g_HUDSunVertical );
   iY += lineHeight + 4;

   float sunAzimuth, sunElevation;
   g_pDemoSky->GetSunPosition( sunAzimuth, sunElevation );
   g_HUDSunHorizontal->SetValue( (int)(0.5f + sunAzimuth / (2.0f * (float)PI) * 1000.0f ) );
   g_HUDSunVertical->SetValue( (int)(0.5f + (sunElevation-0.06f) / (0.45f * (float)PI) * 1000.0f ) );
   vaOnGUIEvent( 0, dhid_SunHorizontal, g_HUDSunHorizontal, NULL );

   vaUpdateHUD();
}

void vaUpdateHUD()
{
   if( g_pDemoRenderer == NULL )
      return;

   if( g_HUDWireframe->GetChecked() != g_pDemoRenderer->GetWireframe() )
      g_HUDWireframe->SetChecked( g_pDemoRenderer->GetWireframe() );

   if( g_HUDShadowQuality->GetSelectedIndex() != g_pDemoRenderer->GetShadowQuality() )
      g_HUDShadowQuality->SetSelectedByIndex( g_pDemoRenderer->GetShadowQuality( ) );

   if( g_HUDDetailmap->GetChecked() != g_pDemoRenderer->GetSettings().DetailHeightmapEnabled )
      g_HUDDetailmap->SetChecked( g_pDemoRenderer->GetSettings().DetailHeightmapEnabled );

   if( g_HUDDebugView->GetChecked() != g_pDemoRenderer->GetDebugView() )
      g_HUDDebugView->SetChecked( g_pDemoRenderer->GetDebugView() );
}