//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"
#include "DXUTgui.h"

#include "DemoMain.h"

#include "DemoRenderer.h"
#include "DemoCamera.h"
#include "DemoSky.h"

#include "CDLODStreaming\CDLODStreamingStorage.h"
#include "CDLODTools.h"

#include "DxCanvas.h"

#include "TiledBitmap\TiledBitmap.h"

#include "iniparser\src\IniParser.hpp"

#include "DxShader.h"

#include <time.h>

#include "ProgressDlg.h"

#include "SimpleProfiler.h"

#ifdef USE_REC_O_MATIC
#include "RecOMatic\DxRecOMatic.h"
#endif

using namespace std;

bool                       g_bHelpText             = false;
bool                       g_bPause                = false;

DemoCamera *               g_pDemoCamera           = NULL;
DemoRenderer *             g_pDemoRenderer         = NULL;
DemoSky *                  g_pDemoSky              = NULL;
CDLODStreamingStorage *    g_pTerrainStorage       = NULL;

bool                       g_deviceCreated         = false;
bool                       g_deviceReset           = false;
D3DSURFACE_DESC            g_backBufferSrufaceDesc;

ShadowMapSupport           g_shadowMapSupport      = smsNotSupported;
bool                       g_BilinearVTFilter      = false;
D3DFORMAT                  g_VertexTextureFormat   = D3DFMT_UNKNOWN;

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

string                     g_sourceFilePath     = "";
string                     g_destFilePath       = "";

wstring                    g_sWindowTitle       = L"StreamingCDLOD";

string                     g_sRevision          = "155";

IniParser                  g_iniFileParser;

string                     g_initialWorkingDir  = "";
string                     g_projectDir         = "";

extern CDXUTDialog         g_HUD;
extern CDXUTDialogResourceManager g_DialogResourceManager;
CDXUTDialog                g_DemoSettingsHUD;
//
CDXUTCheckBox *            g_HUDWireframe          = NULL;
//CDXUTComboBox *            g_HUDShadowQuality      = NULL;
CDXUTCheckBox *            g_HUDDetailmap          = NULL;
CDXUTCheckBox *            g_HUDDebugView          = NULL;
CDXUTSlider *              g_HUDCameraRange        = NULL;
CDXUTComboBox *            g_HUDRenderGridSize     = NULL;
CDXUTCheckBox *            g_HUDLightingEnabled    = NULL;
CDXUTSlider *              g_HUDSunHorizontal      = NULL;
CDXUTSlider *              g_HUDSunVertical        = NULL;
CDXUTComboBox *            g_HUDShowDeferredBuffer = NULL;
CDXUTComboBox *            g_HUDTexturingMode      = NULL;

const float                g_HUDCameraRangeMinToMaxRatio = 6.0f;

#ifdef USE_REC_O_MATIC
DxRecOMatic *              g_recOMatic          = NULL;
#endif

bool                       g_skipOneRenderFrame = false;


enum DemoHUDControlID
{
   dhid_Wireframe          = 0,
   dhid_ShadowQuality      = 1,
   dhid_Detailmap          = 2,
   dhid_DebugView          = 3,
   dhid_CameraRange        = 4,
   dhid_RenderGridSize     = 5,
   dhid_LightingToggle     = 6,
   dhid_SunHorizontal      = 7,
   dhid_SunVertical        = 8,
   dhid_ShowDeferredBuffer = 9,
   dhid_TextureMode        = 10,
};
//

void                       ReleaseCursorCapture( );
void                       vaUpdateHUD();

//

const IniParser &          vaGetIniParser()          { return g_iniFileParser; }

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

bool vaLoadFileToMemory( const char *fileName, void * & outBuffer, int & outSize )
{
   FILE *f = fopen( fileName, "rb" );
   if (f == NULL) 
      return false;

   fseek(f, 0, SEEK_END);
   outSize = ftell(f);
   fseek(f, 0, SEEK_SET);

   outBuffer = NULL;

   if( outSize == 0 )
      return true;

   outBuffer = malloc( outSize );
   if( (size_t)outSize != fread( outBuffer, sizeof(char), outSize, f ) )
   {
      free(outBuffer);
      outBuffer = NULL;
      outSize = 0;
      return false;
   }
   fclose(f);

   return true;
}



static void splitParams(const string & cmdLine, vector<string> & outParams )
{
   erase( outParams );
   string line = cmdLine;

   bool inQuotes = false;
   string::size_type a = 0;
   for( string::size_type i = 0; i < cmdLine.size(); i++ )
   {
      if( cmdLine[i] == '\"' )
      {
         inQuotes = !inQuotes;
         continue;
      }
      if( inQuotes )
         continue;

      if( cmdLine[i] == ' ' )
      {
         if( i - a == 0 )
         {
            a = i+1;
            continue;
         }
         else
         {
            outParams.push_back( cmdLine.substr(a, i-a) );
            a = i+1;
         }
      }
   }

   if( a != line.size() )
      outParams.push_back( line.substr(a) );

   for( size_t i = 0; i < outParams.size(); i++ )
   {
      if( outParams[i][0] == '\"' ) outParams[i] = outParams[i].substr(1);
      if( outParams[i][outParams[i].size()-1] == '\"' ) outParams[i] = outParams[i].substr(0, outParams[i].size()-1);
   }
}

/*
void HeightmapTBMPDownsampleToQuarter( const char * tbmpPathSrc, const char * tbmpPathDest )
{
   VertexAsylum::TiledBitmap * heightmap = VertexAsylum::TiledBitmap::Open( tbmpPathSrc, true );
   if( (heightmap == NULL) || (heightmap->PixelFormat() != VertexAsylum::TBPF_Format16BitGrayScale) ) 
   {
      return;
   }

   VertexAsylum::TiledBitmap * heightmapDest = VertexAsylum::TiledBitmap::Create( tbmpPathDest,
      VertexAsylum::TBPF_Format16BitGrayScale, (heightmap->Width()+1)/2, (heightmap->Height()+1)/2 );
   if( heightmapDest == NULL ) 
   {
      VertexAsylum::TiledBitmap::Close( heightmap );
      return;
   }

   vaProgressWindowOpen( L"Downsampling 2x heightmap..." );

   vaProgressWindowUpdate( 0.5f );

   for( int y = 0; y < heightmapDest->Height(); y++ )
   {
      int sy0 = y * 2;
      int sy1 = ::min( heightmap->Height()-1, y * 2 + 1 );
      for( int x = 0; x < heightmapDest->Width(); x++ )
      {
         int sx0 = x * 2;
         int sx1 = ::min( heightmap->Width()-1, x * 2 + 1 );

         uint16 pixel0, pixel1, pixel2, pixel3;
         heightmap->GetPixel( sx0, sy0, &pixel0 );
         heightmap->GetPixel( sx1, sy0, &pixel1 );
         heightmap->GetPixel( sx0, sy1, &pixel2 );
         heightmap->GetPixel( sx1, sy1, &pixel3 );

         uint16 pixel = (uint16)((pixel0 + pixel1 + pixel2 + pixel3 + 2) / 4);
         heightmapDest->SetPixel( x, y, &pixel );
      }
      vaProgressWindowUpdate( y / (float)(heightmapDest->Height()-1) );
   }

   for( int i = 0; i < 20; i++ )
   {
      vaProgressWindowUpdate( 1.0f );
      Sleep( 10 );
   }

   VertexAsylum::TiledBitmap::Close( heightmap );
   VertexAsylum::TiledBitmap::Close( heightmapDest );
   vaProgressWindowClose();
}


void HeightmapTBMPMirrorEnlarge( const char * tbmpPathSrc, const char * tbmpPathDest )
{
   VertexAsylum::TiledBitmap * heightmap = VertexAsylum::TiledBitmap::Open( tbmpPathSrc, true );
   if( (heightmap == NULL) || (heightmap->PixelFormat() != VertexAsylum::TBPF_Format16BitGrayScale) ) 
   {
      return;
   }

   VertexAsylum::TiledBitmap * heightmapDest = VertexAsylum::TiledBitmap::Create( tbmpPathDest,
      VertexAsylum::TBPF_Format16BitGrayScale, heightmap->Width()*2-1, heightmap->Height()*2-1 );
   if( heightmapDest == NULL ) 
   {
      VertexAsylum::TiledBitmap::Close( heightmap );
      return;
   }

   vaProgressWindowOpen( L"Mirror-enlarging heightmap..." );

   vaProgressWindowUpdate( 0.5f );

   for( int y = 0; y < heightmapDest->Height(); y++ )
   {
      //int ys = (y < heightmap->Height())?(heightmap->Height()-y-1):(y-heightmap->Height());
      int ys = (y < heightmap->Height())?(y):(heightmap->Height()*2-y);
   
      assert( ys >= 0 );
      for( int x = 0; x < heightmapDest->Width(); x++ )
      {
         int xs = (x < heightmap->Width())?(x):(heightmap->Width()*2-x);
         assert( xs >= 0 );

         uint16 pixel = 0;
         heightmap->GetPixel( xs, ys, &pixel );
         heightmapDest->SetPixel( x, y, &pixel );
      }
      vaProgressWindowUpdate( y / (float)(heightmapDest->Height()-1) );
   }

   for( int i = 0; i < 20; i++ )
   {
      vaProgressWindowUpdate( 1.0f );
      Sleep( 10 );
   }

   VertexAsylum::TiledBitmap::Close( heightmap );
   VertexAsylum::TiledBitmap::Close( heightmapDest );
   vaProgressWindowClose();
}
*/

bool vaPreInitialize( const char * cmdLine )
{
   vector<string> cmdParams;
   splitParams( cmdLine, cmdParams );

   //HeightmapTBMPMirrorEnlarge("/TestData/califmnts/heightmap.tbmp",
   //                           "/TestData/califmnts_mega/heightmap.tbmp");

   if( cmdParams.size() > 0 && cmdParams[0] == "-n" )
   {
      if( cmdParams.size() != 6 )
      {
         MessageBox( NULL, L"Invalid params for -n command (required are [input.tbmp output.tbmp worldx worldy worldz])", L"Normalmap creator", MB_OK );
         return false;
      }

      string src = cmdParams[1];
      string dst = cmdParams[2];

      float mapSizeX = (float)atof( cmdParams[3].c_str() );
      float mapSizeY = (float)atof( cmdParams[4].c_str() );
      float mapSizeZ = (float)atof( cmdParams[5].c_str() );

      if( (mapSizeX <= 0) || (mapSizeY <= 0) || (mapSizeZ <= 0) )
         goto normalmap_create_error;

      VertexAsylum::TiledBitmap * heightmap = VertexAsylum::TiledBitmap::Open( src.c_str(), true );
      if( heightmap == NULL ) 
         goto normalmap_create_error;

      VertexAsylum::TiledBitmap * normalmap = VertexAsylum::TiledBitmap::Create( dst.c_str(),
         VertexAsylum::TBPF_Format24BitRGB, heightmap->Width()-1, heightmap->Height()-1 );
      if( normalmap == NULL ) 
      {
         VertexAsylum::TiledBitmap::Close( heightmap );
         goto normalmap_create_error;
      }

      vaGenerateNormalMap( heightmap, normalmap, mapSizeX, mapSizeY, mapSizeZ, true );

      vaProgressWindowOpen( L"Finalizing normalmap write" );

      VertexAsylum::TiledBitmap::Close( heightmap );
      vaProgressWindowUpdate( 0.5f );
      VertexAsylum::TiledBitmap::Close( normalmap );

      for( int i = 0; i < 20; i++ )
      {
         vaProgressWindowUpdate( 1.0f );
         Sleep( 10 );
      }
      vaProgressWindowClose();

      return false;

      normalmap_create_error:
         MessageBox( NULL, L"Error while trying to create normal map", L"Normalmap creator", MB_OK );
         return false;
   }

   if( cmdParams.size() > 0 && cmdParams[0] == "-c" )
   {
      if( cmdParams.size() != 3 )
      {
         MessageBox( NULL, L"Invalid params for -c command", L"CDLOD demo", MB_OK );
         return false;
      }

      g_sourceFilePath = cmdParams[1];
      g_destFilePath = cmdParams[2];

      return true;
   }

   if( cmdParams.size() > 0 && cmdParams[0] == "-o" )
   {
      if( cmdParams.size() != 2 )
      {
         MessageBox( NULL, L"Invalid params for -o command", L"CDLOD demo", MB_OK );
         return false;
      }

      g_sourceFilePath = cmdParams[1];

      return true;
   }

   if( cmdParams.size() == 0 || cmdParams[0] == "//" || cmdParams[0] == "\\\\" )
   {
      OPENFILENAME ofn;			ZeroMemory( &ofn, sizeof(ofn) );
      wchar_t sFile[MAX_PATH];	ZeroMemory( &sFile, sizeof(sFile) );
      ofn.lStructSize = sizeof(ofn);
      ofn.lpstrFilter = L"Streamable CDLOD file (.scdlod)\0*.scdlod\0SCDLOD creation ini file (.ini)\0*.ini\0\0";
      ofn.lpstrFile = sFile;
      ofn.nMaxFile = sizeof(sFile);
      ofn.lpstrTitle = L"Select map file or creation ini file";
      ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
      ofn.hwndOwner = DXUTGetHWND();

      if ( GetOpenFileName( &ofn ) )
      {
         g_sourceFilePath = vaStringSimpleNarrow( wstring(ofn.lpstrFile) );
      }
      else
      {
         return false;
      }

      string fileNameNoExt;
      string fileExt;
      vaSplitPath( g_sourceFilePath.c_str(), g_projectDir, fileNameNoExt, fileExt );

      if( fileExt == ".ini" )
      {
         OPENFILENAMEA ofn;			ZeroMemory( &ofn, sizeof(ofn) );
         char sFile[MAX_PATH];	ZeroMemory( &sFile, sizeof(sFile) );
         
         sprintf( sFile, "%s.scdlod", fileNameNoExt.c_str() );

         ofn.lStructSize = sizeof(ofn);
         ofn.lpstrFilter = "Streamable CDLOD file (.scdlod)\0*.scdlod\0\0";
         ofn.lpstrFile = sFile;
         ofn.nMaxFile = sizeof(sFile);
         ofn.lpstrTitle = "Select output file";
         ofn.lpstrInitialDir = g_projectDir.c_str();
         ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_EXPLORER;
         ofn.hwndOwner = DXUTGetHWND();

         if ( GetSaveFileNameA( &ofn ) )
         {
            g_destFilePath = ofn.lpstrFile;
         }
         else
         {
            return false;
         }
      }


      return true;
   }

   MessageBox( NULL, L"Invalid params", L"Normalmap creator", MB_OK );
   return false;
}

bool  vaInitialize()
{
   TCHAR szDirectory[MAX_PATH] = L"";

   if(!::GetCurrentDirectory(sizeof(szDirectory) - 1, szDirectory))
   {
      MessageBox(NULL, L"Error trying to get working directory", L"Error", MB_OK );
      return false;
   }

   srand( (unsigned)time(NULL) );

   g_initialWorkingDir = vaStringSimpleNarrow( wstring(szDirectory) );
   g_initialWorkingDir += "\\";

   string sourceFilePath = g_sourceFilePath;

   string fileNameNoExt;
   string fileExt;
   string streamableStoragePath;
   vaSplitPath( sourceFilePath.c_str(), g_projectDir, fileNameNoExt, fileExt );

   if( g_projectDir == "" )
   {
      g_projectDir = g_initialWorkingDir;
   }

   g_pTerrainStorage = new CDLODStreamingStorage();

   if( fileExt == ".ini" )
   {
      streamableStoragePath = g_destFilePath;

      if( streamableStoragePath == "" )
      {
         MessageBoxA( NULL, "Invalid destination file path", "Error", MB_OK );
         return false;
      }

      void * iniFileBuffer      = NULL;
      int    iniFileBufferSize  = 0;

      vaLoadFileToMemory( sourceFilePath.c_str(), iniFileBuffer, iniFileBufferSize );
      g_iniFileParser.OpenBuffer( (char *)iniFileBuffer, iniFileBufferSize );

      if( !vaDLODStreamingStorageCreateFromIni( g_pTerrainStorage, g_iniFileParser, streamableStoragePath.c_str(),
                                                iniFileBuffer, iniFileBufferSize ) )
      {
         free( iniFileBuffer );
         iniFileBufferSize = 0;
         goto wsInitializeError;
      }
      free( iniFileBuffer );
      iniFileBufferSize = 0;

      delete g_pTerrainStorage;

      return false;
   }
   else if ( fileExt == ".scdlod" )
   {
      if( !g_pTerrainStorage->Open( sourceFilePath.c_str() ) )
      {
         delete g_pTerrainStorage;
         MessageBoxA( NULL, vaStringFormat( "Error trying to open '%s'.", sourceFilePath.c_str() ).c_str(), "Error", MB_OK );
         return false;
      }

      if( g_pTerrainStorage->GetMetadataBufferSize() == 0 )
      {
         delete g_pTerrainStorage;
         MessageBoxA( NULL, vaStringFormat( "Error trying to open '%s'.", sourceFilePath.c_str() ).c_str(), "Error", MB_OK );
         return false;
      }
      g_iniFileParser.OpenBuffer( (char *)g_pTerrainStorage->GetMetadataBuffer(), g_pTerrainStorage->GetMetadataBufferSize() );
   }
   else
   {
      MessageBoxA( NULL, vaStringFormat( "Format of the input file (%s) not recognized.", sourceFilePath.c_str() ).c_str(), "Error", MB_OK );
      return false;

   }

   g_pDemoCamera     = new DemoCamera();
   g_pDemoSky        = new DemoSky();
   g_pDemoRenderer   = new DemoRenderer();

   g_pDemoRenderer->Start( g_pTerrainStorage );
   g_pDemoCamera->Initialize( g_pTerrainStorage, g_pDemoRenderer->GetSettings().MinViewRange, g_pDemoRenderer->GetSettings().MaxViewRange );
   g_pDemoSky->Initialize();

   vaInitializeHUD();

#ifdef USE_REC_O_MATIC
   g_recOMatic = new DxRecOMatic();
#endif

   return true;

wsInitializeError:

   if( g_pTerrainStorage != NULL ) delete g_pTerrainStorage;

   MessageBoxA( NULL, vaStringFormat( "Unable to load the demo map", sourceFilePath.c_str() ).c_str(), "Error", MB_OK );

   return false;
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

   if( g_pTerrainStorage != NULL )
   {
      delete g_pTerrainStorage;
      g_pTerrainStorage = NULL;
   }

#ifdef USE_REC_O_MATIC
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
   case VK_F10:         return kcToggleTexturingMode;
   case 'P':            return kcPause;
   case 'O':            return kcOneFrameStep;
   case VK_SPACE:       return kcToggleDetailmap;

#ifdef USE_REC_O_MATIC
   case 'R':
      {
         if( vaIsKey( kcControlKey ) )
            return kcToggleMovieRec;
         break;
      }
#endif
#ifdef MY_EXTENDED_STUFF
   case 'N':
      {
         if( vaIsKey( kcControlKey ) )
            return kcToggleCameraScriptRec;
         break;
      }
   case 'M':
      {
         if( vaIsKey( kcControlKey ) )
            return kcToggleCameraScriptPlay;
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
   GetWindowRect( g_hwndMain, &dwr );
   ShowCursor( FALSE );
   g_cursorCenter.x = (dwr.left + dwr.right) / 2; 
   g_cursorCenter.y = (dwr.top + dwr.bottom) / 2;
   SetCursorPos( g_cursorCenter.x, g_cursorCenter.y );
   SetCapture( g_hwndMain );
}
//
void ReleaseCursorCapture( )
{
   if( !g_hasCursorCapture ) return;
   g_hasCursorCapture = false;

   SetCursorPos( g_oldCursorPos.x, g_oldCursorPos.y );
   ShowCursor( TRUE );
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

#ifdef USE_REC_O_MATIC
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

#ifdef USE_REC_O_MATIC
   if( vaIsKeyClicked( kcToggleMovieRec ) )
   {
      static float prevCameraInputLerpK = 500.0f;

      if( !g_recOMatic->IsRecording() )
      {
         wstring outPath = SaveFileDialog( L"Bitmap file (.bmp)\0*.*\0\0", L"Select first output bmp file" );
         if( outPath != L"" )
         {
            g_recOMatic->StartRecording( vaStringSimpleNarrow( outPath ).c_str() );
            prevCameraInputLerpK = g_pDemoCamera->GetInputSmoothingLerpK();
            g_pDemoCamera->SetInputSmoothingLerpK(4.5f);
            g_skipOneRenderFrame = true;
            return;
         }
      }
      else
      {
         g_recOMatic->StopRecording();
         g_pDemoCamera->SetInputSmoothingLerpK(prevCameraInputLerpK);
      }
   }
#endif

   if( vaIsKeyClicked( kcPause ) )
      g_bPause = !g_bPause;

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

   if( g_pDemoCamera != NULL )      g_pDemoCamera->Tick( deltaTime );
   if( g_pTerrainStorage != NULL )
   {
      {
         ProfileScope( CDLOD_StreamingTick );
         g_pTerrainStorage->Tick( deltaTime );
      }
      if( g_pTerrainStorage->IsCurrentlyStreaming() )
      {
         // Yield some thread time to streaming threads
         Sleep(20);
      }
   }
   if( g_pDemoSky != NULL )         g_pDemoSky->Tick( deltaTime );
   if( g_pDemoRenderer != NULL )    g_pDemoRenderer->Tick( g_pDemoCamera, deltaTime );

   DrawInfoText();

   for( int i = 0; i < kcLASTVALUE; i++ )
   {
      g_KeyUps[i]		= false;
      g_KeyDowns[i]	= false;
   }

   vaUpdateHUD();
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
      HRESULT hr;

      if( g_pDemoSky != NULL )      V( g_pDemoSky->Render( g_pDemoCamera ) );
      if( g_pDemoRenderer != NULL ) V( g_pDemoRenderer->Render( g_pDemoCamera, g_pDemoSky, deltaTime ) );

      if( g_pDemoCamera != NULL )   vaDrawCanvas3D( g_pDemoCamera );

      if( g_bHelpText )
      {
         g_DemoSettingsHUD.OnRender( deltaTime );
      }

#ifdef USE_REC_O_MATIC
      if( g_recOMatic->IsRecording() )
      {
         g_recOMatic->GrabBackbuffer();
         g_recOMatic->Render();

         const float framerateLimit = g_recOMatic->GetFrameDeltaTime() - 0.005f;
         static double lastFrameTime = 0;
         double currentTime = DXUTGetGlobalTimer()->GetAbsoluteTime();
         double waitUntilTime = currentTime + framerateLimit;
         while( (currentTime = DXUTGetGlobalTimer()->GetAbsoluteTime()) < waitUntilTime )
         {
            ::Sleep(0);
         }
         lastFrameTime = currentTime;
      }
#endif
   }

#ifdef USE_SIMPLE_PROFILER
   SimpleProfiler::Instance().TickFrame();
   SimpleProfiler::Instance().Render( g_backBufferSrufaceDesc.Width - 390, g_backBufferSrufaceDesc.Height - 110 );
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
      outStr += vaStringFormat(L"%d: %03d, ", i, intArr[i]);
   }
   outStr += vaStringFormat(L"%d: %03d", count-1, intArr[count-1]);
   
   return outStr;
}

void DrawInfoText()
{
   int lineHeight = 12;
   int lineYPos = 26;

   const int lineXPos = 6;

   uint32 penColor = 0xFFFFFF00;
   uint32 shadowColor = 0x80000000;

   if( g_bPause )                      GetCanvas2D()->DrawString( g_backBufferSrufaceDesc.Width/2 - 44, lineYPos, penColor, shadowColor, L"PAUSED" );
   lineYPos += (int)(lineHeight * 2.0);

   if( !g_bHelpText )
   {
      lineYPos += lineHeight;
   }
   else
   {
      lineYPos += lineHeight;
      lineYPos += 6;

      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"Visibility range: %.1f", g_pDemoCamera->GetViewRange() );
      lineYPos += lineHeight+1;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"Camera: speed %.1f, yaw %.2f, pitch %.2f,", g_pDemoCamera->GetSpeed(), g_pDemoCamera->GetYaw() / (float)PI * 180, g_pDemoCamera->GetPitch() / (float)PI * 180 );
      lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"             pos %.1f, %.1f, %.1f", g_pDemoCamera->GetPosition().x, g_pDemoCamera->GetPosition().y, g_pDemoCamera->GetPosition().z );
      lineYPos += lineHeight;
      lineYPos += 6;

      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"commands:" );   lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"  W,S,A,D,Q,Z: camera movement" ); lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"  Shift/Ctrl: boost/slow camera movement speed"); lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"  Right mouse btn: toggle mouse mode"); lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"  Mouse wheel: change camera speed"); lineYPos += lineHeight;

      const DemoRenderer::LastFrameRenderStats & renderStats = g_pDemoRenderer->GetRenderStats();
      
      const CDLODStreamingStorage::MemoryUseInfo & memUseInfo = g_pTerrainStorage->GetCurrentMemoryUseInfo();
      
      lineYPos = g_backBufferSrufaceDesc.Height - 11 * lineHeight - 4;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"Rendering statistics" );   lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"  LOD level count: %d", renderStats.LODLevelCount ); lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"  Quads rendered per level: (%s)", FormatArrayToString(renderStats.TerrainStats.RenderedQuads, renderStats.LODLevelCount).c_str() ); lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"  Quads rendered total: %d, triangles: %d", renderStats.TerrainStats.TotalRenderedQuads, renderStats.TerrainStats.TotalRenderedTriangles ); lineYPos += lineHeight;
      lineYPos += lineHeight / 2;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"Memory use" );   lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"  Quad tree storage: %dKb", memUseInfo.QuadtreeMemory / 1024 ); lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"  Streaming system: %dKb", memUseInfo.StreamingThreadsMemory / 1024 ); lineYPos += lineHeight;

      wchar_t * vertexFmtName = L"D3DFMT_UNKNOWN";
      if( g_VertexTextureFormat == D3DFMT_L16 ) vertexFmtName = L"D3DFMT_L16";
      if( g_VertexTextureFormat == D3DFMT_R32F ) vertexFmtName = L"D3DFMT_R32F";

      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"  Heightmap blocks:  %d, memory %dKb, cache memory %dKb (using %s)", memUseInfo.HeightmapLoadedBlocks, memUseInfo.HeightmapTextureMemory/1024, memUseInfo.HeightmapCacheMemory/1024, vertexFmtName ); lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"  Normalmap blocks:  %d, memory %dKb, cache memory %dKb", memUseInfo.NormalmapLoadedBlocks, memUseInfo.NormalmapTextureMemory/1024, memUseInfo.NormalmapCacheMemory/1024 ); lineYPos += lineHeight;
      GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor, L"  Overlaymap blocks: %d, memory %dKb, cache memory %dKb", memUseInfo.OverlaymapLoadedBlocks, memUseInfo.OverlaymapTextureMemory/1024, memUseInfo.OverlaymapCacheMemory/1024 ); lineYPos += lineHeight;
      //GetCanvas2D()->DrawString( lineXPos, lineYPos, penColor, shadowColor L"    quad tree storage: 

   }

}

bool vaGetBilinearVertexTextureFilterSupport()
{
   return g_BilinearVTFilter;
}

D3DFORMAT vaGetVertexTextureFormat()
{
   return g_VertexTextureFormat;
}

bool vaIsD3D9DeviceAcceptable( D3DCAPS9* pCaps, UINT AdapterOrdinal, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
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

   if( pCaps->TextureCaps & D3DPTEXTURECAPS_SQUAREONLY )
      return false;

   if( pCaps->TextureCaps & D3DPTEXTURECAPS_POW2 )
      return false;

   if( pCaps->TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL )
      return false;

   if( (pCaps->Caps2 & D3DCAPS2_DYNAMICTEXTURES) == 0 )
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

   if( SUCCEEDED( DXUTGetD3D9Object()->CheckDeviceFormat(devCaps.AdapterOrdinal, devCaps.DeviceType, displayMode.Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, D3DFMT_D24S8 ) ) )
   {
      g_shadowMapSupport = smsNVidiaShadows;
   }
   else if( SUCCEEDED( pD3D9->CheckDeviceFormat( cp.AdapterOrdinal, cp.DeviceType, pBackBufferSurfaceDesc->Format, 
      D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, (D3DFORMAT)(MAKEFOURCC('D','F','2','4')) ) ) )
   {
      g_shadowMapSupport = smsATIShadows;
   }
   else
   {
      g_shadowMapSupport = smsNotSupported;
   }

   SAFE_RELEASE( pD3D9 );

   if( SUCCEEDED( DXUTGetD3D9Object()->CheckDeviceFormat( devCaps.AdapterOrdinal, devCaps.DeviceType, displayMode.Format, D3DUSAGE_QUERY_VERTEXTEXTURE, D3DRTYPE_TEXTURE, D3DFMT_L16 ) ) )
   {
      g_VertexTextureFormat = D3DFMT_L16;
   }
   else if( SUCCEEDED( DXUTGetD3D9Object()->CheckDeviceFormat( devCaps.AdapterOrdinal, devCaps.DeviceType, displayMode.Format, D3DUSAGE_QUERY_VERTEXTEXTURE, D3DRTYPE_TEXTURE, D3DFMT_R32F ) ) )
   {
      g_VertexTextureFormat = D3DFMT_R32F;
   }
   else
   {
      assert( false );
   }

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
   case dhid_ShowDeferredBuffer:
      {
         g_pDemoRenderer->SetShowDeferredBufferType( (int)g_HUDShowDeferredBuffer->GetSelectedIndex() );
      }
      break;
   case dhid_TextureMode:
      {
         g_pDemoRenderer->SetTexturingMode( (DemoRenderer::TexturingMode)g_HUDTexturingMode->GetSelectedIndex() );
      }
      break;
   case dhid_LightingToggle:
      g_pDemoRenderer->SetLightingEnabled( g_HUDLightingEnabled->GetChecked() );
      vaUpdateHUD();
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

   const int iX = 10;
   int lineHeight = 20;
   const int lineWidth = 173;
   const int bigLineHeight = lineHeight + 12;

   g_DemoSettingsHUD.AddCheckBox( dhid_Wireframe, L"Wireframe (F6)", iX, iY, lineWidth, 22, false, 0, false, &g_HUDWireframe );
   iY += bigLineHeight;
   //g_DemoSettingsHUD.AddStatic( -1, L"Cascaded shadow maps (F7):", iX, iY, lineWidth, 22 );
   //iY += lineHeight;
   //g_DemoSettingsHUD.AddComboBox( dhid_ShadowQuality, iX, iY, lineWidth, 22, 0, false, &g_HUDShadowQuality );
   //iY += bigLineHeight;
   //
   //if( vaGetShadowMapSupport() != smsNotSupported )
   //{
   //   g_HUDShadowQuality->AddItem( L"Disabled", NULL );
   //   g_HUDShadowQuality->AddItem( L"Low quality", NULL );
   //   g_HUDShadowQuality->AddItem( L"High quality", NULL );
   //}
   //else
   //{
   //   g_HUDShadowQuality->AddItem( L"Disabled (not supported)", NULL );
   //   g_HUDShadowQuality->SetEnabled( false );
   //}

   g_DemoSettingsHUD.AddCheckBox( dhid_Detailmap, L"Detail map (F8)", iX, iY, lineWidth, 22, false, 0, false, &g_HUDDetailmap );
   if( !g_pDemoRenderer->GetDetailmapExists() ) g_HUDDetailmap->SetEnabled( false );
   iY += bigLineHeight;

   g_DemoSettingsHUD.AddCheckBox( dhid_DebugView, L"Debug view (F9)", iX, iY, lineWidth, 22, false, 0, false, &g_HUDDebugView );
   iY += bigLineHeight;

   // Texturing
   g_DemoSettingsHUD.AddStatic( -1, L"Texturing:", iX, iY, lineWidth, 22 );
   iY += lineHeight;
   g_DemoSettingsHUD.AddComboBox( dhid_TextureMode, iX, iY, lineWidth, 22, 0, false, &g_HUDTexturingMode );
   iY += bigLineHeight;
   g_HUDTexturingMode->AddItem( L"Simple", NULL );
   if( g_pDemoRenderer->GetSettings().SplattingEnabled )
      g_HUDTexturingMode->AddItem( L"Texture splatting", NULL );
   else
      g_HUDTexturingMode->AddItem( L"Overlay image", NULL );

   // View range
   float minValue = g_pDemoRenderer->GetSettings().MinViewRange;
   float maxValue = g_pDemoRenderer->GetSettings().MaxViewRange;
   int initialValue = (int)(((g_pDemoCamera->GetViewRange() - minValue) / (maxValue - minValue)) * 1000.0f+0.5);
   g_DemoSettingsHUD.AddStatic( -1, L"View range & LOD detail level:", iX, iY, lineWidth, 22 );
   iY += lineHeight;
   g_DemoSettingsHUD.AddSlider( dhid_CameraRange, iX, iY, lineWidth, 22, 0, 1000, initialValue, false, &g_HUDCameraRange );
   iY += bigLineHeight;

   g_DemoSettingsHUD.AddStatic( -1, L"Render grid size:", iX, iY, lineWidth, 22 );
   iY += lineHeight;
   g_DemoSettingsHUD.AddComboBox( dhid_RenderGridSize, iX, iY, lineWidth, 22, 0, false, &g_HUDRenderGridSize );
   iY += bigLineHeight;

   int leafSize = g_pTerrainStorage->GetWorldDesc().LeafQuadTreeNodeSize;
   int maxGridMult = g_pDemoRenderer->GetMaxRenderGridResolutionMult();
   for( int i = 0, m = 1; m <= maxGridMult; i++, m *= 2 )
   {
      int gridSize = leafSize * m;
      g_HUDRenderGridSize->AddItem( vaStringFormat(L"%d x %d (%d mult)", gridSize, gridSize, m ).c_str(), (void*)m );
      if( g_pTerrainStorage->GetWorldDesc().RenderGridResolutionMult == m )
         g_HUDRenderGridSize->SetSelectedByIndex( i );
   }

   iY += 6;
   g_DemoSettingsHUD.AddCheckBox( dhid_LightingToggle, L"Lighting", iX, iY, lineWidth, 22, false, 0, false, &g_HUDLightingEnabled );
   iY += bigLineHeight;
   g_HUDLightingEnabled->SetEnabled( g_pDemoRenderer->GetLightingSupported() );
   iY -= 6;

   g_DemoSettingsHUD.AddStatic( -1, L"Sun azimuth/elevation:", iX, iY, lineWidth, 22 );
   iY += lineHeight;
   g_DemoSettingsHUD.AddSlider( dhid_SunHorizontal, iX+1, iY, lineWidth/2, 22, 0, 1000, initialValue, false, &g_HUDSunHorizontal );
   //iY += lineHeight;
   g_DemoSettingsHUD.AddSlider( dhid_SunVertical, iX+lineWidth/2 + 23, iY, lineWidth/2-24, 22, 0, 1000, initialValue, false, &g_HUDSunVertical );
   iY += bigLineHeight;

   float sunAzimuth, sunElevation;
   g_pDemoSky->GetSunPosition( sunAzimuth, sunElevation );
   g_HUDSunHorizontal->SetValue( (int)(0.5f + sunAzimuth / (2.0f * (float)PI) * 1000.0f ) );
   g_HUDSunVertical->SetValue( (int)(0.5f + (sunElevation-0.06f) / (0.45f * (float)PI) * 1000.0f ) );
   vaOnGUIEvent( 0, dhid_SunHorizontal, g_HUDSunHorizontal, NULL );

   g_DemoSettingsHUD.SetSize( lineWidth + iX * 2, iY );

   vaUpdateHUD();
}

void vaUpdateHUD()
{
   if( g_pDemoRenderer == NULL )
      return;

   if( (g_HUDWireframe != NULL) && (g_HUDWireframe->GetChecked() != g_pDemoRenderer->GetWireframe()) )
      g_HUDWireframe->SetChecked( g_pDemoRenderer->GetWireframe() );

   if( ( g_HUDDetailmap != NULL ) && (g_HUDDetailmap->GetChecked() != g_pDemoRenderer->GetSettings().DetailHeightmapEnabled) )
         g_HUDDetailmap->SetChecked( g_pDemoRenderer->GetSettings().DetailHeightmapEnabled );

   if( ( g_HUDDebugView != NULL ) && ( g_HUDDebugView->GetChecked() != g_pDemoRenderer->GetDebugView() ) )
      g_HUDDebugView->SetChecked( g_pDemoRenderer->GetDebugView() );

   if( ( g_HUDLightingEnabled != NULL ) && ( g_HUDLightingEnabled->GetChecked() != g_pDemoRenderer->GetLightingEnabled() ) )
      g_HUDLightingEnabled->SetChecked( g_pDemoRenderer->GetLightingEnabled() );

   if( (g_HUDShowDeferredBuffer != NULL ) && (g_HUDShowDeferredBuffer->GetSelectedIndex() != g_pDemoRenderer->GetShowDeferredBufferType() ) )
      g_HUDShowDeferredBuffer->SetSelectedByIndex( g_pDemoRenderer->GetShowDeferredBufferType() );

   if( (g_HUDTexturingMode != NULL ) && (g_HUDTexturingMode->GetSelectedIndex() != g_pDemoRenderer->GetTexturingMode() ) )
      g_HUDTexturingMode->SetSelectedByIndex( g_pDemoRenderer->GetTexturingMode() );

   if( ( g_HUDSunHorizontal != NULL ) && (g_HUDSunHorizontal->GetEnabled() != g_HUDLightingEnabled->GetChecked()) )
      g_HUDSunHorizontal->SetEnabled( g_HUDLightingEnabled->GetChecked() );

   if( (g_HUDSunVertical != NULL ) && ( g_HUDSunVertical->GetEnabled() != g_HUDLightingEnabled->GetChecked() ) )
      g_HUDSunVertical->SetEnabled( g_HUDLightingEnabled->GetChecked() );
}