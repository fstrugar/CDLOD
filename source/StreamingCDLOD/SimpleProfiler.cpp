//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "SimpleProfiler.h"

#include <algorithm>

#include "DxCanvas.h"

#pragma warning ( disable: 4127 )   //warning C4127: conditional expression is constant

SimpleProfiler * SimpleProfiler::s_instance = NULL;
int SimpleProfiler::s_refCount = 0;

#ifdef USE_SIMPLE_PROFILER
// This one also keeps the singleton alive!! Don't remove it (or keep it alive in some other way)
static SimpleProfilerObj m_globalScope("<TotalAverageFrameTime>");
#endif

SimpleProfiler::SimpleProfiler()
{
   BOOL res = QueryPerformanceFrequency( &m_freq );
   assert(res == TRUE);
   res;

   m_measureInterval = 2.0f;
   m_started = false;
   m_lastTickTime = 0.0;
   m_accumulatedTime = 0.0;
}

SimpleProfiler::~SimpleProfiler()
{
   assert( m_profObjs.size() == 0 );
}

void SimpleProfiler::RegisterProfObj( SimpleProfilerObj * me )
{
   if( s_refCount == 0 )
   {
      SimpleProfiler::s_instance = new SimpleProfiler();
   }
   s_refCount++;
   SimpleProfiler::s_instance->m_profObjs.push_back( me );
}

void SimpleProfiler::UnregisterProfObj( SimpleProfilerObj * me )
{
   ProfObjsContainerType::iterator it = std::find( SimpleProfiler::s_instance->m_profObjs.begin(), SimpleProfiler::s_instance->m_profObjs.end(), me );
   if( it != SimpleProfiler::s_instance->m_profObjs.end() )
   {
      SimpleProfiler::s_instance->m_profObjs.erase( it );
   }

   s_refCount--;
   if( s_refCount == 0 )
   {
      delete SimpleProfiler::s_instance;
      SimpleProfiler::s_instance = NULL;
   }
}

void SimpleProfiler::TickFrame()
{
#ifdef USE_SIMPLE_PROFILER
   double currentTime = GetTime();
   if( m_started )
   {
      m_globalScope.End();

      m_accumulatedTime += currentTime - m_lastTickTime;
      if( m_accumulatedTime > m_measureInterval )
      {
         m_accumulatedTime -= m_measureInterval;
         if( m_accumulatedTime > m_measureInterval )
            m_accumulatedTime = 0.0;

         for( ProfObjsContainerType::iterator it = m_profObjs.begin(); it != m_profObjs.end(); it++ )
         {
            (*it)->ResetAndRecalc();
         }
      }
   }

   m_started = true;
   m_lastTickTime = currentTime;

   m_globalScope.Begin();
#endif
}

void SimpleProfiler::Render( int x, int y )
{
#ifdef USE_SIMPLE_PROFILER
   ICanvas2D * canvas2D = GetCanvas2D();

   int sx = 380;
   int sx2 = 310;

   int cy = y;
   canvas2D->DrawLine( (float)x, (float)cy, (float)x + sx, (float)cy, 0xFF000000 );
   cy += 1;

   for( ProfObjsContainerType::iterator it = m_profObjs.begin(); it != m_profObjs.end(); it++ )
   {
      const char * name = (*it)->GetName();
      float avgTime = (float)(*it)->GetLastAverageTimePerFrame();


      canvas2D->DrawString( x + 4, cy, 0xFF000040, name );
      canvas2D->DrawString( x + sx2, cy, 0xFF400000, "%.3fms", avgTime * 1000.0f );
      cy += 15;

      canvas2D->DrawLine( (float)x, (float)cy, (float)x + sx, (float)cy, 0xFF000000 );
      cy += 1;
   }

   canvas2D->DrawLine( (float)x, (float)y, (float)x, (float)cy, 0xFF000000 );
   canvas2D->DrawLine( (float)x+sx, (float)y, (float)x+sx, (float)cy, 0xFF000000 );

#endif
}

SimpleProfilerObj::SimpleProfilerObj( const char * name )
: m_name( name )
{
#ifndef USE_SIMPLE_PROFILER
   assert( false );
#endif

   m_beginTime = 0;
   m_collectedTime = 0;
   m_collectedFrames = 0;
   m_lastAvgTime = 0.0;
   SimpleProfiler::RegisterProfObj( this );
}

SimpleProfilerObj::~SimpleProfilerObj()
{
   SimpleProfiler::UnregisterProfObj( this );
}

