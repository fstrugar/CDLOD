//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#include "DXUT.h"

#include "CDLODQuadTree.h"
#include "CDLODTools.h"

#include "DxCanvas.h"

#include "DemoCamera.h"

#include "Core/IO/vaStream.h"

CDLODQuadTree::CDLODQuadTree()
{
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
   m_leafNodeSize = desc.LeafRenderNodeSize;
   m_rasterSizeX  = desc.RasterSizeX;
   m_rasterSizeY  = desc.RasterSizeY;
   m_MinMaxMaps   = desc.MinMaxMaps;

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
   m_leafNodeWorldSizeX = desc.LeafRenderNodeSize * desc.MapDims.SizeX / (float)(m_rasterSizeX-1);
   m_leafNodeWorldSizeY = desc.LeafRenderNodeSize * desc.MapDims.SizeY / (float)(m_rasterSizeY-1);
   m_LODLevelNodeDiagSizes[0] = sqrtf(m_leafNodeWorldSizeX * m_leafNodeWorldSizeX + m_leafNodeWorldSizeY * m_leafNodeWorldSizeY);
   //
   int totalNodeCount = 0;
   //
   assert( desc.LeafRenderNodeSize < 65535 );
   m_topNodeSize = (unsigned short)desc.LeafRenderNodeSize;
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

   // Calc LOD visibility range distribution
   {
      float currentDetailBalance = 1.0f;

      for( int i = 0; i < m_desc.LODLevelCount-1; i++ )
      {
         m_LODVisRangeDistRatios[i] = currentDetailBalance;
         
         // this is a hack to work around morphing problems with the first two LOD levels
         // (makes the zero LOD level a bit shorter)
         if( i == 0 )
            m_LODVisRangeDistRatios[i] *= 0.9f;

         currentDetailBalance *= m_desc.LODLevelDistanceRatio;
      }
      m_LODVisRangeDistRatios[m_desc.LODLevelCount-1] = currentDetailBalance;
      for( int i = 0; i < m_desc.LODLevelCount; i++ )
      {
         m_LODVisRangeDistRatios[i] /= currentDetailBalance;
      }
   }

   //////////////////////////////////////////////////////////////////////////
   m_topNodeCountX = (m_rasterSizeX-1+m_topNodeSize-1) / m_topNodeSize;
   m_topNodeCountY = (m_rasterSizeY-1+m_topNodeSize-1) / m_topNodeSize;
   //////////////////////////////////////////////////////////////////////////

   return true;
}

CDLODQuadTree::Node::LODSelectResult CDLODQuadTree::Node::LODSelect( LODSelectInfo & lodSelectInfo, bool parentCompletelyInFrustum, unsigned int x, unsigned int y, unsigned short size, int LODLevel, unsigned short parentMinZ, unsigned short parentSizeZ )
{
   AABB boundingBox;
   
   unsigned short minZ, maxZ;
   int minMaxMapX = x / size;
   int minMaxMapY = y / size;

   // This whole codepath could be split into two functions: one that handles the compressed (LODLevel == 0) case, 
   // and the other that handles the rest. The (LODLevel==0) could then be further optimized as it
   // doesn't require subnode selection handling, etc. 

   if( !lodSelectInfo.MinMaxMap[LODLevel].IsCompressed() )
   {
      lodSelectInfo.MinMaxMap[LODLevel].GetMinMax( minMaxMapX, minMaxMapY, minZ, maxZ );
   }
   else
   {
      lodSelectInfo.MinMaxMap[LODLevel].GetMinMaxCmp( minMaxMapX, minMaxMapY, parentMinZ, parentSizeZ, minZ, maxZ );
   }

   assert( maxZ >= minZ );
   unsigned short sizeZ = maxZ - minZ;

   GetAABB( boundingBox, lodSelectInfo.RasterSizeX, lodSelectInfo.RasterSizeY, lodSelectInfo.MapDims, x, y, size, minZ, maxZ );

   //D3DXVECTOR3 boundingSphereCenter;
   //float       boundingSphereRadiusSq;
   //GetBSphere( boundingSphereCenter, boundingSphereRadiusSq, quadTree );

   const D3DXPLANE * frustumPlanes  = lodSelectInfo.SelectionObj->m_frustumPlanes;
   const D3DXVECTOR3 & observerPos  = lodSelectInfo.SelectionObj->m_observerPos;
   const int maxSelectionCount      = lodSelectInfo.SelectionObj->m_maxSelectionCount;
   float * lodRanges                = lodSelectInfo.SelectionObj->m_visibilityRanges;
   
   // if not using frustum cull, just say that the parent is always in it
   parentCompletelyInFrustum |= !lodSelectInfo.SelectionObj->m_useFrustumCull;

   IntersectType frustumIt = (parentCompletelyInFrustum)?(IT_Inside):(boundingBox.TestInBoundingPlanes( frustumPlanes ));
   if( frustumIt == IT_Outside )
      return IT_OutOfFrustum;

   float distanceLimit = lodRanges[LODLevel];

   if( !boundingBox.IntersectSphereSq(observerPos, distanceLimit*distanceLimit) )
      return IT_OutOfRange;

   LODSelectResult subTLSelRes = IT_Undefined;
   LODSelectResult subTRSelRes = IT_Undefined;
   LODSelectResult subBLSelRes = IT_Undefined;
   LODSelectResult subBRSelRes = IT_Undefined;

   if( LODLevel != lodSelectInfo.SelectionObj->m_stopAtLODLevel )
   {
      float nextDistanceLimit = lodRanges[LODLevel-1];
      if( boundingBox.IntersectSphereSq( observerPos, nextDistanceLimit*nextDistanceLimit ) )
      {
         bool weAreCompletelyInFrustum = frustumIt == IT_Inside;

         bool subTLExists, subTRExists, subBLExists, subBRExists;
         lodSelectInfo.MinMaxMap[LODLevel-1].GetSubNodesExist( minMaxMapX, minMaxMapY, subTLExists, subTRExists, subBLExists, subBRExists );

         unsigned short halfSize = size / 2;
         if( subTLExists ) subTLSelRes = Node::LODSelect( lodSelectInfo, weAreCompletelyInFrustum, x, y, halfSize, LODLevel-1, minZ, sizeZ );
         if( subTRExists ) subTRSelRes = Node::LODSelect( lodSelectInfo, weAreCompletelyInFrustum, x + halfSize, y, halfSize, LODLevel-1, minZ, sizeZ );
         if( subBLExists ) subBLSelRes = Node::LODSelect( lodSelectInfo, weAreCompletelyInFrustum, x, y + halfSize, halfSize, LODLevel-1, minZ, sizeZ );
         if( subBRExists ) subBRSelRes = Node::LODSelect( lodSelectInfo, weAreCompletelyInFrustum, x + halfSize, y + halfSize, halfSize, LODLevel-1, minZ, sizeZ );
      }
   }

   // We don't want to select sub nodes that are invisible (out of frustum) or are selected;
   // (we DO want to select if they are out of range, since we are in range)
   bool bRemoveSubTL = (subTLSelRes == IT_OutOfFrustum) || (subTLSelRes == IT_Selected);
   bool bRemoveSubTR = (subTRSelRes == IT_OutOfFrustum) || (subTRSelRes == IT_Selected);
   bool bRemoveSubBL = (subBLSelRes == IT_OutOfFrustum) || (subBLSelRes == IT_Selected);
   bool bRemoveSubBR = (subBRSelRes == IT_OutOfFrustum) || (subBRSelRes == IT_Selected);

   assert( lodSelectInfo.SelectionCount < maxSelectionCount );
   if( 
      ( 
         // select (whole or in part) unless all sub nodes are selected by child nodes, either as parts of this or lower LOD levels
         !(bRemoveSubTL && bRemoveSubTR && bRemoveSubBL && bRemoveSubBR)
         // or select anyway if IncludeAllNodesInRange flag is on (used for streaming)
         || ((lodSelectInfo.SelectionObj->m_flags | LODSelection::IncludeAllNodesInRange) != 0) 
      )
      // but never select if selection buffer is already full
      && (lodSelectInfo.SelectionCount < maxSelectionCount) )
   {
      lodSelectInfo.SelectionObj->m_selectionBuffer[lodSelectInfo.SelectionCount++] = SelectedNode( x, y, size, minZ, maxZ, LODLevel, !bRemoveSubTL, !bRemoveSubTR, !bRemoveSubBL, !bRemoveSubBR );

      // This should be calculated somehow better, but brute force will work for now
      if( 
#ifndef _DEBUG
         !lodSelectInfo.SelectionObj->m_visDistTooSmall && 
#endif
         (LODLevel != lodSelectInfo.LODLevelCount-1) )
      {
         float maxDistFromCam = sqrtf( boundingBox.MaxDistanceFromPointSq( observerPos ) );

         float morphStartRange = lodSelectInfo.SelectionObj->m_morphStart[LODLevel+1];

         if( maxDistFromCam > morphStartRange )
         {
            lodSelectInfo.SelectionObj->m_visDistTooSmall = true;
#ifdef _DEBUG
            GetCanvas3D()->DrawBox( boundingBox.Min, boundingBox.Max, 0x80FF0000, 0x30FF0000 );
#endif
         }
      }
      
      return IT_Selected;
   }

   // if any of child nodes are selected, then return selected - otherwise all of them are out of frustum, so we're out of frustum too
   if( (subTLSelRes == IT_Selected) || (subTRSelRes == IT_Selected) || (subBLSelRes == IT_Selected) || (subBRSelRes == IT_Selected) )
      return IT_Selected;
   else
      return IT_OutOfFrustum;
}

void CDLODQuadTree::Clean( )
{
   // nothing to clean, everything is implicit except the MinMaxMap, for which CDLODQuadTree doesn't hold ownership
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
   const D3DXVECTOR3 & cameraPos    = selectionObj->m_observerPos;
   const int   LODLevelCount        = m_desc.LODLevelCount;

   float LODNear           = selectionObj->m_visibilityDistanceFrom;
   float LODFar            = selectionObj->m_visibilityDistance;

   selectionObj->m_quadTree   = this;
   selectionObj->m_visDistTooSmall = false;

   assert( LODLevelCount <= c_maxLODLevels );

   for( int i = 0; i < LODLevelCount; i++ ) 
   {
      selectionObj->m_visibilityRanges[i] = LODNear + m_LODVisRangeDistRatios[i] * (LODFar - LODNear);
   }

   float prevPos = LODNear;
   for (int i=0; i<LODLevelCount; i++)
   {
      selectionObj->m_morphEnd[i] = selectionObj->m_visibilityRanges[i];
      selectionObj->m_morphStart[i] = prevPos + (selectionObj->m_morphEnd[i] - prevPos) * selectionObj->m_morphStartRatio;

      prevPos = selectionObj->m_morphStart[i];
   }

   Node::LODSelectInfo lodSelInfo;
   lodSelInfo.RasterSizeX     = m_rasterSizeX;
   lodSelInfo.RasterSizeY     = m_rasterSizeY;
   lodSelInfo.MapDims         = m_desc.MapDims;
   lodSelInfo.SelectionCount  = 0;
   lodSelInfo.SelectionObj    = selectionObj;
   lodSelInfo.LODLevelCount   = LODLevelCount;
   lodSelInfo.MinMaxMap       = (CDLODMinMaxMap *)m_MinMaxMaps;

   for( unsigned int y = 0; y < m_topNodeCountY; y++ )
      for( unsigned int x = 0; x < m_topNodeCountX; x++ )
      {
         Node::LODSelect( lodSelInfo, false, x * m_topNodeSize, y * m_topNodeSize, m_topNodeSize, LODLevelCount-1, 0, 65535 );
      }

   selectionObj->m_maxSelectedLODLevel = 0;
   selectionObj->m_minSelectedLODLevel = c_maxLODLevels;

   for( int i = 0; i < lodSelInfo.SelectionCount; i++ )
   {
      AABB naabb;
      selectionObj->m_selectionBuffer[i].GetAABB(naabb, m_rasterSizeX, m_rasterSizeY, m_desc.MapDims);

      if( (selectionObj->m_flags | LODSelection::SortByDistance) != 0 )
         selectionObj->m_selectionBuffer[i].MinDistToCamera = sqrtf( naabb.MinDistanceFromPointSq( cameraPos ) );
      else
         selectionObj->m_selectionBuffer[i].MinDistToCamera = 0;

      selectionObj->m_minSelectedLODLevel = ::min( selectionObj->m_minSelectedLODLevel, selectionObj->m_selectionBuffer[i].LODLevel );
      selectionObj->m_maxSelectedLODLevel = ::max( selectionObj->m_maxSelectedLODLevel, selectionObj->m_selectionBuffer[i].LODLevel );
   }

   selectionObj->m_selectionCount      = lodSelInfo.SelectionCount;

   if( (selectionObj->m_flags | LODSelection::SortByDistance) != 0 )
      qsort( selectionObj->m_selectionBuffer, selectionObj->m_selectionCount, sizeof(*selectionObj->m_selectionBuffer), compare_closerFirst );
}
//
void CDLODQuadTree::Node::GetAreaMinMaxHeight( unsigned int fromX, unsigned int fromY, unsigned int toX, unsigned int toY, float & minZ, float & maxZ, const CDLODQuadTree & quadTree, unsigned int x, unsigned int y, unsigned short size, int LODLevel )
{
   if( ((toX < x) || (toY < y)) || ((fromX > (x+size) ) || (fromY > (y+size)) ))
   {
      // Completely outside
      return;
   }

   bool hasNoChildNodes = LODLevel == 0;

   int minMaxMapX = x / size;
   int minMaxMapY = y / size;

   if( hasNoChildNodes || (((fromX <= x) && (fromY <= y)) && ((toX >= (x+size) ) && (toY >= (y+size)))) )
   {
      unsigned short rminZ, rmaxZ;

      // This codepath could be split into two functions: one that handles the compressed (LODLevel == 0) case, 
      // and the other that handles the rest. 
      if( !quadTree.m_MinMaxMaps[LODLevel].IsCompressed() )
      {
         quadTree.m_MinMaxMaps[LODLevel].GetMinMax( minMaxMapX, minMaxMapY, rminZ, rmaxZ );
      }
      else
      {
         // not optimal at all...
         unsigned short parentMinZ, parentMaxZ;
         quadTree.m_MinMaxMaps[LODLevel+1].GetMinMax( minMaxMapX/2, minMaxMapY/2, parentMinZ, parentMaxZ );
         unsigned short parentSizeZ = (unsigned short)(parentMaxZ - parentMinZ);

         quadTree.m_MinMaxMaps[LODLevel].GetMinMaxCmp( minMaxMapX, minMaxMapY, parentMinZ, parentSizeZ, rminZ, rmaxZ );
      }

      // Completely inside (or leaf node)
      float worldMinZ, worldMaxZ;
      GetWorldMinMaxZ( rminZ, rmaxZ, worldMinZ, worldMaxZ, quadTree.m_desc.MapDims );
      minZ = ::min( minZ, worldMinZ );
      maxZ = ::max( maxZ, worldMaxZ );
      //DebugDrawAABB( 0x80FF0000 );
      return;
   }

   unsigned short halfSize = size / 2;
   CDLODMinMaxMap* minMaxMaps = (CDLODMinMaxMap*)quadTree.m_MinMaxMaps;

   bool subTLExists, subTRExists, subBLExists, subBRExists;
   minMaxMaps[LODLevel-1].GetSubNodesExist( minMaxMapX, minMaxMapY, subTLExists, subTRExists, subBLExists, subBRExists );

   // Partially inside, partially outside
   if( subTLExists )
      Node::GetAreaMinMaxHeight( fromX, fromY, toX, toY, minZ, maxZ, quadTree, x, y, halfSize, LODLevel-1 );
   if( subTRExists )
      Node::GetAreaMinMaxHeight( fromX, fromY, toX, toY, minZ, maxZ, quadTree, x+halfSize, y, halfSize, LODLevel-1 );
   if( subBLExists )
      Node::GetAreaMinMaxHeight( fromX, fromY, toX, toY, minZ, maxZ, quadTree, x, y+halfSize, halfSize, LODLevel-1 );
   if( subBRExists )
      Node::GetAreaMinMaxHeight( fromX, fromY, toX, toY, minZ, maxZ, quadTree, x+halfSize, y+halfSize, halfSize, LODLevel-1 );
}

void CDLODQuadTree::Node::EnumerateQuadsRecursive( QuadEnumeratorCallback * callback, const CDLODQuadTree & quadTree, unsigned int x, unsigned int y, unsigned short size, int LODLevel, unsigned short parentMinZ, unsigned short parentSizeZ )
{
   if( ((int)x >= (quadTree.GetRasterSizeX()-1)) || ((int)y >= (quadTree.GetRasterSizeY()-1)) )
      return;

   AABB boundingBox;

   unsigned short minZ, maxZ;
   int minMaxMapX = x / size;
   int minMaxMapY = y / size;

   if( !quadTree.m_MinMaxMaps[LODLevel].IsCompressed() )
   {
      quadTree.m_MinMaxMaps[LODLevel].GetMinMax( minMaxMapX, minMaxMapY, minZ, maxZ );
   }
   else
   {
      quadTree.m_MinMaxMaps[LODLevel].GetMinMaxCmp( minMaxMapX, minMaxMapY, parentMinZ, parentSizeZ, minZ, maxZ );
   }

   assert( maxZ >= minZ );
   unsigned short sizeZ = maxZ - minZ;

   GetAABB( boundingBox, quadTree.m_rasterSizeX, quadTree.m_rasterSizeY, quadTree.m_desc.MapDims, x, y, size, minZ, maxZ );

   if( !callback->OnQuadTouched( x, y, size, LODLevel, minZ, maxZ, boundingBox ) )
      return;

   if( LODLevel == 0 )
      return;

   unsigned short halfSize = size / 2;
   Node::EnumerateQuadsRecursive( callback, quadTree, x, y, halfSize, LODLevel-1, minZ, sizeZ );
   Node::EnumerateQuadsRecursive( callback, quadTree, x + halfSize, y, halfSize, LODLevel-1, minZ, sizeZ );
   Node::EnumerateQuadsRecursive( callback, quadTree, x, y + halfSize, halfSize, LODLevel-1, minZ, sizeZ );
   Node::EnumerateQuadsRecursive( callback, quadTree, x + halfSize, y + halfSize, halfSize, LODLevel-1, minZ, sizeZ );
}

void CDLODQuadTree::Node::EnumerateQuadsRecursive( QuadEnumeratorCallback * callback, const CDLODQuadTree & quadTree, unsigned int x, unsigned int y, 
                                                   unsigned short size, int LODLevel, unsigned short parentMinZ, unsigned short parentSizeZ,
                                                   const D3DXVECTOR3 & boundingSpherePos, float boundingSphereSize )
{
   AABB boundingBox;

   assert( false );  // codepath not tested

   unsigned short minZ, maxZ;
   int minMaxMapX = x / size;
   int minMaxMapY = y / size;

   if( !quadTree.m_MinMaxMaps[LODLevel].IsCompressed() )
   {
      quadTree.m_MinMaxMaps[LODLevel].GetMinMax( minMaxMapX, minMaxMapY, minZ, maxZ );
   }
   else
   {
      quadTree.m_MinMaxMaps[LODLevel].GetMinMaxCmp( minMaxMapX, minMaxMapY, parentMinZ, parentSizeZ, minZ, maxZ );
   }

   assert( maxZ >= minZ );
   unsigned short sizeZ = maxZ - minZ;

   GetAABB( boundingBox, quadTree.m_rasterSizeX, quadTree.m_rasterSizeY, quadTree.m_desc.MapDims, x, y, size, minZ, maxZ );

   if( !boundingBox.IntersectSphereSq( boundingSpherePos, boundingSphereSize*boundingSphereSize ) )
      return;

   if( !callback->OnQuadTouched( x, y, size, LODLevel, minZ, maxZ, boundingBox ) )
      return;

   if( LODLevel == 0 )
      return;

   unsigned short halfSize = size / 2;
   Node::EnumerateQuadsRecursive( callback, quadTree, x, y, halfSize, LODLevel-1, minZ, sizeZ, boundingSpherePos, boundingSphereSize );
   Node::EnumerateQuadsRecursive( callback, quadTree, x + halfSize, y, halfSize, LODLevel-1, minZ, sizeZ, boundingSpherePos, boundingSphereSize );
   Node::EnumerateQuadsRecursive( callback, quadTree, x, y + halfSize, halfSize, LODLevel-1, minZ, sizeZ, boundingSpherePos, boundingSphereSize );
   Node::EnumerateQuadsRecursive( callback, quadTree, x + halfSize, y + halfSize, halfSize, LODLevel-1, minZ, sizeZ, boundingSpherePos, boundingSphereSize );
}

void CDLODQuadTree::GetAreaMinMaxHeight( float fromX, float fromY, float sizeX, float sizeY, float & minZ, float & maxZ ) const
{
   float bfx = (fromX - m_desc.MapDims.MinX) / m_desc.MapDims.SizeX;
   float bfy = (fromY - m_desc.MapDims.MinY) / m_desc.MapDims.SizeY;
   float btx = (fromX + sizeX - m_desc.MapDims.MinX) / m_desc.MapDims.SizeX;
   float bty = (fromY + sizeY - m_desc.MapDims.MinY) / m_desc.MapDims.SizeY;

   unsigned int rasterFromX   = (unsigned int)clamp( (int)(bfx * (m_rasterSizeX-1)), 0, m_rasterSizeX-1 );
   unsigned int rasterFromY   = (unsigned int)clamp( (int)(bfy * (m_rasterSizeY-1)), 0, m_rasterSizeY-1 );
   unsigned int rasterToX     = (unsigned int)clamp( (int)(btx * (m_rasterSizeX-1)), 0, m_rasterSizeX-1 );
   unsigned int rasterToY     = (unsigned int)clamp( (int)(bty * (m_rasterSizeY-1)), 0, m_rasterSizeY-1 );

   unsigned int baseFromX   = rasterFromX / m_topNodeSize;
   unsigned int baseFromY   = rasterFromY / m_topNodeSize;
   unsigned int baseToX     = rasterToX / m_topNodeSize;
   unsigned int baseToY     = rasterToY / m_topNodeSize;

   assert( baseFromX < m_topNodeCountX );
   assert( baseFromY < m_topNodeCountY );
   assert( baseToX < m_topNodeCountX );
   assert( baseToY < m_topNodeCountY );

   minZ = FLT_MAX;
   maxZ = -FLT_MAX;

   for( unsigned int y = baseFromY; y <= baseToY; y++ )
      for( unsigned int x = baseFromX; x <= baseToX; x++ )
      {
         Node::GetAreaMinMaxHeight( rasterFromX, rasterFromY, rasterToX, rasterToY, minZ, maxZ, *this, (unsigned int)x * m_topNodeSize, (unsigned int)y * m_topNodeSize, (unsigned short)m_topNodeSize, m_desc.LODLevelCount-1 );
      }

   //GetCanvas3D()->DrawBox( D3DXVECTOR3(fromX, fromY, minZ), D3DXVECTOR3(fromX + sizeX, fromY + sizeY, maxZ), 0xFFFFFF00, 0x10FFFF00 );
}
//
void CDLODQuadTree::EnumerateQuadsRecursive( QuadEnumeratorCallback * callback, const int rasterX, const int rasterY, const int rasterSizeX, const int rasterSizeY )
{
   unsigned int rasterFromX   = (unsigned int)clamp( (int)(rasterX), 0, m_rasterSizeX-1 );
   unsigned int rasterFromY   = (unsigned int)clamp( (int)(rasterY), 0, m_rasterSizeY-1 );
   unsigned int rasterToX     = (unsigned int)clamp( (int)(rasterX + rasterSizeX), 0, m_rasterSizeX-1-1 );
   unsigned int rasterToY     = (unsigned int)clamp( (int)(rasterY + rasterSizeY), 0, m_rasterSizeY-1-1 );

   unsigned int baseFromX   = rasterFromX / m_topNodeSize;
   unsigned int baseFromY   = rasterFromY / m_topNodeSize;
   unsigned int baseToX     = rasterToX / m_topNodeSize;
   unsigned int baseToY     = rasterToY / m_topNodeSize;

   assert( baseFromX < m_topNodeCountX );
   assert( baseFromY < m_topNodeCountY );
   assert( baseToX < m_topNodeCountX );
   assert( baseToY < m_topNodeCountY );

   for( unsigned int y = baseFromY; y <= baseToY; y++ )
      for( unsigned int x = baseFromX; x <= baseToX; x++ )
      {
         Node::EnumerateQuadsRecursive( callback, *this, x * m_topNodeSize, y * m_topNodeSize, m_topNodeSize, m_desc.LODLevelCount-1, 0, 65535 );
      }
}
//
void CDLODQuadTree::EnumerateQuadsRecursive( QuadEnumeratorCallback * callback, const D3DXVECTOR3 & boundingSpherePos, float boundingSphereSize )
{
   const float fromX = boundingSpherePos.x - boundingSphereSize;
   const float fromY = boundingSpherePos.y - boundingSphereSize;
   const float toX = boundingSpherePos.x + boundingSphereSize;
   const float toY = boundingSpherePos.y + boundingSphereSize;

   const float bfx = (fromX - m_desc.MapDims.MinX) / m_desc.MapDims.SizeX;
   const float bfy = (fromY - m_desc.MapDims.MinY) / m_desc.MapDims.SizeY;
   const float btx = (toX - m_desc.MapDims.MinX) / m_desc.MapDims.SizeX;
   const float bty = (toY - m_desc.MapDims.MinY) / m_desc.MapDims.SizeY;

   unsigned int rasterFromX   = (unsigned int)clamp( (int)(bfx * (m_rasterSizeX-1)), 0, m_rasterSizeX-1 );
   unsigned int rasterFromY   = (unsigned int)clamp( (int)(bfy * (m_rasterSizeY-1)), 0, m_rasterSizeY-1 );
   unsigned int rasterToX     = (unsigned int)clamp( (int)(btx * (m_rasterSizeX-1)), 0, m_rasterSizeX-1 );
   unsigned int rasterToY     = (unsigned int)clamp( (int)(bty * (m_rasterSizeY-1)), 0, m_rasterSizeY-1 );

   unsigned int baseFromX   = rasterFromX / m_topNodeSize;
   unsigned int baseFromY   = rasterFromY / m_topNodeSize;
   unsigned int baseToX     = rasterToX / m_topNodeSize;
   unsigned int baseToY     = rasterToY / m_topNodeSize;

   assert( baseFromX < m_topNodeCountX );
   assert( baseFromY < m_topNodeCountY );
   assert( baseToX < m_topNodeCountX );
   assert( baseToY < m_topNodeCountY );

   for( unsigned int y = baseFromY; y <= baseToY; y++ )
      for( unsigned int x = baseFromX; x <= baseToX; x++ )
      {
         Node::EnumerateQuadsRecursive( callback, *this, x * m_topNodeSize, y * m_topNodeSize, m_topNodeSize, m_desc.LODLevelCount-1, 
                                          0, 65535, boundingSpherePos, boundingSphereSize );
      }
}
//
bool CDLODQuadTree::Node::CoarseIntersectRay( const D3DXVECTOR3 & rayOrigin, const D3DXVECTOR3 & rayDirection, float maxDistance, D3DXVECTOR3 & hitPoint, const CDLODQuadTree & quadTree, unsigned int x, unsigned int y, unsigned short size, int LODLevel, unsigned short parentMinZ, unsigned short parentSizeZ )
{
   AABB boundingBox;

   int minMaxMapX = x / size;
   int minMaxMapY = y / size;

   unsigned short rminZ, rmaxZ;

   // This codepath could be split into two functions: one that handles the compressed (LODLevel == 0) case, 
   // and the other that handles the rest.
   if( !quadTree.m_MinMaxMaps[LODLevel].IsCompressed() )
   {
      quadTree.m_MinMaxMaps[LODLevel].GetMinMax( minMaxMapX, minMaxMapY, rminZ, rmaxZ );
   }
   else
   {
      quadTree.m_MinMaxMaps[LODLevel].GetMinMaxCmp( minMaxMapX, minMaxMapY, parentMinZ, parentSizeZ, rminZ, rmaxZ );
   }

   GetAABB( boundingBox, quadTree.m_rasterSizeX, quadTree.m_rasterSizeY, quadTree.m_desc.MapDims, x, y, size, rminZ, rmaxZ );

   assert( rmaxZ >= rminZ );
   unsigned short rsizeZ = rmaxZ - rminZ;

   float hitDistance = FLT_MAX;
   if( !boundingBox.IntersectRay( rayOrigin, rayDirection, hitDistance ) )
      return false;

   if( hitDistance > maxDistance )
      return false;

   //GetCanvas3D()->DrawBox( boundingBox.Min, boundingBox.Max, 0xFF000000, 0x2000FF00 );

   if( LODLevel == 0 )
   {
      // This is the place to place your custom heightmap subsection ray intersection.
      // Area that needs to be tested lays between this node's [X, Y] and [X + Size, Y + Size] in
      // the source heightmap coordinates.

      // using collision with far side AABB might be better but this will do for now...
      hitPoint = rayOrigin + rayDirection * hitDistance;

      return true;

      //do the test against the mid plane and back sides of the AABB and return the closest: this
      //could be the best and least complex approximation for this case

      /*
      float worldMidZ = (boundingBox.Min.z + boundingBox.Max.z) * 0.5f;

      D3DXVECTOR3 tl( boundingBox.Min.x, boundingBox.Min.y, worldMidZ );
      D3DXVECTOR3 tr( boundingBox.Max.x, boundingBox.Min.y, worldMidZ );
      D3DXVECTOR3 bl( boundingBox.Min.x, boundingBox.Max.y, worldMidZ );
      D3DXVECTOR3 br( boundingBox.Max.x, boundingBox.Max.y, worldMidZ );

      // show these triangles!
      GetCanvas3D()->DrawTriangle( tl, tr, bl, 0xFF000000, 0x8000FF00 );
      GetCanvas3D()->DrawTriangle( tr, bl, br, 0xFF000000, 0x8000FF00 );

      float u0, v0, dist0;
      float u1, v1, dist1;
      bool t0 = IntersectTri( rayOrigin, rayDirection, tl, tr, bl, u0, v0, dist0 );
      bool t1 = IntersectTri( rayOrigin, rayDirection, tr, bl, br, u1, v1, dist1 );
      if( t0 && (dist0 > maxDistance) ) t0 = false;
      if( t1 && (dist1 > maxDistance) ) t1 = false;

      // No hits
      if( !t0 && !t1 )
      {
         return false;
      }

      // Only 0 hits, or 0 is closer
      if( (t0 && !t1) || ((t0 && t1) && (dist0 < dist1)) )
      {
         hitPoint = rayOrigin + rayDirection * dist0;
         return true;
      }

      hitPoint = rayOrigin + rayDirection * dist1;
      return true;
      */
   }

   unsigned short halfSize = size / 2;
   CDLODMinMaxMap* minMaxMaps = (CDLODMinMaxMap*)quadTree.m_MinMaxMaps;

   bool subTLExists, subTRExists, subBLExists, subBRExists;
   minMaxMaps[LODLevel-1].GetSubNodesExist( minMaxMapX, minMaxMapY, subTLExists, subTRExists, subBLExists, subBRExists );

   // Partially inside, partially outside

   float       closestHitDist = FLT_MAX;
   D3DXVECTOR3 closestHit;
   D3DXVECTOR3 hit;

   if( subTLExists && Node::CoarseIntersectRay( rayOrigin, rayDirection, maxDistance, hit, quadTree, x, y, halfSize, LODLevel-1, rminZ, rsizeZ ) )
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

   if( subTRExists && Node::CoarseIntersectRay( rayOrigin, rayDirection, maxDistance, hit, quadTree, x+halfSize, y, halfSize, LODLevel-1, rminZ, rsizeZ ) )
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

   if( subBLExists && Node::CoarseIntersectRay( rayOrigin, rayDirection, maxDistance, hit, quadTree, x, y+halfSize, halfSize, LODLevel-1, rminZ, rsizeZ ) )
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

   if( subBRExists && Node::CoarseIntersectRay( rayOrigin, rayDirection, maxDistance, hit, quadTree, x+halfSize, y+halfSize, halfSize, LODLevel-1, rminZ, rsizeZ ) )
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
bool CDLODQuadTree::CoarseIntersectRay( const D3DXVECTOR3 & rayOrigin, const D3DXVECTOR3 & rayDirection, float maxDistance, D3DXVECTOR3 & hitPoint ) const
{
   float       closestHitDist = FLT_MAX;
   D3DXVECTOR3 closestHit;

   for( unsigned int y = 0; y < m_topNodeCountY; y++ )
      for( unsigned int x = 0; x < m_topNodeCountX; x++ )
      {
         D3DXVECTOR3 hit;
         if( Node::CoarseIntersectRay( rayOrigin, rayDirection, maxDistance, hit, *this, x * m_topNodeSize, y * m_topNodeSize, m_topNodeSize, m_desc.LODLevelCount-1, 0, 65535 ) )
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
CDLODQuadTree::LODSelection::LODSelection( SelectedNode * selectionBuffer, int maxSelectionCount, const D3DXVECTOR3 & observerPos, 
                                         float visibilityDistance, D3DXPLANE frustumPlanes[6], float visibilityDistanceFrom, 
                                         int stopAtLODLevel, float morphStartRatio, Flags flags )
{
   m_selectionBuffer       = selectionBuffer;
   m_maxSelectionCount     = maxSelectionCount;
   m_observerPos           = observerPos;
   m_visibilityDistance    = visibilityDistance;
   if( frustumPlanes == NULL )
   {
      memset( m_frustumPlanes, 0, sizeof(m_frustumPlanes) );
      m_useFrustumCull = false;
   }
   else
   {
      memcpy( m_frustumPlanes, frustumPlanes, sizeof(m_frustumPlanes) );
      m_useFrustumCull = true;
   }
   m_visibilityDistanceFrom= visibilityDistanceFrom;
   assert( m_visibilityDistance > m_visibilityDistanceFrom );
   m_stopAtLODLevel        = stopAtLODLevel;
   m_selectionCount        = 0;
   m_visDistTooSmall       = false;
   m_quadTree              = NULL;
   m_morphStartRatio       = morphStartRatio;
   assert( morphStartRatio > 0.01 && morphStartRatio < 0.99 );
   m_minSelectedLODLevel   = 0;
   m_maxSelectedLODLevel   = 0;
   m_flags                 = flags;
   m_streamingDataHandler  = NULL;
}
//
CDLODQuadTree::LODSelection::~LODSelection( )
{
   // Forgot to call CDLODStreamingStorage::FlushSelectionStreamingData before destructing the LODSelection object?
   assert( m_streamingDataHandler == NULL );
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
void CDLODMinMaxMap::Initialize( int elementsX, int elementsY, bool compressed )
{
   Deinitialize();

   m_elementsX = elementsX;
   m_elementsY = elementsY;

   int itemCount = elementsX * elementsY * 2;
   if( !compressed )
   {
      m_data = new unsigned short[ itemCount ];
      m_dataSizeInBytes = sizeof(m_data[0]) * itemCount;
   }
   else
   {
      m_dataCmp = new unsigned char[ itemCount ];
      m_dataCmpSizeInBytes = sizeof(m_dataCmp[0]) * itemCount;
   }
}
//
void CDLODMinMaxMap::Deinitialize()
{
   if( m_data != NULL ) delete[] m_data;
   m_data = NULL;
   m_dataSizeInBytes = 0;
   if( m_dataCmp != NULL ) delete[] m_dataCmp;
   m_dataCmp = NULL;
   m_dataCmpSizeInBytes = 0;
   m_elementsX = 0;
   m_elementsY = 0;
}
//
void CDLODMinMaxMap::GenerateFromHigherDetailed( const CDLODMinMaxMap & source )
{
   int srcDimX = source.GetSizeX();
   int srcDimY = source.GetSizeY();
   int dimX = (srcDimX + 1) / 2;
   int dimY = (srcDimY + 1) / 2;

   Initialize( dimX, dimY );

   const unsigned short * srcBuffer = source.GetData();

   unsigned short * destBuffer = m_data;

   // strange backward algorithm but I can't be bothered to write a proper one

   for( int i = 0; i < dimX * dimY * 2; i += 2 )
   {
      destBuffer[ i + 0 ] = 65535;
      destBuffer[ i + 1 ] = 0;
   }

   for( int y = 0; y < srcDimY; y++ )
   {
      for( int x = 0; x < srcDimX; x++ )
      {
         const int dx = x / 2;
         destBuffer[ dx * 2 + 0 ] = ::min( destBuffer[ dx * 2 + 0 ], srcBuffer[ x * 2 + 0 ] );
         destBuffer[ dx * 2 + 1 ] = ::max( destBuffer[ dx * 2 + 1 ], srcBuffer[ x * 2 + 1 ] );
      }
      srcBuffer += srcDimX * 2;
      if( y % 2 == 1 )
         destBuffer += dimX * 2;
   }
}
//
bool CDLODMinMaxMap::Save( VertexAsylum::vaStream * outStream )
{
   if( !outStream->WriteValue<int32>(1) ) // version
      return false;

   if( !outStream->WriteValue<uint32>(m_elementsX) )
      return false;

   if( !outStream->WriteValue<uint32>(m_elementsY) )
      return false;

   if( !outStream->WriteValue<bool>(IsCompressed()) )
      return false;

   if( IsCompressed() )
   {
      if( !outStream->WriteValue(m_dataCmpSizeInBytes) )
         return false;

      if( outStream->Write(m_dataCmp, m_dataCmpSizeInBytes) != (int)m_dataCmpSizeInBytes )
         return false;
   }
   else
   {
      if( !outStream->WriteValue(m_dataSizeInBytes) )
         return false;

      if( outStream->Write(m_data, m_dataSizeInBytes) != (int)m_dataSizeInBytes )
         return false;
   }
   
   return true;
}
//
bool CDLODMinMaxMap::Load( VertexAsylum::vaStream * inStream )
{
   Deinitialize();

   int version;
   if( !inStream->ReadValue( version ) )
      vaFatalError( "Error while reading CDLODMinMaxMap" );
   if( version != 1 )
      vaFatalError( "Error while reading CDLODMinMaxMap - incorrect version" );

   uint32 elementsX, elementsY;

   if( !inStream->ReadValue<uint32>(elementsX) )
      return false;

   if( !inStream->ReadValue<uint32>(elementsY) )
      return false;

   bool isCompressed;
   if( !inStream->ReadValue<bool>(isCompressed) )
      return false;

   Initialize( elementsX, elementsY, isCompressed );

   if( isCompressed )
   {
      if( !inStream->ReadValue(m_dataCmpSizeInBytes) )
         return false;

      if( inStream->Read(m_dataCmp, m_dataCmpSizeInBytes) != (int)m_dataCmpSizeInBytes )
         return false;
   }
   else
   {
      if( !inStream->ReadValue(m_dataSizeInBytes) )
         return false;

      if( inStream->Read(m_data, m_dataSizeInBytes) != (int)m_dataSizeInBytes )
         return false;
   }

   return true;
}
//
inline unsigned char CDLODMinMaxMap_pack_min( unsigned short minZ, unsigned short maxZ, unsigned short val )
{
   assert( val >= minZ && val <= maxZ );

   unsigned short sizeZ = maxZ - minZ;
   val = (unsigned short)(val - minZ);

   if( sizeZ != 0 )
      val = val * 255 / sizeZ;
   assert( val <= 255 );

   return (unsigned char)val;
}
//
inline unsigned char CDLODMinMaxMap_pack_max( unsigned short minZ, unsigned short maxZ, unsigned short val )
{
   assert( val >= minZ && val <= maxZ );

   unsigned short sizeZ = maxZ - minZ;
   val = (unsigned short)(val - minZ);

   if( sizeZ != 0 )
      val = (val * 255 + sizeZ - 1) / sizeZ;
   assert( val <= 255 );

   return (unsigned char)val;
}
//
void CDLODMinMaxMap::CompressBasedOnLowerDetailed( const CDLODMinMaxMap & source )
{
   assert( (m_elementsX == (source.GetSizeX()*2)) || (m_elementsX == (source.GetSizeX()*2-1)) );
   assert( (m_elementsY == (source.GetSizeY()*2)) || (m_elementsY == (source.GetSizeY()*2-1)) );
   assert( m_data != NULL );
   assert( m_dataCmp == NULL );

   const unsigned short * srcBuffer = source.GetData();

   m_dataCmp = new unsigned char[m_elementsX * m_elementsY * 2];
   m_dataCmpSizeInBytes = m_elementsX * m_elementsY * 2;

   int srcDimX = source.GetSizeX();

   for( unsigned int y = 0; y < m_elementsY; y++ )
   {
      for( unsigned int x = 0; x < m_elementsX; x++ )
      {
         unsigned short minZ = srcBuffer[ ((x/2) + (y/2)*srcDimX)*2 + 0 ];
         unsigned short maxZ = srcBuffer[ ((x/2) + (y/2)*srcDimX)*2 + 1 ];

         m_dataCmp[ (x + y * m_elementsX)*2 + 0 ] = CDLODMinMaxMap_pack_min( minZ, maxZ, m_data[ (x + y * m_elementsX)*2 + 0 ] );
         m_dataCmp[ (x + y * m_elementsX)*2 + 1 ] = CDLODMinMaxMap_pack_max( minZ, maxZ, m_data[ (x + y * m_elementsX)*2 + 1 ] );
      }
   }

   delete[] m_data;
   m_data = NULL;
   m_dataSizeInBytes = 0;
}
//
void  CDLODQuadTree::LODSelection::SetStreamingDataHandler( uint32 streamingDataHandler )
{ 
   // We cannot overwrite previous handler unless we properly unlink from it first.
   assert( m_streamingDataHandler == NULL || streamingDataHandler == NULL );
   
   m_streamingDataHandler = streamingDataHandler; 
}
