//////////////////////////////////////////////////////////////////////////
// VertexAsylum Codebase, Copyright (c) Filip Strugar.
// Contents of this file are distributed under Zlib license.
// (http://en.wikipedia.org/wiki/Zlib_License)
//////////////////////////////////////////////////////////////////////////

#include "Core\IO\vaFileStream.h"

#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////
// vaFileStream
//////////////////////////////////////////////////////////////////////////////
using namespace VertexAsylum;
//
vaFileStream::vaFileStream( )
{ 
   m_file = 0; 
}
vaFileStream::~vaFileStream(void) 
{
   Close(); 
}
//
#pragma warning ( disable: 4996 )
//
bool vaFileStream::Open(const wchar_t * file_path, bool read )
{
   if( IsOpen() ) return false;

   if( read )
      m_file = _wfopen(file_path, L"rb"); // File.Open( path, FileMode.Open, FileAccess.Read, FileShare.Read );
   else
      m_file = _wfopen(file_path, L"w+b"); // File.Open( path, FileMode.Open, FileAccess.ReadWrite, FileShare.None );

   if( m_file == NULL )
      return false;

   return true;
}
//
bool vaFileStream::Open(const char * file_path, bool read )
{
   if( IsOpen() ) return false;

   if( read )
      m_file = fopen(file_path, "rb"); // File.Open( path, FileMode.Open, FileAccess.Read, FileShare.Read );
   else
      m_file = fopen(file_path, "w+b"); // File.Open( path, FileMode.Open, FileAccess.ReadWrite, FileShare.None );

   if( m_file == NULL )
      return false;

   return true;
}
//
int vaFileStream::Read( void * buffer, int count )
{
   return (int)fread( buffer, 1, count, m_file );
}
//
int vaFileStream::Write( const void * buffer, int count )
{
   return (int)fwrite( buffer, 1, count, m_file );
}
//
void vaFileStream::Seek(__int64 position)
{
   _fseeki64( m_file, position, SEEK_SET );
}
//
void vaFileStream::Close()
{
   if( m_file == NULL ) return;
   fclose( m_file );
   m_file = NULL;
}
//
bool vaFileStream::IsOpen()		
{ 
   return m_file != NULL;
}
//
__int64 vaFileStream::GetLength()		
{ 
   __int64 current = _ftelli64(m_file);
   _fseeki64(m_file, 0, SEEK_END);
   __int64 filelength = _ftelli64(m_file);
   _fseeki64(m_file, current, SEEK_SET);
   return filelength;
}
//
__int64 vaFileStream::GetPosition()	
{ 
   return _ftelli64(m_file);
}
//////////////////////////////////////////////////////////////////////////////
