//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "Common.h"

#include "DemoCamera.h"

#include "DxCanvas.h"

#include "CDLODStreaming\CDLODStreamingStorage.h"

#include "DemoCameraRecorder.h"

using namespace std;

#pragma warning( disable : 4996 )

static string GetCamRecFilename()
{
   string exePath = vaGetProjectDirectory();
   return exePath + "\\camera.scriptrec";
}

static bool SerializeCamera( D3DXVECTOR3 & pos, float & yaw, float & pitch, float & speed, float & viewRange, bool load )
{
   string exePath = vaGetProjectDirectory();
   exePath += "\\camera.lastposrot";

   if( load )
   {
      FILE * file = fopen( exePath.c_str(), "rb" );
      if( file == NULL )
         return false;

      int ver;
      if( fread( &ver, sizeof(ver), 1, file ) != 1 )
         goto loadErrorRet;

      if( ver != 1 )
         goto loadErrorRet;

      if( fread( &pos, sizeof(pos), 1, file ) != 1 )
         goto loadErrorRet;

      if( fread( &yaw, sizeof(yaw), 1, file ) != 1 )
         goto loadErrorRet;

      if( fread( &pitch, sizeof(pitch), 1, file ) != 1 )
         goto loadErrorRet;

      if( fread( &speed, sizeof(speed), 1, file ) != 1 )
         goto loadErrorRet;

      if( fread( &viewRange, sizeof(viewRange), 1, file ) != 1 )
         goto loadErrorRet;

      return true;

      loadErrorRet:
      fclose( file );
      return false;
   }
   else
   {
      FILE * file = fopen( exePath.c_str(), "wb" );
      if( file == NULL )
         return false;

      fseek( file, 0, SEEK_SET );

      int ver = 1;
      if( fwrite( &ver, sizeof(ver), 1, file ) != 1 )
         goto saveErrorRet;

      if( fwrite( &pos, sizeof(pos), 1, file ) != 1 )
         goto saveErrorRet;

      if( fwrite( &yaw, sizeof(yaw), 1, file ) != 1 )
         goto saveErrorRet;

      if( fwrite( &pitch, sizeof(pitch), 1, file ) != 1 )
         goto saveErrorRet;

      if( fwrite( &speed, sizeof(speed), 1, file ) != 1 )
         goto saveErrorRet;

      if( fwrite( &viewRange, sizeof(viewRange), 1, file ) != 1 )
         goto saveErrorRet;

      return true;

      saveErrorRet:
      fclose( file );
      return false;
   }

}

DemoCamera::DemoCamera()
{
   m_fFOV               = 120.0f / 360.0f * (float)PI;
   m_fAspect            = 1.0f;
   m_fNear              = 0.1f;
   m_fFar               = 10.0f;
   //
   m_fYaw               = 0.88f;
   m_fPitch             = 0.5f;
   m_vPos               = D3DXVECTOR3( -17747.660f, -19706.039f, 17016.523f );
   //
   D3DXMatrixIdentity( &m_mView );
   D3DXMatrixIdentity( &m_mProj );
   //
   m_fSpeed             = 500.0f;
   m_fViewRange         = 0.0f;
   //
   m_fSpeedKeyMod       = 0.0f;
   //
   m_accumMouseDeltaX      = 0.0f;
   m_accumMouseDeltaY      = 0.0f;
   m_accumMove      = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
   //
   m_inputSmoothingLerpK   = 200.0f;
   //
   {
      D3DXMATRIX mCameraRot;
      D3DXMATRIX mRotationY; D3DXMatrixRotationY( &mRotationY, m_fPitch );
      D3DXMATRIX mRotationZ; D3DXMatrixRotationZ( &mRotationZ, m_fYaw );
      //
      mCameraRot = mRotationY * mRotationZ;
      m_vDir = D3DXVECTOR3( mCameraRot.m[0] );
      //
      // vaGetProjectDefaultCameraParams( m_vPos, m_vDir, m_fViewRange );
      //
      // set pitch/yaw from dir here...
   }
   //
   m_bInitialSettingsSerialised = false;
//#ifdef _DEBUG
   m_bInitialSettingsSerialised = SerializeCamera( m_vPos, m_fYaw, m_fPitch, m_fSpeed, m_fViewRange, true );
//#endif

   m_viewProjOverride      = vpoDisabled;

   m_observerID = 0;

   m_recorder = new DemoCameraRecorder(this);
}
//
DemoCamera::~DemoCamera()
{
//#ifdef _DEBUG
   SerializeCamera( m_vPos, m_fYaw, m_fPitch, m_fSpeed, m_fViewRange, false );
//#endif
   delete m_recorder;
}
//
static void GetMapOrthoViewProj( const MapDimensions & mapDims, D3DXMATRIX & view, D3DXMATRIX & proj )
{
   D3DXVECTOR3 eye( mapDims.MinX + mapDims.SizeX / 2.0f, mapDims.MinY + mapDims.SizeY / 2.0f, mapDims.MinZ + mapDims.SizeZ );
   D3DXVECTOR3 up( 0, -1, 0 );
   D3DXVECTOR3 at( 0, 0, 0 );

   D3DXMatrixLookAtLH( &view, &eye, &at, &up );
   D3DXMatrixOrthoLH( &proj, mapDims.SizeX, mapDims.SizeY, 0, mapDims.SizeZ );
}
//
void DemoCamera::Initialize( CDLODStreamingStorage * storage, float minViewRange, float maxViewRange )
{
   m_storage = storage;
   
   if( storage == NULL )
      return;

   if( !m_bInitialSettingsSerialised )
   {
      // Just setup camera settings so that we can see the loaded map

      m_fViewRange = (minViewRange * 9.0f + maxViewRange) / 10.0f;

      const MapDimensions & dims = storage->GetWorldMapDims();

      // Update the m_vDir and other stuff
      Tick( 0.0f );

      D3DXVECTOR3 midPt( dims.MinX + dims.SizeX*0.5f, dims.MinY + dims.SizeY*0.5f, dims.MinZ + dims.SizeZ*0.5f );

      m_vPos = midPt - m_fViewRange * 0.5f * m_vDir;
   }

   assert( m_observerID == 0 );
   m_observerID = m_storage->AddObserver( m_vPos, m_fViewRange, true );
   assert( m_observerID != 0 );

   GetMapOrthoViewProj( storage->GetWorldMapDims(), m_overrideViewMat, m_overrideProjMat );

   m_accumMouseDeltaX = 0.0f;
   m_accumMouseDeltaY = 0.0f;
   m_accumMove      = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
}
//
void DemoCamera::SetOrientation( D3DXVECTOR3 & direction )
{
   m_fYaw = atan2f(direction.y, direction.x);
   m_fPitch = -atan2f(direction.z, sqrtf((direction.x * direction.x) + (direction.y * direction.y)));
}
//
void DemoCamera::Tick( float deltaTime )
{
   bool updateFromKeyboard = true;

   if( vaIsKeyClicked(kcToggleCameraScriptRec) && !m_recorder->IsPlaying() )
   {
      if( m_recorder->IsRecording() )
      {
         m_recorder->Stop();
         wstring recPath = vaStringSimpleWiden( GetCamRecFilename() );
         if( m_recorder->Save(recPath.c_str()) )
            MessageBox(NULL, (wstring(L"Camera movement script saved at ") + recPath).c_str(), L"", MB_OK);
         else
            MessageBox(NULL, L"Error while trying to save camera movement script", L"", MB_OK);
      }
      else
      {
         m_recorder->StartRecording();
      }
   }
   if( vaIsKeyClicked(kcToggleCameraScriptPlay) && !m_recorder->IsRecording() )
   {
      if( m_recorder->IsPlaying() )
      {
         m_recorder->Stop();
      }
      else
      {
         wstring recPath = vaStringSimpleWiden( GetCamRecFilename() );
         if( m_recorder->Load(recPath.c_str()) )
         {
            m_recorder->StartPlaying(0.75f, 0.0f);
         }
      }
   }

   if( m_recorder->IsPlaying() )
   {
      m_recorder->Update( deltaTime );

      string info = vaStringFormat("DemoCameraRecorder: playing (time: %.2f of %.2f)", m_recorder->GetProgressTime(), m_recorder->GetTotalTime() );
      GetCanvas2D()->DrawString( GetBackbufferSurfaceDesc().Width/2 - 128, GetBackbufferSurfaceDesc().Height-32, 0xFFFF4040, info.c_str() );

      updateFromKeyboard = false;
   }

   float smoothingLerpK = timeIndependentLerpF( deltaTime, m_inputSmoothingLerpK );
   float speedBoost = vaIsKey( kcShiftKey )?(12.0f):(1.0f);
   speedBoost *= vaIsKey( kcControlKey )?(0.1f):(1.0f);
   //
   if( updateFromKeyboard )
   {
      //
      ///////////////////////////////////////////////////////////////////////////
      // Update camera range/speed changes
      //if( vaIsKeyClicked( kcCameraSpeedDown ) )   m_fSpeed *= 0.95f;
      //if( vaIsKeyClicked( kcCameraSpeedUp ) )     m_fSpeed *= 1.05f;
      m_fSpeed = clamp( m_fSpeed, 50.0f, 5000.0f );
      if( vaIsKeyClicked( kcViewRangeDown ) )     m_fViewRange *= 0.99f;
      if( vaIsKeyClicked( kcViewRangeUp ) )       m_fViewRange *= 1.01f;
      m_fViewRange = clamp( m_fViewRange, 1000.0f, 1000000.0f );
      //
      ///////////////////////////////////////////////////////////////////////////
      // Update camera rotation
      float cdX, cdY;
      vaGetCursorDelta( cdX, cdY );
      //
      // smoothing
      {
         m_accumMouseDeltaX += cdX;
         m_accumMouseDeltaY += cdY;
         cdX = smoothingLerpK * m_accumMouseDeltaX;
         cdY = smoothingLerpK * m_accumMouseDeltaY;
         m_accumMouseDeltaX = (1 - smoothingLerpK) * m_accumMouseDeltaX;
         m_accumMouseDeltaY = (1 - smoothingLerpK) * m_accumMouseDeltaY;
      }
      //
      m_fYaw		+= cdX * 0.005f;
      m_fPitch	   += cdY * 0.003f;
      m_fYaw		= vaWrapAngle( m_fYaw );
      m_fPitch	= clamp( m_fPitch, -(float)PI/2 + 1e-1f, +(float)PI/2 - 1e-1f );
      //
   }
   ///////////////////////////////////////////////////////////////////////////
   // Update camera matrices
   D3DXMATRIX mCameraRot;
   D3DXMATRIX mRotationY; D3DXMatrixRotationY( &mRotationY, m_fPitch );
   D3DXMATRIX mRotationZ; D3DXMatrixRotationZ( &mRotationZ, m_fYaw );
   mCameraRot = mRotationY * mRotationZ;
   //
   if( updateFromKeyboard )
   {
      ///////////////////////////////////////////////////////////////////////////
      // Update camera movement
      bool hasInput = vaIsKey(kcForward) || vaIsKey(kcBackward) || vaIsKey(kcRight) || vaIsKey(kcLeft) || vaIsKey(kcUp) || vaIsKey(kcDown);
      m_fSpeedKeyMod = (hasInput)?(::min(m_fSpeedKeyMod + deltaTime, 2.0f)):(0.0f);
      float moveSpeed = m_fSpeed * deltaTime * (0.2f + 0.8f * (m_fSpeedKeyMod / 2.0f) * (m_fSpeedKeyMod / 2.0f)) * speedBoost;

      D3DXVECTOR3    forward( mCameraRot.m[0] );
      D3DXVECTOR3    left( mCameraRot.m[1] );
      D3DXVECTOR3    up( mCameraRot.m[2] );

      if( vaIsKey( kcForward ) )   m_accumMove += forward * moveSpeed;
      if( vaIsKey( kcBackward ) )  m_accumMove -= forward * moveSpeed;
      if( vaIsKey( kcRight) )      m_accumMove += left * moveSpeed;
      if( vaIsKey( kcLeft) )       m_accumMove -= left * moveSpeed;
      if( vaIsKey( kcUp) )         m_accumMove += up * moveSpeed;
      if( vaIsKey( kcDown ) )      m_accumMove -= up * moveSpeed;

      m_vPos += m_accumMove * smoothingLerpK;
      m_accumMove *= (1-smoothingLerpK);
   }
   //
   //UpdateCollision( deltaTime );
   //
   m_vDir = D3DXVECTOR3( mCameraRot.m[0] );
   D3DXVECTOR3 vLookAtPt = m_vPos + m_vDir * 100.0f;
   D3DXVECTOR3 vUpVec( 0.0f, 0.0f, 1.0f );
   //
   //now do these two...
   D3DXMatrixLookAtLH( &m_mView, &m_vPos, &vLookAtPt, &vUpVec );

   m_fNear = 3.0f; //m_fViewRange / 20000.0f;
   m_fFar  = m_fViewRange * 1.5f;
   D3DXMatrixPerspectiveFovLH( &m_mProj, m_fFOV, m_fAspect, m_fNear, m_fFar );

   if( vaIsKeyClicked( kcToggleViewProjOverride ) )
      m_viewProjOverride = (ViewProjOverride)((m_viewProjOverride+1) % 3);

   if( vaIsKeyClicked( kcSaveViewProjOverride ) )
   {
      m_overrideViewMat = m_mView;
      m_overrideProjMat = m_mProj;
   }
   if( vaIsKeyClicked( kcOrthoViewProjOverride ) )
   {
      GetMapOrthoViewProj( m_storage->GetWorldMapDims(), m_overrideViewMat, m_overrideProjMat );
   }
   if( m_viewProjOverride != vpoDisabled )
   {
      // draw camera position marker

      D3DXMATRIX world; D3DXMatrixIdentity( &world );
      D3DXMATRIX view   = GetViewMatrix();
      D3DXMATRIX proj   = GetProjMatrix();

      D3DVIEWPORT9 viewport;
      GetD3DDevice()->GetViewport( &viewport );

      D3DXVECTOR3 pos2D;
      D3DXVec3Project( &pos2D, &m_vPos, &viewport, &proj, &view, &world );

      GetCanvas2D()->DrawString( (int)pos2D.x, (int)pos2D.y, 0xFF000000, "Camera is here" );
   }

   if( (m_storage != NULL) && (m_observerID != 0) )
   {
      m_storage->UpdateObserver( m_observerID, m_vPos, m_fViewRange );
   }

   if( m_recorder->IsRecording() )
   {
      m_recorder->Update( deltaTime );
      
      string info = vaStringFormat("DemoCameraRecorder: recording (time: %.2f of %.2f)", m_recorder->GetProgressTime(), m_recorder->GetTotalTime() );
      GetCanvas2D()->DrawString( GetBackbufferSurfaceDesc().Width/2 - 128, GetBackbufferSurfaceDesc().Height-32, 0xFFFF4040, info.c_str() );
   }
}
//
HRESULT DemoCamera::OnResetDevice(const D3DSURFACE_DESC* pBackBufferSurfaceDesc)
{
   SetAspect( pBackBufferSurfaceDesc->Width / (float) pBackBufferSurfaceDesc->Height );
   return S_OK;
}
//
D3DXMATRIX DemoCamera::GetViewMatrix() const
{
   if( m_viewProjOverride == vpoDisabled )
      return m_mView; 
   else
      return m_overrideViewMat;
}
D3DXMATRIX DemoCamera::GetProjMatrix() const
{
   if( m_viewProjOverride == vpoDisabled )
      return m_mProj; 
   else
      return m_overrideProjMat;
}
//
void DemoCamera::GetFrustumPlanes( D3DXPLANE pPlanes[6] ) const
{
   D3DXMATRIX mCameraViewProj;
   if( m_viewProjOverride == vpoEnabledWithFrustum ) 
      mCameraViewProj = m_overrideViewMat * m_overrideProjMat;
   else
      mCameraViewProj = m_mView * m_mProj;

   vaGetFrustumPlanes(pPlanes, mCameraViewProj);
}
//
void DemoCamera::GetZOffsettedProjMatrix( D3DXMATRIX & mat, float zModMul, float zModAdd ) const
{
   D3DXMatrixPerspectiveFovLH( &mat, m_fFOV, m_fAspect, m_fNear * zModMul + zModAdd, m_fFar * zModMul + zModAdd );
}
//
