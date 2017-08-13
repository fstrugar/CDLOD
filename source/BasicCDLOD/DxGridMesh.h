//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#pragma once

#include "DxEventNotifier.h"

class DxGridMesh : protected DxEventReceiver
{
private:
   IDirect3DIndexBuffer9 *    m_indexBuffer;
   IDirect3DVertexBuffer9 *   m_vertexBuffer;
   int                        m_dimension;
   int                        m_indexEndTL;
   int                        m_indexEndTR;
   int                        m_indexEndBL;
   int                        m_indexEndBR;

public:
   DxGridMesh(void);
   ~DxGridMesh(void);
   //
   void                             SetDimensions( int dim );
   int                              GetDimensions() const         { return m_dimension; }
   //
   const IDirect3DIndexBuffer9 *    GetIndexBuffer() const        { return m_indexBuffer; }
   const IDirect3DVertexBuffer9 *   GetVertexBuffer() const       { return m_vertexBuffer; }
   int                              GetIndexEndTL() const         { return m_indexEndTL; }
   int                              GetIndexEndTR() const         { return m_indexEndTR; }
   int                              GetIndexEndBL() const         { return m_indexEndBL; }
   int                              GetIndexEndBR() const         { return m_indexEndBR; }
   //
private:
   //
   virtual HRESULT 				      OnCreateDevice();
   virtual void                     OnDestroyDevice();
};
