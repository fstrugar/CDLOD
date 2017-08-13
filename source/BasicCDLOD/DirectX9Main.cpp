//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "Resources\resource.h"

#include "DemoMain.h"
#include "DxEventNotifier.h"

#include "DemoCamera.h"

#ifdef MY_EXTENDED_STUFF
#include "IProf/prof.h"
#endif

using namespace std;

#pragma warning ( disable: 4996 )

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager    g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg               g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*              g_pTxtHelper = NULL;
CDXUTDialog                   g_HUD;                  // manages the 3D UI
CDXUTDialog                   g_ShowHUD;              // manages one lonely button
CDXUTButton *                 g_ShowHUDButton = NULL; // lonely button

extern string                 g_cmdLine;
extern wstring                g_sWindowTitle;
extern string                 g_sRevision;

extern bool                   g_deviceCreated;
extern bool                   g_deviceReset;
extern D3DSURFACE_DESC        g_backBufferSrufaceDesc;
extern HWND                   g_hwndMain;

extern bool                   g_bHelpText;

// Direct3D 9 resources
ID3DXFont*                    g_pFont = NULL;
ID3DXSprite*                  g_pTextSprite = NULL;
LPDIRECT3DVERTEXBUFFER9       g_pVB = NULL;
LPDIRECT3DINDEXBUFFER9        g_pIB = NULL;
LPDIRECT3DVERTEXSHADER9       g_pVertexShader = NULL;
LPD3DXCONSTANTTABLE           g_pConstantTable = NULL;
LPDIRECT3DVERTEXDECLARATION9  g_pVertexDeclaration = NULL;

IDirect3DSurface9 *           g_depthStencil = NULL;


//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           3
#define IDC_CHANGEDEVICE        4

#define IDC_TOGGLEHUD           5


//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, UINT AdapterOrdinal, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat,
                                      bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice,
                                     const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                           void* pUserContext );
void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime,
                                        void* pUserContext );
void CALLBACK OnD3D9LostDevice( void* pUserContext );
void CALLBACK OnD3D9DestroyDevice( void* pUserContext );


bool CALLBACK     ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK     OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK  MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK     OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );

void              InitApp();
void              RenderText();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
   // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
   _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

   g_cmdLine = vaStringSimpleNarrow( wstring(lpCmdLine) );

   // Set DXUT callbacks
   DXUTSetCallbackMsgProc( MsgProc );
   DXUTSetCallbackFrameMove( OnFrameMove );

   DXUTSetCallbackD3D9DeviceAcceptable( IsD3D9DeviceAcceptable );
   DXUTSetCallbackD3D9DeviceCreated( OnD3D9CreateDevice );
   DXUTSetCallbackD3D9DeviceReset( OnD3D9ResetDevice );
   DXUTSetCallbackD3D9FrameRender( OnD3D9FrameRender );
   DXUTSetCallbackD3D9DeviceLost( OnD3D9LostDevice );
   DXUTSetCallbackD3D9DeviceDestroyed( OnD3D9DestroyDevice );

   DXUTSetD3DVersionSupport( true, false );

   DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

   InitApp();
   DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
   DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen

   string rev = g_sRevision;
   if( rev.size() > 3 )
      rev = rev.substr(2, rev.size()-3);
   wstring winTitle = g_sWindowTitle + L" (r" + vaStringSimpleWiden( rev ) + L")";

   DXUTCreateWindow( winTitle.c_str() );

   //if( FAILED( DXUTCreateDevice( true, 720, 480 ) ) )
   if( FAILED( DXUTCreateDevice( true, 1024, 768 ) ) )
   {
      return DXUTGetExitCode();
   }

   if( !vaInitialize() )
   {
      return DXUTGetExitCode();
   }

   DXUTMainLoop(); // Enter into the DXUT render loop

   vaDeinitialize();

   DXUTShutdown();

   return DXUTGetExitCode();
}

void UpdateShowHUDButton()
{
   g_ShowHUDButton->SetText( (g_bHelpText)?(L"Hide HUD (F1)"):(L"Show HUD (F1)") );
}

//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
   g_SettingsDlg.Init( &g_DialogResourceManager );
   g_HUD.Init( &g_DialogResourceManager );

   g_HUD.SetCallback( OnGUIEvent ); int iY = 10;
   g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22 );
   g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22, VK_F3 );
   g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125, 22, VK_F2 );

   g_ShowHUD.Init( &g_DialogResourceManager );
   CDXUTElement element = *g_ShowHUD.GetDefaultElement( DXUT_CONTROL_BUTTON, 0 );
   element.FontColor.States[ DXUT_STATE_MOUSEOVER ] = D3DCOLOR_ARGB( 255, 0, 0, 0 );
   element.FontColor.States[ DXUT_STATE_NORMAL ] = D3DCOLOR_ARGB( 255, 255, 255, 0 );
   element.FontColor.States[ DXUT_STATE_FOCUS ] = D3DCOLOR_ARGB( 255, 255, 255, 0 );
   element.FontColor.States[ DXUT_STATE_MOUSEOVER ] = D3DCOLOR_ARGB( 255, 0, 0, 0 );
   element.FontColor.States[ DXUT_STATE_PRESSED ] = D3DCOLOR_ARGB( 255, 255, 255, 0 );
   g_ShowHUD.SetDefaultElement( DXUT_CONTROL_BUTTON, 101, &element );
   g_ShowHUD.SetCallback( OnGUIEvent );
   g_ShowHUD.AddButton( IDC_TOGGLEHUD, L"Show HUD (F1)", 0, 0, 128, 22, VK_F1, false, &g_ShowHUDButton );
   g_ShowHUDButton->SetElementIndex(101);
   UpdateShowHUDButton();
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    if( pDeviceSettings->ver == DXUT_D3D9_DEVICE )
    {
        IDirect3D9* pD3D = DXUTGetD3D9Object();
        D3DCAPS9 Caps;
        pD3D->GetDeviceCaps( pDeviceSettings->d3d9.AdapterOrdinal, pDeviceSettings->d3d9.DeviceType, &Caps );

#ifdef MY_EXTENDED_STUFF
        // needed for RecOMatic
        pDeviceSettings->d3d9.BehaviorFlags |= D3DCREATE_MULTITHREADED;
#endif

        // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
        // then switch to SWVP.
        if( ( Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) == 0 ||
            Caps.VertexShaderVersion < D3DVS_VERSION( 2, 0 ) )
        {
            pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }

        // Debugging vertex shaders requires either REF or software vertex processing 
        // and debugging pixel shaders requires REF.  
#ifdef DEBUG_VS
        if( pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF )
        {
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;
            pDeviceSettings->d3d9.BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }
#endif
#ifdef DEBUG_PS
        pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
#endif
    }

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF ) ||
            ( DXUT_D3D10_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE ) )
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );

        pDeviceSettings->d3d9.pp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
   assert( DXUTIsAppRenderingWithD3D9() );

   fElapsedTime = clamp( fElapsedTime, 0.0f, 0.05f );

   vaTick( (float)fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText()
{
    // Output statistics
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 5, 5 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{

    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    if( g_bHelpText )
    {
      // Give the dialogs a chance to handle the message first
      *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
      if( *pbNoFurtherProcessing )
        return 0;
    }

    *pbNoFurtherProcessing = g_ShowHUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
       return 0;

    vaMsgProc( hWnd, uMsg, wParam, lParam, pbNoFurtherProcessing );
    if( *pbNoFurtherProcessing )
       return 0;

    return 0;
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:
            DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:
            g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() ); break;
            break;
        case IDC_TOGGLEHUD:
            g_bHelpText = !g_bHelpText;
            UpdateShowHUDButton();
            break;
    }
}

extern void vaDrawCanvas2D();

//--------------------------------------------------------------------------------------
// Rejects any D3D9 devices that aren't acceptable to the app by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, UINT AdapterOrdinal, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
                                     D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
   if( pCaps->DeviceType != D3DDEVTYPE_HAL )
      return false;

   // Skip backbuffer formats that don't support alpha blending
   IDirect3D9* pD3D = DXUTGetD3D9Object();
   if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
      AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
      D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
   {
      return false;
   }

   if( !vaIsD3D9DeviceAcceptable( pCaps, AdapterOrdinal, DeviceType, AdapterFormat, BackBufferFormat, bWindowed, pUserContext ) )
      return false;

   //if( FAILED( pD3D->CheckDevicevaFormatString( pCaps->AdapterOrdinal, pCaps->DeviceType,
   //   D3DFMT_R32F, D3DUSAGE_RENDERTARGET | D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
   //   D3DRTYPE_SURFACE, BackBufferFormat ) ) )
   //{
   //   return false;
   //}

   return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that will live through a device reset (D3DPOOL_MANAGED)
// and aren't tied to the back buffer size
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                    void* pUserContext )
{
   HRESULT hr;

   vaOnCreateDevice( pd3dDevice, pBackBufferSurfaceDesc );

   g_deviceCreated = true;

   g_hwndMain = DXUTGetHWND();

   V_RETURN( DxEventNotifier::PostCreateDevice(pd3dDevice) );

   V_RETURN( g_DialogResourceManager.OnD3D9CreateDevice( pd3dDevice ) );
   V_RETURN( g_SettingsDlg.OnD3D9CreateDevice( pd3dDevice ) );
   V_RETURN( D3DXCreateFont( pd3dDevice, 13, 0, FW_SEMIBOLD, 1, FALSE, DEFAULT_CHARSET,
      OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
      L"Verdana", &g_pFont ) );

   return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that won't live through a device reset (D3DPOOL_DEFAULT) 
// or that are tied to the back buffer size 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                   void* pUserContext )
{
   HRESULT hr;

   g_deviceReset = true;
   g_backBufferSrufaceDesc = *pBackBufferSurfaceDesc;

   g_hwndMain = DXUTGetHWND();

   V_RETURN( pd3dDevice->CreateDepthStencilSurface( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height, D3DFMT_D24S8, pBackBufferSurfaceDesc->MultiSampleType, pBackBufferSurfaceDesc->MultiSampleQuality, false, &g_depthStencil, NULL ) );

   V_RETURN( DxEventNotifier::PostResetDevice(pBackBufferSurfaceDesc) );

   V_RETURN( g_DialogResourceManager.OnD3D9ResetDevice() );
   V_RETURN( g_SettingsDlg.OnD3D9ResetDevice() );

   if( g_pFont )
      V_RETURN( g_pFont->OnResetDevice() );

   // Create a sprite to help batch calls when drawing many lines of text
   V_RETURN( D3DXCreateSprite( pd3dDevice, &g_pTextSprite ) );
   g_pTxtHelper = new CDXUTTextHelper( g_pFont, g_pTextSprite, NULL, NULL, 15 );

   pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
   pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

   g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
   g_HUD.SetSize( 170, 170 );

   g_ShowHUD.SetLocation( 1, 42 );
   g_ShowHUD.SetSize( 128, 22 );

   V_RETURN( vaOnResetDevice( pd3dDevice, pBackBufferSurfaceDesc ) );

   return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D9 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
   fElapsedTime = clamp( fElapsedTime, 0.0f, 0.5f );

   // If the settings dialog is being shown, then render it instead of rendering the app's scene
   if( g_SettingsDlg.IsActive() )
   {
      g_SettingsDlg.OnRender( fElapsedTime );
      return;
   }

   HRESULT hr;


   IDirect3DSurface9 * pOldDepthStencil = NULL;
   pd3dDevice->GetDepthStencilSurface( &pOldDepthStencil );

   V( pd3dDevice->SetDepthStencilSurface( g_depthStencil ) );
   V( pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE ) );
   V( pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE ) );

   // Clear the render target and the zbuffer 
   V( pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB( 0, 64, 128, 192 ), 1.0f, 0 ) ); //D3DCOLOR_ARGB( 0, 45, 50, 170 )

   // Render the scene
   if( SUCCEEDED( pd3dDevice->BeginScene() ) )
   {
      V( vaRender( fElapsedTime ) );

      RenderText();
      vaDrawCanvas2D();

      if( g_bHelpText )
      {
         V( g_HUD.OnRender( fElapsedTime ) );
      }
      V( g_ShowHUD.OnRender( fElapsedTime ) );

      V( pd3dDevice->EndScene() );
   }

   if( pOldDepthStencil )
   {
      pd3dDevice->SetDepthStencilSurface( pOldDepthStencil );
      SAFE_RELEASE( pOldDepthStencil );
   }
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9ResetDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9LostDevice( void* pUserContext )
{
   g_deviceReset = false;

   DxEventNotifier::PostLostDevice();

   g_DialogResourceManager.OnD3D9LostDevice();
   g_SettingsDlg.OnD3D9LostDevice();
   if( g_pFont )
      g_pFont->OnLostDevice();

   SAFE_RELEASE( g_pIB );
   SAFE_RELEASE( g_pVB );
   SAFE_RELEASE( g_pTextSprite );
   SAFE_DELETE( g_pTxtHelper );
   SAFE_RELEASE( g_depthStencil );
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9CreateDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9DestroyDevice( void* pUserContext )
{
   g_deviceCreated = false;

   DxEventNotifier::PostDestroyDevice();

   g_DialogResourceManager.OnD3D9DestroyDevice();
   g_SettingsDlg.OnD3D9DestroyDevice();
   SAFE_RELEASE( g_pFont );
   SAFE_RELEASE( g_pVertexShader );
   SAFE_RELEASE( g_pConstantTable );
   SAFE_RELEASE( g_pVertexDeclaration );
}

