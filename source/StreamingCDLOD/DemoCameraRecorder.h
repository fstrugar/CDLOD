//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////


#include <vector>

class DemoCamera;


class DemoCameraRecorder
{
#pragma pack( push, 8 )
	struct KeyFrameEvent
	{
		int						ID;
		//
		bool					SaveToFile( HANDLE hFile );
		bool					LoadFromFile( HANDLE hFile );
		//
		KeyFrameEvent() {}
		KeyFrameEvent(int ID) : ID(ID) {}
	};
	struct KeyFrame
	{
		D3DXQUATERNION		Orientation;
		D3DXVECTOR3			Position;
		float					Time;

      std::vector<KeyFrameEvent>	Events;

		void					ToCamera( DemoCamera * camera );
		static KeyFrame	FromCamera( DemoCamera * camera, float time = 0 );
		static KeyFrame	Interpolate( const KeyFrame & l, const KeyFrame & a, const KeyFrame & b, const KeyFrame & r, float f );
		//
		bool					SaveToFile( HANDLE hFile, int version );
		bool					LoadFromFile( HANDLE hFile, int version );
	};
#pragma pack( pop )
	//
	DemoCamera *         camera;
	//
	bool						isRecording;
	bool						isPlaying;
	float						progressTime;
	float						totalTime;
	//
	float						avgRecKeyTime;
	float						fromLastRecKeyTime;
	//
	KeyFrame					cameraRestore;
	int							lastFoundPlayKey;
	float						playSpeed;
	bool						loop;
	//
   std::vector<KeyFrame>			keyFrames;
	int							lastPlayedEventsFrame;
	//
public:
	std::vector<KeyFrameEvent>		frameEvents;
	//
public:
	DemoCameraRecorder( DemoCamera * camera );
	~DemoCameraRecorder();
	//
	void						StartRecording( );
	bool						IsRecording( );
	//
	void						StartPlaying( float playSpeed = 1.0f, float timeFrom = 0 );
	bool						IsPlaying( );
	//
	void						Stop( );
	//
	void						Update( float deltaTime );
	float						GetProgressTime();
	float						GetTotalTime();
	//
	bool						Save( const wchar_t * fileName );
	bool						Load( const wchar_t * fileName );
	//
private:
	void						UpdateRecord( bool forceRecord = false );
	void						UpdatePlay( );
	void						FindKeys( float time, int & l, int & a, int & b, int & r, float & f );
};
