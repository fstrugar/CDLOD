//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "Common.h"

#include "DxGridMesh.h"

DxGridMesh::DxGridMesh(void)
{
   m_indexBuffer    = NULL;
   m_vertexBuffer   = NULL;
   m_dimension      = 0;
   m_indexEndTL = m_indexEndTR = m_indexEndBL = m_indexEndBR = 0;

   if( GetD3DDevice() != NULL )
      OnCreateDevice();
}

DxGridMesh::~DxGridMesh(void)
{
   OnDestroyDevice();
}

void DxGridMesh::SetDimensions( int dim )
{
   m_dimension = dim;
   
   OnDestroyDevice();
   OnCreateDevice();
}

HRESULT DxGridMesh::OnCreateDevice()
{
   if( GetD3DDevice() == NULL )
      return S_OK;

   if( m_dimension == 0 )
      return S_OK;

   HRESULT hr;
   IDirect3DDevice9 * pDevice = GetD3DDevice();

   assert( m_indexBuffer == NULL );
   assert( m_vertexBuffer == NULL );

   const int gridDim = m_dimension;

   int totalVertices = (gridDim+1) * (gridDim+1);
   assert( totalVertices <= 65535 );
   V( pDevice->CreateVertexBuffer( sizeof(PositionVertex) * totalVertices, 0, PositionVertex::FVF, D3DPOOL_MANAGED, &m_vertexBuffer, NULL ) );

   int totalIndices = gridDim * gridDim * 2 * 3;
   V( pDevice->CreateIndexBuffer( sizeof(unsigned short) * totalIndices, 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &m_indexBuffer, NULL ) );

   int vertDim = gridDim + 1;

   {
      // Make a grid of (gridDim+1) * (gridDim+1) vertices

      PositionVertex * vertices = NULL;
      V( m_vertexBuffer->Lock(0, sizeof(PositionVertex) * totalVertices, (void**)&vertices, 0 ) );

      for( int y = 0; y < vertDim; y++ )
         for( int x = 0; x < vertDim; x++ )
            vertices[x + vertDim * y] = PositionVertex( x / (float)(gridDim), y / (float)(gridDim), 0 );

      V( m_vertexBuffer->Unlock() );
   }

   {
      // Make indices for the gridDim * gridDim triangle grid, but make it as a combination of 4 subquads so that they
      // can be rendered separately when needed!

      unsigned short * indices = NULL;
      V( m_indexBuffer->Lock(0, sizeof(unsigned short) * totalIndices, (void**)&indices, 0 ) );

      int index = 0;

      int halfd = (vertDim/2);
      int fulld = gridDim;

      // Top left part
      for( int y = 0; y < halfd; y++ )
      {
         for( int x = 0; x < halfd; x++ )
         {
            indices[index++] = (unsigned short)(x + vertDim * y);         indices[index++] = (unsigned short)((x+1) + vertDim * y);     indices[index++] = (unsigned short)(x + vertDim * (y+1));
            indices[index++] = (unsigned short)((x+1) + vertDim * y);     indices[index++] = (unsigned short)((x+1) + vertDim * (y+1)); indices[index++] = (unsigned short)(x + vertDim * (y+1));
         }
      }
      m_indexEndTL = index;

      // Top right part
      for( int y = 0; y < halfd; y++ )
      {
         for( int x = halfd; x < fulld; x++ )
         {
            indices[index++] = (unsigned short)(x + vertDim * y);         indices[index++] = (unsigned short)((x+1) + vertDim * y);     indices[index++] = (unsigned short)(x + vertDim * (y+1));
            indices[index++] = (unsigned short)((x+1) + vertDim * y);     indices[index++] = (unsigned short)((x+1) + vertDim * (y+1)); indices[index++] = (unsigned short)(x + vertDim * (y+1));
         }
      }
      m_indexEndTR = index;

      // Bottom left part
      for( int y = halfd; y < fulld; y++ )
      {
         for( int x = 0; x < halfd; x++ )
         {
            indices[index++] = (unsigned short)(x + vertDim * y);         indices[index++] = (unsigned short)((x+1) + vertDim * y);     indices[index++] = (unsigned short)(x + vertDim * (y+1));
            indices[index++] = (unsigned short)((x+1) + vertDim * y);     indices[index++] = (unsigned short)((x+1) + vertDim * (y+1)); indices[index++] = (unsigned short)(x + vertDim * (y+1));
         }
      }
      m_indexEndBL = index;

      // Bottom right part
      for( int y = halfd; y < fulld; y++ )
      {
         for( int x = halfd; x < fulld; x++ )
         {
            indices[index++] = (unsigned short)(x + vertDim * y);         indices[index++] = (unsigned short)((x+1) + vertDim * y);     indices[index++] = (unsigned short)(x + vertDim * (y+1));
            indices[index++] = (unsigned short)((x+1) + vertDim * y);     indices[index++] = (unsigned short)((x+1) + vertDim * (y+1)); indices[index++] = (unsigned short)(x + vertDim * (y+1));
         }
      }
      m_indexEndBR = index;

      V( m_indexBuffer->Unlock() );
   }

   return S_OK;
}

void DxGridMesh::OnDestroyDevice()
{
   SAFE_RELEASE( m_indexBuffer );
   SAFE_RELEASE( m_vertexBuffer );
   m_indexEndTL = m_indexEndTR = m_indexEndBL = m_indexEndBR = 0;
}
