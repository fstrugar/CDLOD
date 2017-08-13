//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#pragma once

#include "DxEventNotifier.h"

class DemoCamera : DxEventReceiver
{
   enum ViewProjOverride
   {
      vpoDisabled          = 0,
      vpoEnabledNoFrustum,
      vpoEnabledWithFrustum
   };

protected:
   //
   float                m_fFOV;
   float                m_fAspect;
   float                m_fNear;
   float                m_fFar;
   //
   float                m_fYaw;
   float                m_fPitch;
   D3DXVECTOR3          m_vPos;
   D3DXVECTOR3          m_vDir;
   //
   D3DXMATRIX           m_mView;
   D3DXMATRIX           m_mProj;
   //
   float                m_fSpeed;
   float                m_fViewRange;
   //
   float                m_fSpeedKeyMod;
   //
   //                   for debugging/screenshot purposes
   ViewProjOverride     m_viewProjOverride;
   D3DXMATRIX           m_overrideViewMat;
   D3DXMATRIX           m_overrideProjMat;

public:
   DemoCamera();
   virtual ~DemoCamera();
   //
public:
   void                 Initialize();
   //
   float                GetViewRange( ) const                   { return m_fViewRange; }
   void                 SetViewRange( float viewRange )        { m_fViewRange = viewRange; }
   float                GetSpeed( ) const                      { return m_fSpeed; }
   void                 SetSpeed( float speed )                { m_fSpeed = speed; }
   //
   void                 SetAspect( float aspect )              { m_fAspect = aspect; }
   //
   void                 Tick( float deltaTime );
   //
   float                GetFOV() const                         { return m_fFOV; }
   float                GetAspect() const                      { return m_fAspect; }
   //
   void                 SetFOV( float fov )                    { m_fFOV = fov; }
   //
   float                GetYaw() const                         { return m_fYaw; }
   float                GetPitch() const                       { return m_fPitch; }
   const D3DXVECTOR3 &  GetPosition() const                    { return m_vPos; }
   const D3DXVECTOR3 &  GetDirection() const                   { return m_vDir; }
   //
   void                 SetPosition( D3DXVECTOR3 & position )  { m_vPos = position; }
   void                 SetOrientation( float yaw, float pitch ) { m_fYaw = yaw; m_fPitch = pitch; }
   //
   void                 GetFrustumPlanes( D3DXPLANE pPlanes[6] ) const;
   //
   D3DXMATRIX           GetViewMatrix() const;
   D3DXMATRIX           GetProjMatrix() const;
   //
   void                 GetZOffsettedProjMatrix( D3DXMATRIX & mat, float zModMul = 1.0f, float zModAdd = 0.0f ) const;
   //
	virtual HRESULT      OnResetDevice(const D3DSURFACE_DESC* pBackBufferSurfaceDesc);
   //
   //private:
   //   void                 UpdateCollision( float deltaTime );
   //
};
