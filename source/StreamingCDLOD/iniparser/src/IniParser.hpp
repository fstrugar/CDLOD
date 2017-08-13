//////////////////////////////////////////////////////////////////////////
// Simple C++ wrapper of the iniparser by N. Devillard
// Author: Filip Strugar, 2009
//////////////////////////////////////////////////////////////////////////

#pragma once

extern "C" 
{
   #include "iniparser.h"
}


class IniParser
{
public:
   dictionary *      m_ini;

public:
   IniParser()                            { m_ini = NULL; }
   IniParser( const char * filePath );
   ~IniParser();
   //
   bool              isOpen( ) const                                    { return m_ini != NULL; }
   bool              Open( const char * filePath )                      { m_ini = iniparser_load( filePath ); return isOpen(); }
   bool              OpenBuffer( const char * buffer, int bufferSize )  { m_ini = iniparser_load_buffer( buffer, bufferSize ); return isOpen(); }
   void              Close( )                                           { if( m_ini != NULL ) iniparser_freedict( m_ini ); m_ini = NULL; }

   //
   const char *      getString( const char * key, const char * def ) const {  return iniparser_getstring(m_ini, key, def);  }
   double            getDouble( const char * key, double def ) const       {  return iniparser_getdouble(m_ini, key, def);  }
   float             getFloat( const char * key, float def ) const         {  return (float)iniparser_getdouble(m_ini, key, def);  }
   bool              getBool( const char * key, bool def ) const           {  return iniparser_getboolean(m_ini, key, def?1:0) != 0; }
   int               getInt( const char * key, int def ) const             {  return iniparser_getint(m_ini, key, def);  }

};