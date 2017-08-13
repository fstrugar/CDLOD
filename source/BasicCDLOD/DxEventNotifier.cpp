//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "DxEventNotifier.h"

#pragma warning (default : 4995)

using namespace std;

DxEventNotifier & DxEventNotifier::Instance()
{
   static DxEventNotifier me;
   return me;
}

DxEventNotifier::DxEventNotifier(void)
{
	d3ddcnt_pd3dDevice = NULL;
	d3ddcnt_deviceLost = true;
}
//
DxEventNotifier::~DxEventNotifier(void)
{
   assert( DXNotifyTargets.size() == 0 );
}
//
HRESULT DxEventNotifier::PostCreateDevice(IDirect3DDevice9* pd3dDevice)
{
   DxEventNotifier::Instance().d3ddcnt_pd3dDevice = pd3dDevice;
	DxEventNotifier::Instance().d3ddcnt_deviceLost = false;
	HRESULT hr;
	for( list<DxEventReceiver*>::iterator it = DxEventNotifier::Instance().DXNotifyTargets.begin(); it != DxEventNotifier::Instance().DXNotifyTargets.end(); it++ )
		V_RETURN( (*it)->OnCreateDevice() );
	return S_OK;
}
//
HRESULT	DxEventNotifier::PostResetDevice(const D3DSURFACE_DESC* pBackBufferSurfaceDesc)
{
	DxEventNotifier::Instance().d3ddcnt_deviceLost = false;
   DxEventNotifier::Instance().backBufferSurfaceDesc = *pBackBufferSurfaceDesc;
	HRESULT hr;
	for( list<DxEventReceiver*>::iterator it = DxEventNotifier::Instance().DXNotifyTargets.begin(); it != DxEventNotifier::Instance().DXNotifyTargets.end(); it++ )
		V_RETURN( (*it)->OnResetDevice(pBackBufferSurfaceDesc) );
	return S_OK;
}
//
void DxEventNotifier::PostLostDevice()
{
	if( DxEventNotifier::Instance().d3ddcnt_deviceLost ) return;
	for( list<DxEventReceiver*>::reverse_iterator  it = DxEventNotifier::Instance().DXNotifyTargets.rbegin(); it != DxEventNotifier::Instance().DXNotifyTargets.rend(); it++ )
		(*it)->OnLostDevice();
	DxEventNotifier::Instance().d3ddcnt_deviceLost = true;
}
//
void DxEventNotifier::PostDestroyDevice()
{
	DxEventNotifier::Instance().d3ddcnt_pd3dDevice = NULL;
	DxEventNotifier::Instance().d3ddcnt_deviceLost = true;
	for( list<DxEventReceiver*>::reverse_iterator it = DxEventNotifier::Instance().DXNotifyTargets.rbegin(); it != DxEventNotifier::Instance().DXNotifyTargets.rend(); it++ )
		(*it)->OnDestroyDevice();
}
//
void DxEventNotifier::RegisterNotifyTarget(DxEventReceiver * rh)
{
	DXNotifyTargets.push_back( rh );
}
//
void DxEventNotifier::UnregisterNotifyTarget(DxEventReceiver * rh)
{
	list<DxEventReceiver*>::iterator it = find(DXNotifyTargets.begin(), DXNotifyTargets.end(), rh );
	if( it != DXNotifyTargets.end() ) 
		DXNotifyTargets.erase( it );
}
//

//
DxEventReceiver::DxEventReceiver( )
{
   DxEventNotifier::Instance().RegisterNotifyTarget( this );
}

DxEventReceiver::~DxEventReceiver()
{
	DxEventNotifier::Instance().UnregisterNotifyTarget( this );
}
