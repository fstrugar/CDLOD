//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#pragma once

#include "DxEventNotifier.h"
#include "DemoCamera.h"

// Manages directional light, drawing of the sun, etc
class DemoSky : protected DxEventReceiver
{
   D3DXVECTOR3                   m_directionalLightDir;
   //
   D3DXVECTOR3                   m_directionalLightDirTarget;
   D3DXVECTOR3                   m_directionalLightDirTargetL1;
   //
   ID3DXMesh *                   m_sunMesh;
   //
   float                         m_sunAzimuth;
   float                         m_sunElevation;
   //
public:
   DemoSky(void);
   ~DemoSky(void);
   //
   const D3DXVECTOR3 &           GetDirectionalLightDir()               { return m_directionalLightDir; }
   //
   void                          Tick( float deltaTime );
   HRESULT                       Render( DemoCamera * pCamera );
   //
   void                          GetSunPosition( float & azimuth, float & elevation ) const     { azimuth = m_sunAzimuth; elevation = m_sunElevation; }
   void                          SetSunPosition( const float azimuth, const float elevation )   { m_sunAzimuth = azimuth; m_sunElevation = elevation; }
   //
protected:
   virtual HRESULT 				   OnCreateDevice( );
   virtual void                  OnDestroyDevice( );
   //
};
