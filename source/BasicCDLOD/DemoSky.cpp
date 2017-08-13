//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "Common.h"

#include "DemoSky.h"

//
DemoSky::DemoSky()
{
   // initial pos
   m_directionalLightDir = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );

   m_sunAzimuth   = 200.0f / 180.0f * (float)PI;
   m_sunElevation = 30.0f / 180.0f * (float)PI;

   m_directionalLightDirTarget   = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
   m_directionalLightDirTargetL1 = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );

   m_sunMesh = NULL;

   if( DxEventNotifier::GetD3DDevice() != NULL )
      OnCreateDevice( );
}
//
DemoSky::~DemoSky()
{
   DemoSky::OnDestroyDevice();
}
//
void DemoSky::Tick( float deltaTime )
{
   // this smoothing is not needed here, but I'll leave it in anyway
   static float someValue = 1000.0f;
   float lf = timeIndependentLerpF( deltaTime, someValue );

   if( D3DXVec3LengthSq(&m_directionalLightDir) < 1e-5f )
      lf = 1.0f;

   D3DXMATRIX mCameraRot;
   D3DXMATRIX mRotationY; D3DXMatrixRotationY( &mRotationY, m_sunElevation );
   D3DXMATRIX mRotationZ; D3DXMatrixRotationZ( &mRotationZ, m_sunAzimuth );
   mCameraRot = mRotationY * mRotationZ;
   m_directionalLightDirTarget = D3DXVECTOR3( mCameraRot.m[0] );


   D3DXVec3Lerp( &m_directionalLightDirTargetL1, &m_directionalLightDirTargetL1, &m_directionalLightDirTarget, lf );
   D3DXVec3Lerp( &m_directionalLightDir, &m_directionalLightDir, &m_directionalLightDirTargetL1, lf );

   D3DXVec3Normalize( &m_directionalLightDirTarget, &m_directionalLightDirTarget );
   D3DXVec3Normalize( &m_directionalLightDirTargetL1, &m_directionalLightDirTargetL1 );
   D3DXVec3Normalize( &m_directionalLightDir, &m_directionalLightDir );
}
//
HRESULT DemoSky::Render( DemoCamera * pCamera )
{
   IDirect3DDevice9 * pDevice = DxEventNotifier::GetD3DDevice();

   D3DXMATRIX view = pCamera->GetViewMatrix();
   D3DXMATRIX proj = pCamera->GetProjMatrix();

   //D3DXMATRIX proj;
   //D3DXMatrixPerspectiveFovLH( &proj, pCamera->GetFOV(), pCamera->GetAspect(), 10.0f, 2e8 );

   view._41 = 0.0; view._42 = 0.0; view._43 = 0.0;
   //D3DXMATRIX viewProj = view * proj;

   D3DXMATRIXA16 matWorld;
   D3DXMatrixTranslation( &matWorld, m_directionalLightDir.x * -1e4f, m_directionalLightDir.y * -1e4f, m_directionalLightDir.z * -1e4f );
   pDevice->SetTransform( D3DTS_WORLD, &matWorld );

   pDevice->SetTransform( D3DTS_VIEW, &view );
   pDevice->SetTransform( D3DTS_PROJECTION, &proj );

   D3DMATERIAL9 sunMat;
   sunMat.Ambient.r = 1.0f; sunMat.Ambient.g = 1.0f; sunMat.Ambient.b = 1.0f; sunMat.Ambient.a = 1.0f;
   sunMat.Diffuse.r = 1.0f; sunMat.Diffuse.g = 1.0f; sunMat.Diffuse.b = 1.0f; sunMat.Diffuse.a = 1.0f;
   sunMat.Emissive.r = 1.0f; sunMat.Emissive.g = 1.0f; sunMat.Emissive.b = 1.0f; sunMat.Emissive.a = 1.0f;
   sunMat.Specular.r = 1.0f; sunMat.Specular.g = 1.0f; sunMat.Specular.b = 1.0f; sunMat.Specular.a = 1.0f;
   sunMat.Power = 1.0f;

   pDevice->SetFVF( m_sunMesh->GetFVF() );

   pDevice->SetMaterial( &sunMat );

   pDevice->SetTexture( 0, NULL );

   pDevice->SetVertexShader( NULL );
   pDevice->SetPixelShader( NULL );
   pDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
   pDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );

   m_sunMesh->DrawSubset( 0 );

   return S_OK;
}
//
HRESULT DemoSky::OnCreateDevice( )
{
   HRESULT hr;

   SAFE_RELEASE( m_sunMesh );

   IDirect3DDevice9 * pDevice = DxEventNotifier::GetD3DDevice();

   hr = D3DXCreateSphere( pDevice, 3e2f, 9, 9, &m_sunMesh, NULL );

   return hr;
}
//
void DemoSky::OnDestroyDevice( )
{
   if( m_sunMesh != NULL )
   {
      m_sunMesh->Release();
      m_sunMesh = NULL;
   }
}
//
