//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "Common.h"

#include "DemoCamera.h"

#include "DxCanvas.h"

#include "DemoCameraRecorder.h"

//////////////////////////////////////////////////////////////////////////
//
// DemoCameraRecorder
//
//////////////////////////////////////////////////////////////////////////


//
template<class T>
static bool FileWrite( HANDLE hFile, const T & value )
{
   DWORD dw;
   if( !::WriteFile( hFile, &value, sizeof(value), &dw, NULL ) )
      return false;
   if( dw != sizeof(value) )
      return false;
   return true;
}
//
template<class T>
static bool FileRead( HANDLE hFile, T & value )
{
   DWORD dw;
   if( !::ReadFile( hFile, &value, sizeof(value), &dw, NULL ) )
      return false;
   if( dw != sizeof(value) )
      return false;
   return true;
}

const int c_ColorCount = 6;
const int c_Colors[c_ColorCount] = { 0xFFFFC0C0, 0xFFC0FFC0, 0xFFC0C0FF, 0xFFFFFFA0, 0xFFFFA0FF, 0xFFA0FFFF };

DemoCameraRecorder::KeyFrame DemoCameraRecorder::KeyFrame::Interpolate( const KeyFrame & l, const KeyFrame & a, const KeyFrame & b, const KeyFrame & r, float f )
{
	KeyFrame key;
	key.Time			= a.Time + (b.Time - a.Time) * f;
	D3DXVec3CatmullRom( &key.Position, &l.Position, &a.Position, &b.Position, &r.Position, f );

	D3DXQUATERNION qa, qb, qc;
	D3DXQuaternionSquadSetup( &qa, &qb, &qc, &l.Orientation, &a.Orientation, &b.Orientation, &r.Orientation );
	D3DXQuaternionSquad( &key.Orientation, &a.Orientation, &qa, &qb, &qc, f );

	return key;
}
//
DemoCameraRecorder::KeyFrame DemoCameraRecorder::KeyFrame::FromCamera( DemoCamera * camera, float time )
{
	KeyFrame key;
	key.Time			= time;
	key.Position		= camera->GetPosition();

	D3DXQUATERNION ry, rz;
   const D3DXVECTOR3 forward(0, 1, 0);
   const D3DXVECTOR3 up(0, 0, 1);
	D3DXQuaternionRotationAxis( &ry, &forward, camera->GetPitch() );
	D3DXQuaternionRotationAxis( &rz, &up, camera->GetYaw() );
	key.Orientation = ry * rz;

	return key;
}
//
void DemoCameraRecorder::KeyFrame::ToCamera( DemoCamera * camera )
{
	//	camera->SetViewParams( &Position, &(Position + LookDirection) );
	D3DXMATRIX mat;
	D3DXMatrixRotationQuaternion( &mat, &Orientation );
   D3DXVECTOR3 dir = D3DXVECTOR3( mat.m[0] );

   camera->SetPosition( Position );
   camera->SetOrientation( dir );
}
//
bool DemoCameraRecorder::KeyFrameEvent::SaveToFile( HANDLE hFile )
{
	if( !FileWrite( hFile, ID) )		return false;
	return true;
}
bool DemoCameraRecorder::KeyFrameEvent::LoadFromFile( HANDLE hFile )
{
	if( !FileRead( hFile, ID) )			return false;
	return true;
}
//
bool DemoCameraRecorder::KeyFrame::SaveToFile( HANDLE hFile, int version )
{
	if( !FileWrite( hFile, Orientation ) )	return false;
	if( !FileWrite( hFile, Position ) )		return false;
	if( !FileWrite( hFile, Time) )			return false;

	if( version == 0 ) return true;

	int eventCount = (int)Events.size();
	if( !FileWrite( hFile, eventCount ) )	return false;

	for( size_t i = 0; i < Events.size(); i++ )
		if( !Events[i].SaveToFile( hFile ) )
			return false;

	return true;
}
bool DemoCameraRecorder::KeyFrame::LoadFromFile( HANDLE hFile, int version )
{
	if( !FileRead( hFile, Orientation ) )	return false;
	if( !FileRead( hFile, Position ) )		return false;
	if( !FileRead( hFile, Time) )			return false;

	if( version == 0 ) return true;

	int eventCount;
	if( !FileRead( hFile, eventCount ) )	return false;

	Events.resize( eventCount );
	for( size_t i = 0; i < Events.size(); i++ )
		if( !Events[i].LoadFromFile( hFile ) )
			return false;

	return true;
}
//
DemoCameraRecorder::DemoCameraRecorder( DemoCamera * camera ) 
: camera(camera)
{
	progressTime	= 0;
	totalTime		= 0;
	isPlaying		= false;
	isRecording		= false;
	avgRecKeyTime	= 1.0f;
	loop			= false;
}
//
DemoCameraRecorder::~DemoCameraRecorder() 
{
}
//
void DemoCameraRecorder::StartRecording( )
{
	if( isRecording ) Stop();
	if( isPlaying ) return;

	progressTime		= 0;
	totalTime			= 0;
	isRecording			= true;
	fromLastRecKeyTime	= 0;

	erase(keyFrames);
	UpdateRecord( true );
}
//
bool DemoCameraRecorder::IsRecording( )
{
	return isRecording;
}
//
void DemoCameraRecorder::StartPlaying( float playSpeed, float timeFrom )
{
	if( isPlaying ) Stop();
	if( isRecording ) return;
	if( keyFrames.empty() ) return;

	this->playSpeed = playSpeed;
	progressTime = timeFrom;
	isPlaying = true;
	lastFoundPlayKey		= -1;
	lastPlayedEventsFrame	= 0;
	frameEvents.clear();
}
//
bool DemoCameraRecorder::IsPlaying( )
{
	return isPlaying;
}
//
void DemoCameraRecorder::Stop( )
{
	if( isPlaying ) 
	{
		isPlaying = false;
	}

	if( isRecording )
	{
		UpdateRecord( true );
		isRecording = false;
	}
	frameEvents.clear();
}
//
void DemoCameraRecorder::Update( float deltaTime )
{
	if( isPlaying ) 
	{
		progressTime += deltaTime * playSpeed;

		if( progressTime > totalTime )
		{
			if( loop )
				progressTime = 0;
			else
			{
				progressTime = totalTime;
				Stop();
				return;
			}
		}
		UpdatePlay( );
	}

	if( isRecording )
	{
		progressTime		+= deltaTime;
		totalTime			+= deltaTime;
		fromLastRecKeyTime	+= deltaTime;
		UpdateRecord( );
	}
}
//
void DemoCameraRecorder::UpdateRecord( bool forceRecord )
{
	if( forceRecord )
		fromLastRecKeyTime = 0;
	else
		if( fromLastRecKeyTime < avgRecKeyTime ) 
			return;
		else
			fromLastRecKeyTime -= avgRecKeyTime;

	keyFrames.push_back( KeyFrame::FromCamera( camera, progressTime ) );
	keyFrames.back().Events = frameEvents;
	frameEvents.clear();
}
//
void DemoCameraRecorder::UpdatePlay( )
{
	if( keyFrames.empty() ) return;
	if( (keyFrames.size() == 1) || progressTime <= keyFrames[0].Time) 
	{
		keyFrames[0].ToCamera( camera );
		return;
	}
	if( (keyFrames.size() == 1) || progressTime >= keyFrames[keyFrames.size()-1].Time) 
	{
		keyFrames[keyFrames.size()-1].ToCamera( camera );
		return;
	}

	int l, a, b, r;
	float f;

	FindKeys( progressTime, l, a, b, r, f );
	KeyFrame::Interpolate( keyFrames[l], keyFrames[a], keyFrames[b], keyFrames[r], f ).ToCamera( camera );

	if( b > lastPlayedEventsFrame )
	{
		lastPlayedEventsFrame = b;
		frameEvents = keyFrames[b].Events;
	}
}
//
void DemoCameraRecorder::FindKeys( float time, int & l, int & a, int & b, int & r, float & f )
{
	if (lastFoundPlayKey == -1)
		lastFoundPlayKey = 0;//(int)(sceneKeyFrames.Count * recRes / time);

	a = lastFoundPlayKey;
	l = r = b = a;

	int la = -1;

	int jumpDistance = 0;
	int jumpDivider = 3; //MUST be > 1, should be > 3
	f = 0;
	bool forward = true;

	int iteration = 0;

	while( 1 )
	{
		a = clamp( a, 0, (int)keyFrames.size() - 1 );
		b = clamp( a + 1, 0, (int)keyFrames.size() - 1 );

		//if last iteration had same a, and total jump distance is 1, then we're blocked here
		if( (la == a) && ((jumpDistance / jumpDivider) == 0) )
			break;
		la = a;

		const KeyFrame & ka = keyFrames[a];
		const KeyFrame & kb = keyFrames[b];

		// if ka is before, and kb is after then we've found required indices
		if( ka.Time == time ) break; // f stays 0
		if( ka.Time < time && kb.Time >= time )
		{
			f = (time - ka.Time) / (kb.Time - ka.Time);
			break;
		}

		// change direction if required
		if ( ((ka.Time < time) && !forward) || ((ka.Time >= time) && forward) )
		{
			forward = !forward;
			jumpDistance = jumpDistance / 3;
		}
		else //else increase step
			jumpDistance = (jumpDistance == 0) ? (1) : (jumpDistance*2);

		a += ((forward)?(1):(-1)) * ( 1 + jumpDistance / jumpDivider );

		iteration++;
		if( iteration > 1000 ) //some bug maybe?
		{
			assert( false );
			break;
		}
	}

	l = clamp( a - 1, 0, (int)keyFrames.size() - 1 );
	r = clamp( b + 1, 0, (int)keyFrames.size() - 1 );

	lastFoundPlayKey = a;
}
//
float DemoCameraRecorder::GetProgressTime()
{
	return progressTime;
}
//
float DemoCameraRecorder::GetTotalTime()
{
	return totalTime;
}
//
bool DemoCameraRecorder::Save( const wchar_t * fileName )
{
	if( keyFrames.size() < 2 ) return false;
	HANDLE hFile = ::CreateFileW(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
	if( hFile == INVALID_HANDLE_VALUE )
		return false;

	DWORD dwBytesWritten;

	DWORD val = 0;
	//write zero for old version compatibility
	if( !::WriteFile( hFile, &val, 4, &dwBytesWritten, NULL ) )
		goto OnError;

	//write version
	int version = 1;
	if( !::WriteFile( hFile, &version, 4, &dwBytesWritten, NULL ) )
		goto OnError;

	size_t size = keyFrames.size(); 
	if( !::WriteFile( hFile, &size, 4, &dwBytesWritten, NULL ) )
		goto OnError;

	for( size_t i = 0; i < keyFrames.size(); i++ )
	{
		if( !keyFrames[i].SaveToFile( hFile, version ) )
			goto OnError;
	}

	CloseHandle( hFile );
	return true;

OnError: 
	CloseHandle( hFile );
	MessageBox(NULL, L"Error writing camera.scriptrec", L"", MB_OK);
	return false;
}
//
bool DemoCameraRecorder::Load( const wchar_t * fileName )
{
	HANDLE hFile = ::CreateFileW(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
	if( hFile == INVALID_HANDLE_VALUE )
		return false;

	int version = 0;

	size_t size;
	// read zero
	if( !FileRead( hFile, size ) ) goto OnError;

	if( size == 0 )
	{
		if( !FileRead( hFile, version ) ) goto OnError;
		if( !FileRead( hFile, size ) ) goto OnError;
	}

	progressTime	= 0;
	totalTime		= 0;

	keyFrames.resize(size);
	for( size_t i = 0; i < keyFrames.size(); i++ )
	{
		if( !keyFrames[i].LoadFromFile( hFile, version ) )
			goto OnError;

		if( keyFrames[i].Time < totalTime ) 
			goto OnError;
		totalTime = keyFrames[i].Time;
	}

	CloseHandle(hFile);
	return true;

OnError:
	keyFrames.clear();
	totalTime = 0;
	CloseHandle(hFile);
	MessageBox(NULL, L"Error reading camera.scriptrec", L"", MB_OK);
	return false;
}
//