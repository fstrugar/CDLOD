//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "CDLODQuadTree.h"

#include "DxCanvas.h"

#include "DemoCamera.h"

#ifdef MY_EXTENDED_STUFF
#include "IProf/prof.h"
#endif

CDLODQuadTree::CDLODQuadTree()
{
   m_allNodesBuffer  = NULL;
   m_topLevelNodes   = NULL;
}
//
CDLODQuadTree::~CDLODQuadTree()
{
   Clean();
}
//
bool CDLODQuadTree::Create( const CreateDesc & desc )
{
   Clean();

   m_desc         = desc;
   m_rasterSizeX  = desc.pHeightmap->GetSizeX();
   m_rasterSizeY  = desc.pHeightmap->GetSizeY();

   if( m_rasterSizeX > 65535 || m_rasterSizeY > 65535 )
   {
      assert( false );
      return false;
   }
   if( m_desc.LODLevelCount > c_maxLODLevels )
   {
      assert( false );
      return false;
   }

   //////////////////////////////////////////////////////////////////////////
   // Determine how many nodes will we use, and the size of the top (root) tree
   // node.
   //
   m_leafNodeWorldSizeX = desc.LeafRenderNodeSize * desc.MapDims.SizeX / (float)m_rasterSizeX;
   m_leafNodeWorldSizeY = desc.LeafRenderNodeSize * desc.MapDims.SizeY / (float)m_rasterSizeY;
   m_LODLevelNodeDiagSizes[0] = sqrtf(m_leafNodeWorldSizeX * m_leafNodeWorldSizeX + m_leafNodeWorldSizeY * m_leafNodeWorldSizeY);
   //
   int totalNodeCount = 0;
   //
   m_topNodeSize = desc.LeafRenderNodeSize;
   for( int i = 0; i < m_desc.LODLevelCount; i++ )
   {

      if( i != 0 ) 
      {
         m_topNodeSize *= 2;
         m_LODLevelNodeDiagSizes[i] = 2 * m_LODLevelNodeDiagSizes[i-1];
      }

      int nodeCountX = (m_rasterSizeX-1) / m_topNodeSize + 1;
      int nodeCountY = (m_rasterSizeY-1) / m_topNodeSize + 1;

      totalNodeCount += (nodeCountX) * (nodeCountY);
   }
   //////////////////////////////////////////////////////////////////////////

   //////////////////////////////////////////////////////////////////////////
   // Initialize the tree memory, create tree nodes, and extract min/max Zs (heights)
   //
   m_allNodesBuffer = new Node[totalNodeCount];
   int nodeCounter = 0;
   //
   m_topNodeCountX = (m_rasterSizeX-1) / m_topNodeSize + 1;
   m_topNodeCountY = (m_rasterSizeY-1) / m_topNodeSize + 1;
   m_topLevelNodes = new Node**[m_topNodeCountY];
   for( int y = 0; y < m_topNodeCountY; y++ )
   {
      m_topLevelNodes[y] = new Node*[m_topNodeCountX];
      for( int x = 0; x < m_topNodeCountX; x++ )
      {
         m_topLevelNodes[y][x] = &m_allNodesBuffer[nodeCounter];
         nodeCounter++;

         m_topLevelNodes[y][x]->Create( x * m_topNodeSize, y * m_topNodeSize, m_topNodeSize, 0, m_desc, m_allNodesBuffer, nodeCounter );
      }
   }
   m_allNodesCount = nodeCounter;
   assert( nodeCounter == totalNodeCount );

   int sizeInMemory = totalNodeCount * sizeof(Node);
   printf( "CDLODQuadTree created, size in memory: ~%.2fKb", sizeInMemory / 1024.0f );
   //////////////////////////////////////////////////////////////////////////

   return true;
}
//
void CDLODQuadTree::Node::Create( int x, int y, int size, int level, const CreateDesc & createDesc, Node * allNodesBuffer, int & allNodesBufferLastIndex )
{
   this->X     = (unsigned short)x;       assert( x >= 0 && x < 65535 );
   this->Y     = (unsigned short)y;       assert( y >= 0 && y < 65535 );
   this->Level = (unsigned short)level;   assert( level >= 0 && level < 16 );
   this->Size  = (unsigned short)size;    assert( size >= 0 && level < 32768 );

   int rasterSizeX = createDesc.pHeightmap->GetSizeX();
   int rasterSizeY = createDesc.pHeightmap->GetSizeY();

   //this->WorldMinX   = createDesc.MapDims.MinX + (x / (float)(rasterSizeX-1)) * createDesc.MapDims.SizeX;
   //this->WorldMinY   = createDesc.MapDims.MinY + (y / (float)(rasterSizeY-1)) * createDesc.MapDims.SizeY;
   //this->WorldMaxX   = createDesc.MapDims.MinX + ((x+size) / (float)(rasterSizeX-1)) * createDesc.MapDims.SizeX;
   //this->WorldMaxY   = createDesc.MapDims.MinY + ((y+size) / (float)(rasterSizeY-1)) * createDesc.MapDims.SizeY;

   this->SubTL = NULL;
   this->SubTR = NULL;
   this->SubBL = NULL;
   this->SubBR = NULL;

   // Are we done yet?
   if( size == (createDesc.LeafRenderNodeSize) )
   {
      assert( level == (createDesc.LODLevelCount-1) );

      // Mark leaf node!
      Level |= 0x8000;

      // Find min/max heights at this patch of terrain
      int limitX = ::min( rasterSizeX, x + size + 1 );
      int limitY = ::min( rasterSizeY, y + size + 1 );
      createDesc.pHeightmap->GetAreaMinMaxZ( x, y, limitX - x, limitY - y, this->MinZ, this->MaxZ );

      //// Convert to world space...
      //this->WorldMinZ = createDesc.MapDims.MinZ + this->MinZ * createDesc.MapDims.SizeZ / 65535.0f;
      //this->WorldMaxZ = createDesc.MapDims.MinZ + this->MaxZ * createDesc.MapDims.SizeZ / 65535.0f;

      // Find heights for 4 corner points (used for approx ray casting)
      // (reuse otherwise empty pointers used for sub nodes)
      {
         float * pTLZ = (float *)&this->SubTL;
         float * pTRZ = (float *)&this->SubTR;
         float * pBLZ = (float *)&this->SubBL;
         float * pBRZ = (float *)&this->SubBR;

         int limitX = ::min( rasterSizeX - 1, x + size );
         int limitY = ::min( rasterSizeY - 1, y + size );
         *pTLZ = createDesc.MapDims.MinZ + createDesc.pHeightmap->GetHeightAt( x, y ) * createDesc.MapDims.SizeZ / 65535.0f;
         *pTRZ = createDesc.MapDims.MinZ + createDesc.pHeightmap->GetHeightAt( limitX, y ) * createDesc.MapDims.SizeZ / 65535.0f;
         *pBLZ = createDesc.MapDims.MinZ + createDesc.pHeightmap->GetHeightAt( x, limitY ) * createDesc.MapDims.SizeZ / 65535.0f;
         *pBRZ = createDesc.MapDims.MinZ + createDesc.pHeightmap->GetHeightAt( limitX, limitY ) * createDesc.MapDims.SizeZ / 65535.0f;
      }
   }
   else
   {
      int subSize = size / 2;

      this->SubTL = &allNodesBuffer[allNodesBufferLastIndex++];
      this->SubTL->Create( x, y, subSize, level+1, createDesc, allNodesBuffer, allNodesBufferLastIndex );
      this->MinZ = this->SubTL->MinZ;
      this->MaxZ = this->SubTL->MaxZ;
      //this->WorldMinZ = this->SubTL->WorldMinZ;
      //this->WorldMaxZ = this->SubTL->WorldMaxZ;

      if( (x + subSize) < rasterSizeX )
      {
         this->SubTR = &allNodesBuffer[allNodesBufferLastIndex++];
         this->SubTR->Create( x + subSize, y, subSize, level+1, createDesc, allNodesBuffer, allNodesBufferLastIndex );
         this->MinZ = ::min( this->MinZ, this->SubTR->MinZ );
         this->MaxZ = ::max( this->MaxZ, this->SubTR->MaxZ );
         //this->WorldMinZ = ::min( this->WorldMinZ, this->SubTR->WorldMinZ );
         //this->WorldMaxZ = ::max( this->WorldMaxZ, this->SubTR->WorldMaxZ );
      }

      if( (y + subSize) < rasterSizeY )
      {
         this->SubBL = &allNodesBuffer[allNodesBufferLastIndex++];
         this->SubBL->Create( x, y + subSize, subSize, level+1, createDesc, allNodesBuffer, allNodesBufferLastIndex );
         this->MinZ = ::min( this->MinZ, this->SubBL->MinZ );
         this->MaxZ = ::max( this->MaxZ, this->SubBL->MaxZ );
         //this->WorldMinZ = ::min( this->WorldMinZ, this->SubBL->WorldMinZ );
         //this->WorldMaxZ = ::max( this->WorldMaxZ, this->SubBL->WorldMaxZ );
      }

      if( ((x + subSize) < rasterSizeX) && ((y + subSize) < rasterSizeY) )
      {
         this->SubBR = &allNodesBuffer[allNodesBufferLastIndex++];
         this->SubBR->Create( x + subSize, y + subSize, subSize, level+1, createDesc, allNodesBuffer, allNodesBufferLastIndex );
         this->MinZ = ::min( this->MinZ, this->SubBR->MinZ );
         this->MaxZ = ::max( this->MaxZ, this->SubBR->MaxZ );
         //this->WorldMinZ = ::min( this->WorldMinZ, this->SubBR->WorldMinZ );
         //this->WorldMaxZ = ::max( this->WorldMaxZ, this->SubBR->WorldMaxZ );
      }
   }
}

void CDLODQuadTree::Node::DebugDrawAABB( unsigned int penColor, const CDLODQuadTree & quadTree ) const
{
   AABB boundingBox;
   GetAABB( boundingBox, quadTree.m_rasterSizeX, quadTree.m_rasterSizeY, quadTree.m_desc.MapDims );
   GetCanvas3D()->DrawBox( boundingBox.Min, boundingBox.Max, penColor );
}

CDLODQuadTree::Node::LODSelectResult CDLODQuadTree::Node::LODSelect( LODSelectInfo & lodSelectInfo, bool parentCompletelyInFrustum ) const
{
   AABB boundingBox;
   GetAABB( boundingBox, lodSelectInfo.RasterSizeX, lodSelectInfo.RasterSizeY, lodSelectInfo.MapDims );

   //D3DXVECTOR3 boundingSphereCenter;
   //float       boundingSphereRadiusSq;
   //GetBSphere( boundingSphereCenter, boundingSphereRadiusSq, quadTree );

   const D3DXPLANE * frustumPlanes  = lodSelectInfo.SelectionObj->m_frustumPlanes;
   const D3DXVECTOR3 & observerPos  = lodSelectInfo.SelectionObj->m_observerPos;
   const int maxSelectionCount      = lodSelectInfo.SelectionObj->m_maxSelectionCount;
   float * lodRanges                = lodSelectInfo.SelectionObj->m_visibilityRanges;

   IntersectType frustumIt = (parentCompletelyInFrustum)?(IT_Inside):(boundingBox.TestInBoundingPlanes( frustumPlanes ));
   if( frustumIt == IT_Outside )
      return IT_OutOfFrustum;

   float distanceLimit = lodRanges[this->GetLevel()];

   if( !boundingBox.IntersectSphereSq(observerPos, distanceLimit*distanceLimit) )
      return IT_OutOfRange;

   LODSelectResult SubTLSelRes = IT_Undefined;
   LODSelectResult SubTRSelRes = IT_Undefined;
   LODSelectResult SubBLSelRes = IT_Undefined;
   LODSelectResult SubBRSelRes = IT_Undefined;

   if( this->GetLevel() != lodSelectInfo.StopAtLevel )
   {
      float nextDistanceLimit = lodRanges[this->GetLevel()+1];
      if( boundingBox.IntersectSphereSq( observerPos, nextDistanceLimit*nextDistanceLimit ) )
      {
         bool weAreCompletelyInFrustum = frustumIt == IT_Inside;

         if( SubTL != NULL ) SubTLSelRes = this->SubTL->LODSelect( lodSelectInfo, weAreCompletelyInFrustum );
         if( SubTR != NULL ) SubTRSelRes = this->SubTR->LODSelect( lodSelectInfo, weAreCompletelyInFrustum );
         if( SubBL != NULL ) SubBLSelRes = this->SubBL->LODSelect( lodSelectInfo, weAreCompletelyInFrustum );
         if( SubBR != NULL ) SubBRSelRes = this->SubBR->LODSelect( lodSelectInfo, weAreCompletelyInFrustum );
      }
   }

   // We don't want to select sub nodes that are invisible (out of frustum) or are selected;
   // (we DO want to select if they are out of range, since we are not)
   bool bRemoveSubTL = (SubTLSelRes == IT_OutOfFrustum) || (SubTLSelRes == IT_Selected);
   bool bRemoveSubTR = (SubTRSelRes == IT_OutOfFrustum) || (SubTRSelRes == IT_Selected);
   bool bRemoveSubBL = (SubBLSelRes == IT_OutOfFrustum) || (SubBLSelRes == IT_Selected);
   bool bRemoveSubBR = (SubBRSelRes == IT_OutOfFrustum) || (SubBRSelRes == IT_Selected);

   assert( lodSelectInfo.SelectionCount < maxSelectionCount );
   if( !(bRemoveSubTL && bRemoveSubTR && bRemoveSubBL && bRemoveSubBR) && (lodSelectInfo.SelectionCount < maxSelectionCount) )
   {
      int LODLevel = lodSelectInfo.StopAtLevel - this->GetLevel();
      lodSelectInfo.SelectionObj->m_selectionBuffer[lodSelectInfo.SelectionCount++] = SelectedNode( this, LODLevel, !bRemoveSubTL, !bRemoveSubTR, !bRemoveSubBL, !bRemoveSubBR );

      // This should be calculated somehow better, but brute force will work for now
      if( 
#ifndef _DEBUG
         !lodSelectInfo.SelectionObj->m_visDistTooSmall && 
#endif
         (this->GetLevel() != 0) )
      {
         float maxDistFromCam = sqrtf( boundingBox.MaxDistanceFromPointSq( observerPos ) );

         float morphStartRange = lodSelectInfo.SelectionObj->m_morphStart[lodSelectInfo.StopAtLevel - this->GetLevel() + 1 ];

         if( maxDistFromCam > morphStartRange )
         {
            lodSelectInfo.SelectionObj->m_visDistTooSmall = true;
#ifdef _DEBUG
            GetCanvas3D()->DrawBox( boundingBox.Min, boundingBox.Max, 0xFFFF0000, 0x40FF0000 );
#endif
         }
      }
      
      return IT_Selected;
   }

   // if any of child nodes are selected, then return selected - otherwise all of them are out of frustum, so we're out of frustum too
   if( (SubTLSelRes == IT_Selected) || (SubTRSelRes == IT_Selected) || (SubBLSelRes == IT_Selected) || (SubBRSelRes == IT_Selected) )
      return IT_Selected;
   else
      return IT_OutOfFrustum;
}

void CDLODQuadTree::Clean( )
{
   if( m_allNodesBuffer != NULL )
      delete[] m_allNodesBuffer;
   m_allNodesBuffer = NULL;

   if( m_topLevelNodes != NULL )
   {
      for( int y = 0; y < m_topNodeCountY; y++ )
         delete[] m_topLevelNodes[y];

	  delete[] m_topLevelNodes;
      m_topLevelNodes = NULL;
   }
}

void CDLODQuadTree::DebugDrawAllNodes( ) const
{
   for( int i = 0; i < m_allNodesCount; i++ )
      if( m_allNodesBuffer[i].GetLevel() != 0 ) m_allNodesBuffer[i].DebugDrawAABB( 0xFF00FF00, *this );
   for( int i = 0; i < m_allNodesCount; i++ )
      if( m_allNodesBuffer[i].GetLevel() == 0 ) m_allNodesBuffer[i].DebugDrawAABB( 0xFFFF0000, *this );
}

int compare_closerFirst( const void *arg1, const void *arg2 )
{
   const CDLODQuadTree::SelectedNode * a = (const CDLODQuadTree::SelectedNode *)arg1;
   const CDLODQuadTree::SelectedNode * b = (const CDLODQuadTree::SelectedNode *)arg2;

   return a->MinDistToCamera > b->MinDistToCamera;
}
//
void CDLODQuadTree::LODSelect( LODSelection * selectionObj ) const
{
#ifdef MY_EXTENDED_STUFF
   Prof(DLODQuadTree_LODSelect);
#endif

   const D3DXVECTOR3 & cameraPos    = selectionObj->m_observerPos;
   const float visibilityDistance   = selectionObj->m_visibilityDistance;
   const int   layerCount           = m_desc.LODLevelCount;

   float LODNear           = 0;
   float LODFar            = visibilityDistance;
   float detailBalance     = selectionObj->m_LODDistanceRatio;

   float total                = 0;
   float currentDetailBalance = 1.0f;

   selectionObj->m_quadTree   = this;
   selectionObj->m_visDistTooSmall = false;

   assert( layerCount <= c_maxLODLevels );

   for (int i=0; i<layerCount; i++) 
   {
      total += currentDetailBalance;
      currentDetailBalance *= detailBalance;
   }

   float sect = (LODFar - LODNear) / total;

   float prevPos = LODNear;
   currentDetailBalance = 1.0f;
   for( int i = 0; i < layerCount; i++ ) 
   {
      selectionObj->m_visibilityRanges[layerCount - i - 1] = prevPos + sect * currentDetailBalance;
      prevPos = selectionObj->m_visibilityRanges[layerCount - i - 1];
      currentDetailBalance *= detailBalance;
   }

   prevPos = LODNear;
   for (int i=0; i<layerCount; i++)
   {
      int index = layerCount - i - 1;
      selectionObj->m_morphEnd[i] = selectionObj->m_visibilityRanges[index];
      selectionObj->m_morphStart[i] = prevPos + (selectionObj->m_morphEnd[i] - prevPos) * selectionObj->m_morphStartRatio;

      prevPos = selectionObj->m_morphStart[i];
   }

   Node::LODSelectInfo lodSelInfo;
   lodSelInfo.RasterSizeX     = m_rasterSizeX;
   lodSelInfo.RasterSizeY     = m_rasterSizeY;
   lodSelInfo.MapDims         = m_desc.MapDims;
   lodSelInfo.SelectionCount  = 0;
   lodSelInfo.SelectionObj    = selectionObj;
   lodSelInfo.StopAtLevel     = layerCount-1;

   for( int y = 0; y < m_topNodeCountY; y++ )
      for( int x = 0; x < m_topNodeCountX; x++ )
      {
         m_topLevelNodes[y][x]->LODSelect( lodSelInfo, false );
      }

   selectionObj->m_maxSelectedLODLevel = 0;
   selectionObj->m_minSelectedLODLevel = c_maxLODLevels;

   for( int i = 0; i < lodSelInfo.SelectionCount; i++ )
   {
      AABB naabb;
      selectionObj->m_selectionBuffer[i].GetAABB(naabb, m_rasterSizeX, m_rasterSizeY, m_desc.MapDims);

      if( selectionObj->m_sortByDistance )
         selectionObj->m_selectionBuffer[i].MinDistToCamera = sqrtf( naabb.MinDistanceFromPointSq( cameraPos ) );
      else
         selectionObj->m_selectionBuffer[i].MinDistToCamera = 0;

      selectionObj->m_minSelectedLODLevel = ::min( selectionObj->m_minSelectedLODLevel, selectionObj->m_selectionBuffer[i].LODLevel );
      selectionObj->m_maxSelectedLODLevel = ::max( selectionObj->m_maxSelectedLODLevel, selectionObj->m_selectionBuffer[i].LODLevel );
   }

   selectionObj->m_selectionCount      = lodSelInfo.SelectionCount;

   if( selectionObj->m_sortByDistance )
      qsort( selectionObj->m_selectionBuffer, selectionObj->m_selectionCount, sizeof(*selectionObj->m_selectionBuffer), compare_closerFirst );
}
//
void CDLODQuadTree::Node::GetAreaMinMaxHeight( int fromX, int fromY, int toX, int toY, float & minZ, float & maxZ, const CDLODQuadTree & quadTree ) const
{
   if( ((toX < this->X) || (toY < this->Y)) || ((fromX > (this->X+this->Size) ) || (fromY > (this->Y+this->Size)) ))
   {
      // Completely outside
      return;
   }

   bool hasNoLeafs = this->IsLeaf();

   if( hasNoLeafs || (((fromX <= this->X) && (fromY <= this->Y)) && ((toX >= (this->X+this->Size) ) && (toY >= (this->Y+this->Size)))) )
   {
      // Completely inside
      float worldMinZ, worldMaxZ;
      GetWorldMinMaxZ( worldMinZ, worldMaxZ, quadTree.m_desc.MapDims );
      minZ = ::min( minZ, worldMinZ );
      maxZ = ::max( maxZ, worldMaxZ );
      //DebugDrawAABB( 0x80FF0000 );
      return;
   }

   // Partially inside, partially outside
   if( this->SubTL != NULL )
      this->SubTL->GetAreaMinMaxHeight( fromX, fromY, toX, toY, minZ, maxZ, quadTree );
   if( this->SubTR != NULL )
      this->SubTR->GetAreaMinMaxHeight( fromX, fromY, toX, toY, minZ, maxZ, quadTree );
   if( this->SubBL != NULL )
      this->SubBL->GetAreaMinMaxHeight( fromX, fromY, toX, toY, minZ, maxZ, quadTree );
   if( this->SubBR != NULL )
      this->SubBR->GetAreaMinMaxHeight( fromX, fromY, toX, toY, minZ, maxZ, quadTree );
}

void CDLODQuadTree::GetAreaMinMaxHeight( float fromX, float fromY, float sizeX, float sizeY, float & minZ, float & maxZ ) const
{
   float bfx = (fromX - m_desc.MapDims.MinX) / m_desc.MapDims.SizeX;
   float bfy = (fromY - m_desc.MapDims.MinY) / m_desc.MapDims.SizeY;
   float btx = (fromX + sizeX - m_desc.MapDims.MinX) / m_desc.MapDims.SizeX;
   float bty = (fromY + sizeY - m_desc.MapDims.MinY) / m_desc.MapDims.SizeY;

   int rasterFromX   = clamp( (int)(bfx * m_rasterSizeX), 0, m_rasterSizeX-1 );
   int rasterFromY   = clamp( (int)(bfy * m_rasterSizeY), 0, m_rasterSizeY-1 );
   int rasterToX     = clamp( (int)(btx * m_rasterSizeX), 0, m_rasterSizeX-1 );
   int rasterToY     = clamp( (int)(bty * m_rasterSizeY), 0, m_rasterSizeY-1 );

   int baseFromX   = rasterFromX / m_topNodeSize;
   int baseFromY   = rasterFromY / m_topNodeSize;
   int baseToX     = rasterToX / m_topNodeSize;
   int baseToY     = rasterToY / m_topNodeSize;

   assert( baseFromX < m_topNodeCountX );
   assert( baseFromY < m_topNodeCountY );
   assert( baseToX < m_topNodeCountX );
   assert( baseToY < m_topNodeCountY );

   minZ = FLT_MAX;
   maxZ = -FLT_MAX;

   for( int y = baseFromY; y <= baseToY; y++ )
      for( int x = baseFromX; x <= baseToX; x++ )
      {
         m_topLevelNodes[y][x]->GetAreaMinMaxHeight( rasterFromX, rasterFromY, rasterToX, rasterToY, minZ, maxZ, *this );
      }

   //GetCanvas3D()->DrawBox( D3DXVECTOR3(fromX, fromY, minZ), D3DXVECTOR3(fromX + sizeX, fromY + sizeY, maxZ), 0xFFFFFF00, 0x10FFFF00 );
}
//
void CDLODQuadTree::Node::FillSubNodes( Node * nodes[4], int & count ) const
{
   count = 0;

   if( this->IsLeaf() )
      return;

   nodes[count++] = this->SubTL;

   if( this->SubTR != NULL )
      nodes[count++] = this->SubTR;
   if( this->SubBL != NULL )
      nodes[count++] = this->SubBL;
   if( this->SubBR != NULL )
      nodes[count++] = this->SubBR;

}
//
// Ray-Triangle Intersection Test Routines          //
// Different optimizations of my and Ben Trumbore's //
// code from journals of graphics tools (JGT)       //
// http://www.acm.org/jgt/                          //
// by Tomas Moller, May 2000                        //
//
#define _IT_CROSS(dest,v1,v2) \
   dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
   dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
   dest[2]=v1[0]*v2[1]-v1[1]*v2[0];
#define _IT_DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])
#define _IT_SUB(dest,v1,v2) \
   dest[0]=v1[0]-v2[0]; \
   dest[1]=v1[1]-v2[1]; \
   dest[2]=v1[2]-v2[2]; 
bool IntersectTri(const D3DXVECTOR3 & _orig, const D3DXVECTOR3 & _dir, const D3DXVECTOR3 & _vert0, const D3DXVECTOR3 & _vert1, 
                          const D3DXVECTOR3 & _vert2, float &u, float &v, float &dist)
{
   const float c_epsilon = 1e-6f;

   float * orig = (float *)&_orig;
   float * dir = (float *)&_dir;
   float * vert0 = (float *)&_vert0;
   float * vert1 = (float *)&_vert1;
   float * vert2 = (float *)&_vert2;

   float edge1[3], edge2[3], tvec[3], pvec[3], qvec[3];
   float det,inv_det;

   // find vectors for two edges sharing vert0
   _IT_SUB(edge1, vert1, vert0);
   _IT_SUB(edge2, vert2, vert0);

   // begin calculating determinant - also used to calculate U parameter
   _IT_CROSS(pvec, dir, edge2);

   // if determinant is near zero, ray lies in plane of triangle 
   det = _IT_DOT(edge1, pvec);

   // calculate distance from vert0 to ray origin 
   _IT_SUB(tvec, orig, vert0);
   inv_det = 1.0f / det;

   if (det > c_epsilon)
   {
      // calculate U parameter and test bounds 
      u = _IT_DOT(tvec, pvec);
      if (u < 0.0 || u > det)
         return false;

      // prepare to test V parameter 
      _IT_CROSS(qvec, tvec, edge1);

      // calculate V parameter and test bounds 
      v = _IT_DOT(dir, qvec);
      if (v < 0.0 || u + v > det)
         return false;

   }
   else if(det < -c_epsilon)
   {
      // calculate U parameter and test bounds 
      u = _IT_DOT(tvec, pvec);
      if (u > 0.0 || u < det)
         return false;

      // prepare to test V parameter 
      _IT_CROSS(qvec, tvec, edge1);

      // calculate V parameter and test bounds 
      v = _IT_DOT(dir, qvec) ;
      if (v > 0.0 || u + v < det)
         return false;
   }
   else return false;  // ray is parallel to the plane of the triangle 

   // calculate t, ray intersects triangle 
   dist = _IT_DOT(edge2, qvec) * inv_det;
   u *= inv_det;
   v *= inv_det;

   return dist >= 0;
}
//
bool CDLODQuadTree::Node::IntersectRay( const D3DXVECTOR3 & rayOrigin, const D3DXVECTOR3 & rayDirection, float maxDistance, D3DXVECTOR3 & hitPoint, const CDLODQuadTree & quadTree ) const
{
   AABB boundingBox;
   GetAABB( boundingBox, quadTree.m_rasterSizeX, quadTree.m_rasterSizeY, quadTree.m_desc.MapDims );

   float hitDistance = FLT_MAX;
   if( !boundingBox.IntersectRay(rayOrigin, rayDirection, hitDistance ) )
      return false;

   if( hitDistance > maxDistance )
      return false;

   //DebugDrawAABB( 0xFF0000FF, quadTree );

   if( IsLeaf() )
   {
      // This is the place to place your custom heightmap subsection ray intersection.
      // Area that needs to be tested lays between this node's [X, Y] and [X + Size, Y + Size] in
      // the source heightmap coordinates.
      
      // Following is the simple two-triangle per leaf-quad approximation.

      float * pTLZ = (float *)&this->SubTL;
      float * pTRZ = (float *)&this->SubTR;
      float * pBLZ = (float *)&this->SubBL;
      float * pBRZ = (float *)&this->SubBR;

      D3DXVECTOR3 tl( boundingBox.Min.x, boundingBox.Min.y, *pTLZ );
      D3DXVECTOR3 tr( boundingBox.Max.x, boundingBox.Min.y, *pTRZ );
      D3DXVECTOR3 bl( boundingBox.Min.x, boundingBox.Max.y, *pBLZ );
      D3DXVECTOR3 br( boundingBox.Max.x, boundingBox.Max.y, *pBRZ );

      // show these triangles!
      //GetCanvas3D()->DrawTriangle( tl, tr, bl, 0xFF000000, 0x8000FF00 );
      //GetCanvas3D()->DrawTriangle( tr, bl, br, 0xFF000000, 0x8000FF00 );

      float u0, v0, dist0;
      float u1, v1, dist1;
      bool t0 = IntersectTri( rayOrigin, rayDirection, tl, tr, bl, u0, v0, dist0 );
      bool t1 = IntersectTri( rayOrigin, rayDirection, tr, bl, br, u1, v1, dist1 );
      if( t0 && (dist0 > maxDistance) ) t0 = false;
      if( t1 && (dist1 > maxDistance) ) t1 = false;

      // No hits
      if( !t0 && !t1 )
         return false;

      // Only 0 hits, or 0 is closer
      if( (t0 && !t1) || ((t0 && t1) && (dist0 < dist1)) )
      {
         hitPoint = rayOrigin + rayDirection * dist0;
         return true;
      }

      hitPoint = rayOrigin + rayDirection * dist1;
      return true;
   }

   Node * subNodes[4];
   int subNodeCount;
   FillSubNodes( subNodes, subNodeCount );

   float       closestHitDist = FLT_MAX;
   D3DXVECTOR3 closestHit;

   for( int i = 0; i < subNodeCount; i++ )
   {
      D3DXVECTOR3 hit;
      if( subNodes[i]->IntersectRay( rayOrigin, rayDirection, maxDistance, hit, quadTree ) )
      {
         D3DXVECTOR3 diff = hit - rayOrigin;
         float dist = D3DXVec3Length(&diff);
         assert( dist <= maxDistance );
         if( dist < closestHitDist )
         {
            closestHitDist = dist;
            closestHit = hit;
         }
      }
   }

   if( closestHitDist != FLT_MAX )
   {
      hitPoint = closestHit;      

      //D3DXVECTOR3 vz( 0.5f, 0.5f, 200.0f );
      //GetCanvas3D()->DrawBox( hitPoint, hitPoint + vz, 0xFF000000, 0x8000FF00 );

      return true;
   }

   return false;
}
//
bool CDLODQuadTree::IntersectRay( const D3DXVECTOR3 & rayOrigin, const D3DXVECTOR3 & rayDirection, float maxDistance, D3DXVECTOR3 & hitPoint ) const
{
   float       closestHitDist = FLT_MAX;
   D3DXVECTOR3 closestHit;

   for( int y = 0; y < m_topNodeCountY; y++ )
      for( int x = 0; x < m_topNodeCountX; x++ )
      {
         D3DXVECTOR3 hit;
         if( m_topLevelNodes[y][x]->IntersectRay( rayOrigin, rayDirection, maxDistance, hit, *this ) )
         {
            D3DXVECTOR3 diff = hit - rayOrigin;
            float dist = D3DXVec3Length(&diff);
            assert( dist <= maxDistance );
            if( dist < closestHitDist )
            {
               closestHitDist = dist;
               closestHit = hit;
            }
         }
      }

   if( closestHitDist != FLT_MAX )
   {
      hitPoint = closestHit;      
      return true;
   }

   return false;
}
//
CDLODQuadTree::LODSelection::LODSelection( SelectedNode * selectionBuffer, int maxSelectionCount, const D3DXVECTOR3 & observerPos, float visibilityDistance, D3DXPLANE frustumPlanes[6], float LODDistanceRatio, float morphStartRatio, bool sortByDistance )
{
   m_selectionBuffer       = selectionBuffer;
   m_maxSelectionCount     = maxSelectionCount;
   m_observerPos           = observerPos;
   m_visibilityDistance    = visibilityDistance;
   memcpy( m_frustumPlanes, frustumPlanes, sizeof(m_frustumPlanes) );
   m_selectionCount        = 0;
   m_visDistTooSmall       = false;
   m_quadTree              = NULL;
   m_LODDistanceRatio      = LODDistanceRatio;
   m_morphStartRatio       = morphStartRatio;
   m_minSelectedLODLevel   = 0;
   m_maxSelectedLODLevel   = 0;
   m_sortByDistance        = sortByDistance;
}
//
CDLODQuadTree::LODSelection::~LODSelection( )
{
}
//
void CDLODQuadTree::LODSelection::GetMorphConsts( int LODLevel, float consts[4] ) const
{
   float mStart = m_morphStart[LODLevel];
   float mEnd = m_morphEnd[LODLevel];
   //
   const float errorFudge = 0.01f;
   mEnd = ::lerp( mEnd, mStart, errorFudge );
   //
   consts[0] = mStart;
   consts[1] = 1.0f / (mEnd-mStart);
   //
   consts[2] = mEnd / (mEnd-mStart);
   consts[3] = 1.0f / (mEnd-mStart);
}
//
