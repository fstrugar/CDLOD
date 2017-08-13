//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#pragma once

#include "DxEventNotifier.h"

#include <vector>
#include <string>

HRESULT        wsCompileVertexShader( IDirect3DDevice9 * device, DWORD dwShaderFlags, const wchar_t * filePath, const char * entryProc, ID3DXConstantTable ** outConstants, IDirect3DVertexShader9 ** outShader );
HRESULT        wsCompilePixelShader( IDirect3DDevice9 * device, DWORD dwShaderFlags, const wchar_t * filePath, const char * entryProc, ID3DXConstantTable ** outConstants, IDirect3DPixelShader9 ** outShader );

class DxShader : protected DxEventReceiver
{
   struct MacroPair
   {
      std::string      Name;
      std::string      Definition;
      MacroPair() {}
      MacroPair(const char * name, const char * definition) : Name(name), Definition(definition) { }
   };

protected:
   IUnknown *              m_shader;
   ID3DXConstantTable *    m_constantTable;

   std::string             m_path;
   std::string             m_entryPoint;

   static const int        c_maxMacros    =  64;
   std::vector<MacroPair>  m_macros;

public:
   DxShader();
   DxShader( const char * path, const char * entryPoint, D3DXMACRO * macroDefines = NULL );
   virtual ~DxShader();
   //
   void                       SetShaderInfo( const char * path, const char * entryPoint, D3DXMACRO * defines = NULL );
   //
   virtual void               Reload()                   { DestroyShader(); CreateShader(); }
   virtual void               Reload( D3DXMACRO * newMacroDefines );
   //
   ID3DXConstantTable *       GetConstantTable()         { return m_constantTable; }
   //
public:
   HRESULT                    SetBool( const char * name, bool val );
   //
   HRESULT                    SetFloat( const char * name, float val );
   HRESULT                    SetFloatArray( const char * name, float val0, float val1 );
   HRESULT                    SetFloatArray( const char * name, float val0, float val1, float val2 );
   HRESULT                    SetFloatArray( const char * name, float val0, float val1, float val2, float val3 );
   HRESULT                    SetFloatArray( const char * name, const float * valArray, int valArrayCount );
   //
   HRESULT                    SetVector( const char * name, const D3DXVECTOR4 & vec );
   HRESULT                    SetMatrix( const char * name, const D3DXMATRIX & matrix);

protected:
   static std::vector<DxShader *> & GetAllShadersList();

   void                       StoreMacros( CONST D3DXMACRO* pMacroDefines );
   const D3DXMACRO *          GetStoredMacros();   // returning temporary static storage, will be invalidated when called again in any other instance, or if StoreMacros() is called, or if the object is destructed

public:
   //
   static void                ReloadAllShaders();
   //
protected:
   virtual void               OnDestroyDevice();
   virtual HRESULT            OnCreateDevice();
   //
   virtual void               CreateShader()             = 0;
   void                       DestroyShader();
   //
};

class DxPixelShader : public DxShader
{
public:
   DxPixelShader()            {}
   DxPixelShader( const char * path, const char * entryPoint ) : DxShader( path, entryPoint ) { DxPixelShader::CreateShader(); }
   virtual ~DxPixelShader()   {}

public:
   IDirect3DPixelShader9 *    GetShader();

   operator IDirect3DPixelShader9 * () { return GetShader(); }

public:
   HRESULT                    SetTexture( const char * name, IDirect3DBaseTexture9 * texture,
                                                                                          DWORD addrU = D3DTADDRESS_CLAMP, 
                                                                                          DWORD addrV = D3DTADDRESS_CLAMP,
                                                                                          DWORD minFilter = D3DTEXF_POINT,
                                                                                          DWORD magFilter = D3DTEXF_POINT,
                                                                                          DWORD mipFilter = D3DTEXF_POINT );
   int                        GetTextureSamplerIndex( const char * name );

protected:
   virtual void               CreateShader();

};

class DxVertexShader : public DxShader
{
public:
   DxVertexShader()           {}
   DxVertexShader( const char * path, const char * entryPoint ) : DxShader( path, entryPoint ) { DxVertexShader::CreateShader(); }
   virtual ~DxVertexShader()  {}

public:
   IDirect3DVertexShader9 *   GetShader();

   operator IDirect3DVertexShader9 * () { return GetShader(); }

public:
   HRESULT                    SetTexture( const char * name, IDirect3DTexture9 * texture,
                                                                                          DWORD addrU = D3DTADDRESS_CLAMP, 
                                                                                          DWORD addrV = D3DTADDRESS_CLAMP,
                                                                                          DWORD minFilter = D3DTEXF_POINT,
                                                                                          DWORD magFilter = D3DTEXF_POINT,
                                                                                          DWORD mipFilter = D3DTEXF_POINT );


protected:
   virtual void               CreateShader();

};
