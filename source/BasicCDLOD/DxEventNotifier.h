//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#pragma once

#pragma warning (disable : 4995)
#include <list>
#include <algorithm>
#include <string>

class DxEventReceiver;

// Singleton class
class DxEventNotifier
{
private:
	IDirect3DDevice9 *            d3ddcnt_pd3dDevice;
	bool                          d3ddcnt_deviceLost;		
   std::list<DxEventReceiver*>   DXNotifyTargets;
   D3DSURFACE_DESC               backBufferSurfaceDesc;
	//
	DxEventNotifier();
   ~DxEventNotifier();
   //
public:
	//
	static HRESULT                PostCreateDevice(IDirect3DDevice9* pd3dDevice);
	static HRESULT                PostResetDevice(const D3DSURFACE_DESC* pBackBufferSurfaceDesc);
	static void                   PostLostDevice();
	static void                   PostDestroyDevice();
	//
private:
	friend class DxEventReceiver;
	//
	void                          RegisterNotifyTarget(DxEventReceiver * rh);
	void                          UnregisterNotifyTarget(DxEventReceiver * rh);	
	//
public:
   static IDirect3DDevice9 *     GetD3DDevice()                                  { return Instance().d3ddcnt_pd3dDevice; }
   static bool                   IsD3DDeviceLost()                               { return Instance().d3ddcnt_deviceLost; }
   static D3DSURFACE_DESC &      GetBackbufferSurfaceDesc()                      { return Instance().backBufferSurfaceDesc; }
   //
private:
   static DxEventNotifier &      Instance();
};

//
class DxEventReceiver
{
protected:
	friend class DxEventNotifier;
	//
protected:
	//
	DxEventReceiver();
	virtual ~DxEventReceiver();
	//
   IDirect3DDevice9 *            GetD3DDevice() const                            { return DxEventNotifier::GetD3DDevice(); }
	bool                          IsD3DDeviceLost() const                         { return DxEventNotifier::IsD3DDeviceLost(); }
   const D3DSURFACE_DESC &       GetBackbufferSurfaceDesc() const                { return DxEventNotifier::GetBackbufferSurfaceDesc(); }
   //
	virtual HRESULT 				   OnCreateDevice()                                { return S_OK; };
	virtual HRESULT               OnResetDevice(const D3DSURFACE_DESC* pBackBufferSurfaceDesc)   { return S_OK; };
	virtual void                  OnLostDevice()                                  {};
	virtual void                  OnDestroyDevice()                               {};
};
