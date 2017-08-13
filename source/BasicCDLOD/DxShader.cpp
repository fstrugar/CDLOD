//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "Common.h"

#include "DxShader.h"

using namespace std;

const char *               g_VertexShaderProfile   = "vs_3_0";
const char *               g_PixelShaderProfile    = "ps_3_0";

//
HRESULT wsCompileVertexShader( IDirect3DDevice9 * device, DWORD dwShaderFlags, const wchar_t * filePath, const char * entryProc, CONST D3DXMACRO* pMacroDefines, ID3DXConstantTable ** outConstants, IDirect3DVertexShader9 ** outShader )
{
   LPD3DXBUFFER pCode = NULL;
   LPD3DXBUFFER pErrorMsg = NULL;

   HRESULT hr;

#if SHADER_LOAD_FROM_EMBEDDED
   int builtInIndex = shadersFindFile( vaStringSimpleNarrow(filePath).c_str() );
   if( builtInIndex != -1 )
   {
      int dataSize = shadersGetDataSize( builtInIndex );
      hr = D3DXCompileShader( (LPCSTR)shadersGetData( builtInIndex ), dataSize-1, pMacroDefines, &s_dxIncludeHandler, entryProc, g_VertexShaderProfile, dwShaderFlags, &pCode, &pErrorMsg, outConstants );
      hr = device->CreateVertexShader( ( DWORD* )pCode->GetBufferPointer(), outShader );
      pCode->Release();
      return hr;
   }
#endif

retryCompileVS:

   hr = D3DXCompileShaderFromFile( filePath, pMacroDefines, NULL, entryProc, g_VertexShaderProfile, dwShaderFlags, &pCode, &pErrorMsg, outConstants );
   if( FAILED( hr ) )
   {
      wprintf( L"\n" );
      wprintf( (const wchar_t *)pErrorMsg->GetBufferPointer() );
      wprintf( L"\n" );
      pErrorMsg->Release();
      if( MessageBox( DXUTGetHWND(), L"Error while compiling vertex shader, retry?", L"Shader error", MB_YESNO | MB_SETFOREGROUND ) == IDYES )
         goto retryCompileVS;
      return hr;
   }

retryCreateVS:

   hr = device->CreateVertexShader( ( DWORD* )pCode->GetBufferPointer(), outShader );
   pCode->Release();
   if( FAILED( hr ) )
   {
      if( MessageBox( DXUTGetHWND(), L"Error while creating vertex shader, retry?", L"Shader error", MB_YESNO | MB_SETFOREGROUND ) == IDYES )
         goto retryCreateVS;
      return hr;
   }

   return S_OK;
}

HRESULT wsCompilePixelShader( IDirect3DDevice9 * device, DWORD dwShaderFlags, const wchar_t * filePath, const char * entryProc, CONST D3DXMACRO* pMacroDefines, ID3DXConstantTable ** outConstants, IDirect3DPixelShader9 ** outShader )
{
   LPD3DXBUFFER pCode = NULL;
   LPD3DXBUFFER pErrorMsg = NULL;

   HRESULT hr;

#if SHADER_LOAD_FROM_EMBEDDED
   int builtInIndex = shadersFindFile( vaStringSimpleNarrow(filePath).c_str() );
   if( builtInIndex != -1 )
   {
      int dataSize = shadersGetDataSize( builtInIndex );
      hr = D3DXCompileShader( (LPCSTR)shadersGetData( builtInIndex ), dataSize, pMacroDefines, &s_dxIncludeHandler, entryProc, g_PixelShaderProfile, dwShaderFlags, &pCode, &pErrorMsg, outConstants );
      hr = device->CreatePixelShader( ( DWORD* )pCode->GetBufferPointer(), outShader );
      pCode->Release();
      return hr;
   }
#endif

retryCompilePS:

   hr = D3DXCompileShaderFromFile( filePath, pMacroDefines, NULL, entryProc, g_PixelShaderProfile, dwShaderFlags, &pCode, &pErrorMsg, outConstants );
   if( FAILED( hr ) )
   {
      wprintf( L"\n" );
      wprintf( (const wchar_t *)pErrorMsg->GetBufferPointer() );
      wprintf( L"\n" );
      pErrorMsg->Release();
      if( MessageBox( DXUTGetHWND(), L"Error while creating pixel shader, retry?", L"Shader error", MB_YESNO | MB_SETFOREGROUND ) == IDYES )
         goto retryCompilePS;
      return hr;
   }

retryCreatePS:

   hr = device->CreatePixelShader( ( DWORD* )pCode->GetBufferPointer(), outShader );
   pCode->Release();
   if( FAILED( hr ) )
   {
      if( MessageBox( DXUTGetHWND(), L"Error while creating pixel shader, retry?", L"Shader error", MB_YESNO | MB_SETFOREGROUND ) == IDYES )
         goto retryCreatePS;
      return hr; //DXTRACE_ERR( TEXT( "CreatePixelShader" ), hr );
   }

   return S_OK;
}
//
DxShader::DxShader()
{
   m_shader          = NULL;
   m_constantTable   = NULL;
   m_path            = "";
   m_entryPoint      = "";

   GetAllShadersList().push_back(this);
}
//
DxShader::DxShader( const char * path, const char * entryPoint, D3DXMACRO * defines )
{
   m_shader          = NULL;
   m_constantTable   = NULL;
   m_path            = path;
   m_entryPoint      = entryPoint;

   StoreMacros( defines );

   GetAllShadersList().push_back(this);
}
//
DxShader::~DxShader()
{
   DestroyShader();

   for( size_t i = 0; i < GetAllShadersList().size(); i++ )
   {
      if( GetAllShadersList()[i] == this )
      {
         GetAllShadersList().erase( GetAllShadersList().begin() + i );
         break;
      }
   }
}
//
std::vector<DxShader *> & DxShader::GetAllShadersList()
{
   static std::vector<DxShader *> list;
   return list;
}
//
void DxShader::StoreMacros( CONST D3DXMACRO* pMacroDefines )
{
   ::erase( m_macros );

   if( pMacroDefines == NULL )
      return;

   CONST D3DXMACRO* pNext = pMacroDefines;
   while( pNext->Name != NULL && pNext->Definition != NULL )
   {
      m_macros.push_back( MacroPair(pNext->Name, pNext->Definition) );
      pNext++;

      if( m_macros.size() > c_maxMacros-1 )
      {
         assert( false ); // no more than c_maxMacros-1 supported, increase c_maxMacros
         break;
      }
   }
}
//
const D3DXMACRO * DxShader::GetStoredMacros( )
{
   if( m_macros.size() == 0 )
      return NULL;

   static D3DXMACRO tmpBuffer[c_maxMacros];
   size_t i;
   for( i = 0; i < m_macros.size(); i++ )
   {
      tmpBuffer[i].Definition = m_macros[i].Definition.c_str();
      tmpBuffer[i].Name       = m_macros[i].Name.c_str();
   }
   tmpBuffer[i].Definition = NULL;
   tmpBuffer[i].Name = NULL;

   return tmpBuffer;
}
//
void DxShader::ReloadAllShaders()
{
   for( size_t i = 0; i < GetAllShadersList().size(); i++ )
      GetAllShadersList()[i]->Reload();
}
//
void DxShader::SetShaderInfo( const char * path, const char * entryPoint, D3DXMACRO * defines )
{
   DestroyShader();
   m_path         = path;
   m_entryPoint   = entryPoint;
   StoreMacros( defines );
   CreateShader();
}
//
void DxShader::Reload( D3DXMACRO * newMacroDefines )
{
   StoreMacros( newMacroDefines );
   Reload();
}
//
void DxShader::OnDestroyDevice()
{
   DestroyShader();
}
//
void DxShader::DestroyShader()
{
   SAFE_RELEASE( m_shader );
   SAFE_RELEASE( m_constantTable );
}
//
HRESULT DxShader::OnCreateDevice()
{
   CreateShader();

   return S_OK;
}
//
IDirect3DPixelShader9 * DxPixelShader::GetShader()
{
   if(m_shader == NULL) 
      return NULL; 

   HRESULT hr;
   IDirect3DPixelShader9 * ret;
   V( m_shader->QueryInterface( IID_IDirect3DPixelShader9, (void**)&ret ) );
   ret->Release();

   return ret;
}
//
IDirect3DVertexShader9 * DxVertexShader::GetShader()
{
   if(m_shader == NULL) 
      return NULL; 

   HRESULT hr;
   IDirect3DVertexShader9 * ret;
   V( m_shader->QueryInterface( IID_IDirect3DVertexShader9, (void**)&ret ) );
   ret->Release();

   return ret;
}
//
void DxPixelShader::CreateShader()
{
   HRESULT hr;
   IDirect3DDevice9 * device = GetD3DDevice();
   if( device == NULL || m_path.size() == 0 )
      return;

   DWORD dwShaderFlags = 0;
#ifdef DEBUG_PS
   dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
#endif

#if _DEBUG
   dwShaderFlags |= D3DXSHADER_DEBUG;
#endif

#if SHADER_LOAD_FROM_EMBEDDED
   wstring path = vaStringSimpleWiden( m_path );
#else
   wstring path = vaStringSimpleWiden( vaFindResource(m_path) );
#endif

   IDirect3DPixelShader9 * shader;
   V( wsCompilePixelShader( device, dwShaderFlags, path.c_str(), m_entryPoint.c_str(), GetStoredMacros(), &m_constantTable, &shader ) );

   V( shader->QueryInterface( IID_IUnknown, (void**)&m_shader ) );
   shader->Release();
}
//
void DxVertexShader::CreateShader()
{
   HRESULT hr;
   IDirect3DDevice9 * device = GetD3DDevice();
   if( device == NULL || m_path.size() == 0 )
      return;

   DWORD dwShaderFlags = 0;
#ifdef DEBUG_VS
   dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
#endif

#if _DEBUG
   dwShaderFlags |= D3DXSHADER_DEBUG;
#endif

#if SHADER_LOAD_FROM_EMBEDDED
   wstring path = vaStringSimpleWiden( m_path );
#else
   wstring path = vaStringSimpleWiden( vaFindResource(m_path) );
#endif


   IDirect3DVertexShader9 * shader;
   V( wsCompileVertexShader( device, dwShaderFlags, path.c_str(), m_entryPoint.c_str(), GetStoredMacros(), &m_constantTable, &shader ) );

   V( shader->QueryInterface( IID_IUnknown, (void**)&m_shader ) );
   shader->Release();
}
//
//
HRESULT DxShader::SetBool( const char * name, bool val )
{
   return m_constantTable->SetBool( GetD3DDevice(), name, val );
}
//
HRESULT DxShader::SetFloat( const char * name, float val )
{
   return m_constantTable->SetFloat( GetD3DDevice(), name, val );
}
//
HRESULT DxShader::SetFloatArray( const char * name, float val0, float val1 )
{
   float v[2] = { val0, val1 };
   return m_constantTable->SetFloatArray( GetD3DDevice(), name, v, 2 );
}
//
HRESULT DxShader::SetFloatArray( const char * name, float val0, float val1, float val2 )
{
   float v[3] = { val0, val1, val2 };
   return m_constantTable->SetFloatArray( GetD3DDevice(), name, v, 3 );
}
//
HRESULT DxShader::SetFloatArray( const char * name, float val0, float val1, float val2, float val3 )
{
   float v[4] = { val0, val1, val2, val3 };
   return m_constantTable->SetFloatArray( GetD3DDevice(), name, v, 4 );
}
//
HRESULT DxShader::SetFloatArray( const char * name, const float * valArray, int valArrayCount )
{
   return m_constantTable->SetFloatArray( GetD3DDevice(), name, valArray, valArrayCount );
}
//
HRESULT DxShader::SetVector( const char * name, const D3DXVECTOR4 & vec )
{
   return m_constantTable->SetVector( GetD3DDevice(), name, &vec );
}
//
HRESULT DxShader::SetMatrix( const char * name, const D3DXMATRIX & matrix)
{
   return m_constantTable->SetMatrix( GetD3DDevice(), name, &matrix );
}
//
HRESULT DxPixelShader::SetTexture( const char * name, IDirect3DBaseTexture9 * texture, DWORD addrU, DWORD addrV, DWORD minFilter, DWORD magFilter, DWORD mipFilter )
{
   IDirect3DDevice9 * device = GetD3DDevice();

   int samplerIndex = m_constantTable->GetSamplerIndex( name );
   if( samplerIndex != -1 )
   {
      device->SetTexture( samplerIndex, texture );
      device->SetSamplerState( samplerIndex, D3DSAMP_ADDRESSU, addrU );
      device->SetSamplerState( samplerIndex, D3DSAMP_ADDRESSV, addrV );
      device->SetSamplerState( samplerIndex, D3DSAMP_MINFILTER, minFilter );
      device->SetSamplerState( samplerIndex, D3DSAMP_MAGFILTER, magFilter );
      device->SetSamplerState( samplerIndex, D3DSAMP_MIPFILTER, mipFilter );
      return S_OK;
   }
   return E_FAIL;
}
//
int DxPixelShader::GetTextureSamplerIndex( const char * name )
{
   return m_constantTable->GetSamplerIndex( name );
}
//
HRESULT DxVertexShader::SetTexture( const char * name, IDirect3DTexture9 * texture, DWORD addrU, DWORD addrV, DWORD minFilter, DWORD magFilter, DWORD mipFilter )
{
   IDirect3DDevice9 * device = GetD3DDevice();

   int samplerIndex = m_constantTable->GetSamplerIndex( name );
   if( samplerIndex != -1 )
   {
      device->SetTexture( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, texture );
      device->SetSamplerState( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, D3DSAMP_ADDRESSU, addrU );
      device->SetSamplerState( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, D3DSAMP_ADDRESSV, addrV );
      device->SetSamplerState( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, D3DSAMP_MINFILTER, minFilter );
      device->SetSamplerState( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, D3DSAMP_MAGFILTER, magFilter );
      device->SetSamplerState( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, D3DSAMP_MIPFILTER, mipFilter );
      return S_OK;
   }
   return E_FAIL;
}
//