//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "ProgressDlg.h"

#include "Resources\resource.h"

#pragma warning ( disable: 4127 )   //warning C4127: conditional expression is constant

static HWND g_hProgressDialogWin = NULL;
static HWND g_hProgressDialogControl = NULL;

void DoEvents( HWND onlyToThis = NULL )
{
	MSG msg; 
	while(1) 
	{ 
		if ( ::PeekMessage(&msg, onlyToThis, 0, 0, PM_REMOVE) ) 
		{ 
			::TranslateMessage(&msg); 
			// process message 
			::DispatchMessage(&msg); 
		} 
		else 
		{
			//if there are no more window messages bailout
			break;
		}
	} 
} 

void vaProgressWindowUpdate(float progress)
{
   assert( g_hProgressDialogWin != NULL );
   if( g_hProgressDialogWin == NULL )
      return;

	SendMessage(g_hProgressDialogControl, PBM_SETPOS, (int)(progress * 1000.0f), 0);
	DoEvents( NULL ); //DoEvents(g_hProgressDialogWin);
}

HWND vaProgressWindowGetHWND()
{
   return g_hProgressDialogWin;
}

bool vaProgressWindowOpen( const wchar_t * strTitle )
{
   if( g_hProgressDialogWin != NULL )
      return false;

   g_hProgressDialogWin = ::CreateDialog( NULL, MAKEINTRESOURCE(IDD_LOADPROGRESS), DXUTGetHWND(), NULL );
   assert( g_hProgressDialogWin != NULL );

   RECT rect;
   ::GetWindowRect( g_hProgressDialogWin, &rect );
   SIZE size;
   size.cx = rect.right - rect.left;
   size.cy = rect.bottom - rect.top;
   ::GetWindowRect( ::GetDesktopWindow(), &rect );
   ::SetWindowPos( g_hProgressDialogWin,  HWND_TOP,	(rect.right - rect.left) / 2 - size.cx / 2,
      (rect.bottom - rect.top) / 2 - size.cy,
      0, 0, SWP_NOSIZE | SWP_NOZORDER );
   ::SetWindowText( g_hProgressDialogWin, strTitle );

   g_hProgressDialogControl = ::GetDlgItem(g_hProgressDialogWin, IDC_PROGRESS);
   SendMessage(g_hProgressDialogControl, PBM_SETRANGE, 0, MAKELPARAM(0, 1000));

   ::ShowWindow( g_hProgressDialogWin, SW_SHOW );

   vaProgressWindowUpdate( 0.0f );

   return true;
}

bool vaProgressWindowClose()
{
   if( g_hProgressDialogWin == NULL )
      return false;

   assert( g_hProgressDialogWin != NULL );


   for( int i = 0; i < 10; i++ )
   {
      vaProgressWindowUpdate(1.0f);
      ::Sleep(10);
   }

   DoEvents( NULL ); //g_hProgressDialogWin );

   ::DestroyWindow(g_hProgressDialogWin);
   DoEvents( );

   g_hProgressDialogWin = NULL;
   g_hProgressDialogControl = NULL;

   return true;
}

