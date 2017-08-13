//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Common.h"

#include <string>
#include <vector>

class SimpleProfilerObj
{
private:
   int64             m_beginTime;
   int64             m_collectedTime;
   int               m_collectedFrames;
   //
   double            m_lastAvgTime;
   //
   const std::string m_name;
   //
   SimpleProfilerObj& operator = ( const SimpleProfilerObj & other ) { assert(false); return *this; }
public:
   SimpleProfilerObj( const char * name );
   ~SimpleProfilerObj();
   //
   void              Begin();
   void              End();
   //
   void              ResetAndRecalc();
   double            GetLastAverageTimePerFrame() const  { return m_lastAvgTime; }
   //
   const char *      GetName() const { return m_name.c_str(); };
};

class SimpleProfiler
{
private:
   typedef std::vector<SimpleProfilerObj*> ProfObjsContainerType;
   ProfObjsContainerType      m_profObjs;

   LARGE_INTEGER              m_freq;
   float                      m_measureInterval;

   bool                       m_started;

   double                     m_lastTickTime;
   double                     m_accumulatedTime;

   static int                 s_refCount;
   static SimpleProfiler *    s_instance;

   SimpleProfiler();
   ~SimpleProfiler();

public:

   inline double              GetTime();
   inline int64               GetPerfCounterTime() const;
   inline int64               GetPerfCounterFreq() const { return m_freq.QuadPart; }

   void                       TickFrame();
   void                       Render( int x, int y );

   static SimpleProfiler &    Instance() { assert( s_instance != NULL ); return *s_instance; }

private:
   friend class SimpleProfilerObj;

   static void                RegisterProfObj( SimpleProfilerObj * me );
   static void                UnregisterProfObj( SimpleProfilerObj * me );

};

class SimpleProfilerObjScope
{
private:
   SimpleProfilerObj &     m_obj;
   SimpleProfilerObjScope& operator = ( const SimpleProfilerObjScope & other ) { assert(false); return *this; }
public:
   SimpleProfilerObjScope( SimpleProfilerObj & obj ) : m_obj(obj) { m_obj.Begin(); }
   ~SimpleProfilerObjScope( ) { m_obj.End(); }
};

#ifdef USE_SIMPLE_PROFILER

#define ProfileScope(x) \
static SimpleProfilerObj   SimpleProfilerObj_##x(#x); \
SimpleProfilerObjScope     SimpleProfilerObjScope_##x( SimpleProfilerObj_##x );

#else

#define ProfileScope(x) ;

#endif



///////////////////////////////////////////////////////////////////////////////////////////////////
// Inline

inline double SimpleProfiler::GetTime()
{
   LARGE_INTEGER time;

   BOOL res = QueryPerformanceCounter(&time);
   assert( res == TRUE );
   res;

   return time.QuadPart / (double) m_freq.QuadPart;
}

inline int64 SimpleProfiler::GetPerfCounterTime( ) const 
{
   LARGE_INTEGER time;
   BOOL res = QueryPerformanceCounter(&time);
   assert( res == TRUE );
   res;

   return time.QuadPart;
}

inline void SimpleProfilerObj::Begin()
{
   assert( m_beginTime == 0 );
   m_beginTime = SimpleProfiler::Instance().GetPerfCounterTime( );
}

inline void SimpleProfilerObj::End()
{
   assert( m_beginTime != 0 );

   int64 time = SimpleProfiler::Instance().GetPerfCounterTime();

   m_collectedTime += time - m_beginTime;
   m_collectedFrames++;

   m_beginTime = 0;
}

inline void SimpleProfilerObj::ResetAndRecalc()
{
   m_lastAvgTime = m_collectedTime / m_collectedFrames / (double)SimpleProfiler::Instance().GetPerfCounterFreq();
   m_collectedFrames = 0;
   m_collectedTime = 0;
}
