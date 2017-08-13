#pragma once

#include "..\DxEventNotifier.h"

#include "Avi.h"

class DxRecOMatic : DxEventReceiver
{
   bool                       isRecording;
   std::string                path;
   float                      frameDeltaTime;

   IDirect3DSurface9 *        screenGrabSourceSurface;
   IDirect3DSurface9 *        screenGrabLockableSurface;

   Avi *                      avi;
   HBITMAP                    screenBitmap;
   void *                     screenBitmapData;
   int                        screenBitmapScanLineSize;
   HDC                        captureDC;

   CRITICAL_SECTION           CS;

   HANDLE                     workerThread;
   bool                       workerThreadHasJob;
   bool                       shutdownWorker;

public:
   DxRecOMatic(void);
   ~DxRecOMatic(void);

   bool                       StartRecording( const char * path, int frameRate = 30 );
   void                       StopRecording( );

   bool                       IsRecording( )                                                    { return isRecording; }
   float                      GetFrameDeltaTime( )                                              { return frameDeltaTime; }

   void                       GrabBackbuffer( );

   void                       Render( );

private:

   virtual HRESULT 				OnCreateDevice();
   virtual HRESULT            OnResetDevice(const D3DSURFACE_DESC* pBackBufferSurfaceDesc);
   virtual void               OnLostDevice();
   virtual void               OnDestroyDevice();

private:
   friend DWORD WINAPI        ThreadProcProxy( LPVOID lpThreadParameter );

   unsigned int               WriterThreadProc();
};
