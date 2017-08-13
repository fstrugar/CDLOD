//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Common.h"

#include "MiniMath.h"

//////////////////////////////////////////////////////////////////////////
// Interface for providing source height data to CDLODQuadTree
//////////////////////////////////////////////////////////////////////////
class IHeightmapSource
{
public:
   virtual int             GetSizeX( )                   = 0;
   virtual int             GetSizeY( )                   = 0;

   // returned value is converted to height using following formula:
   // 'WorldHeight = WorldMinZ + GetHeightAt(,) * WorldSizeZ / 65535.0f'
   virtual unsigned short  GetHeightAt( int x, int y )   = 0;

   virtual void            GetAreaMinMaxZ( int x, int y, int sizeX, int sizeY, unsigned short & minZ, unsigned short & maxZ ) = 0;
};

//////////////////////////////////////////////////////////////////////////
// Main class for storing and working with CDLOD quadtree
//////////////////////////////////////////////////////////////////////////
class CDLODQuadTree
{
public:

   static const int     c_maxLODLevels    = 15;

   struct Node;

   struct CreateDesc
   {
      IHeightmapSource* pHeightmap;

      int               LeafRenderNodeSize;

      // The number of LOD levels possible - quad tree will have one level more.
      int               LODLevelCount;

      // Heightmap world dimensions
      MapDimensions     MapDims;
   };

   struct SelectedNode
   {
      unsigned int   X;
      unsigned int   Y;
      unsigned short Size;
      unsigned short MinZ;
      unsigned short MaxZ;

      bool           TL;
      bool           TR;
      bool           BL;
      bool           BR;
      float          MinDistToCamera;     // this field is valid only if LODSelection::m_sortByDistance is enabled
      int            LODLevel;

      SelectedNode()      {}
      SelectedNode( const Node * node, int LODLevel, bool tl, bool tr, bool bl, bool br );

      void           GetAABB( AABB & aabb, int rasterSizeX, int rasterSizeY, const MapDimensions & mapDims ) const;
   };

   class LODSelection
   {
   private:
      friend class CDLODQuadTree;
      friend struct CDLODQuadTree::Node;

      // Input
      SelectedNode *       m_selectionBuffer;
      int                  m_maxSelectionCount;
      D3DXVECTOR3          m_observerPos;
      float                m_visibilityDistance;
      D3DXPLANE            m_frustumPlanes[6];
      float                m_LODDistanceRatio;
      float                m_morphStartRatio;                  // [0, 1] - when to start morphing to the next (lower-detailed) LOD level; default is 0.667 - first 0.667 part will not be morphed, and the morph will go from 0.667 to 1.0
      bool                 m_sortByDistance;

      // Output 
      const CDLODQuadTree * m_quadTree;
      float                m_visibilityRanges[c_maxLODLevels];
      float                m_morphEnd[c_maxLODLevels];
      float                m_morphStart[c_maxLODLevels];
      int                  m_selectionCount;
      bool                 m_visDistTooSmall;
      int                  m_minSelectedLODLevel;
      int                  m_maxSelectedLODLevel;

   public:
      LODSelection( SelectedNode * selectionBuffer, int maxSelectionCount, const D3DXVECTOR3 & observerPos, float visibilityDistance, D3DXPLANE frustumPlanes[6], float LODDistanceRatio, float morphStartRatio = 0.66f, bool sortByDistance = false );
      ~LODSelection();

      const CDLODQuadTree *          GetQuadTree() const              { return m_quadTree; }

      void                          GetMorphConsts( int LODLevel, float consts[4] ) const;

      inline const SelectedNode *   GetSelection() const             { return m_selectionBuffer; }
      inline int                    GetSelectionCount() const        { return m_selectionCount; }

      float                         GetLODDistanceRatio() const      { return m_LODDistanceRatio; }

      const float *                 GetLODLevelRanges() const        { return m_morphEnd; }

      // Ugly brute-force mechanism - could be replaced by deterministic one possibly
      bool                          IsVisDistTooSmall() const        { return m_visDistTooSmall; }

      int                           GetMinSelectedLevel() const      { return m_minSelectedLODLevel; }
      int                           GetMaxSelectedLevel() const      { return m_maxSelectedLODLevel; }
   };

   template< int maxSelectionCount >
   class LODSelectionOnStack : public LODSelection
   {
      SelectedNode      m_selectionBufferOnStack[maxSelectionCount];

   public:
      LODSelectionOnStack( const D3DXVECTOR3 & observerPos, float visibilityDistance, D3DXPLANE frustumPlanes[6], float LODDistanceRatio, float morphStartRatio = 0.66f, bool sortByDistance = false )
         : LODSelection( m_selectionBufferOnStack, maxSelectionCount, observerPos, visibilityDistance, frustumPlanes, LODDistanceRatio, morphStartRatio, sortByDistance )
      { }

   };

   // Although relatively small (28 bytes) the Node struct can use a lot of memory when used on
   // big datasets and high granularity settings (big depth).
   // For example, for a terrain of 16384*8192 with a leaf node size of 32, it will consume
   // around 4.6Mb.
   //
   // However, most of its values can be created implicitly at runtime, while the only ones needed to
   // be stored are MinZ/MaxZ values. StreamingCDLOD demo uses such implicit storage.
   struct Node
   {
      enum LODSelectResult
      {
         IT_Undefined,
         IT_OutOfFrustum,
         IT_OutOfRange,
         IT_Selected,
      };

      struct LODSelectInfo
      {
         LODSelection *       SelectionObj;
         int                  SelectionCount;
         int                  StopAtLevel;
         int                  RasterSizeX;
         int                  RasterSizeY;
         MapDimensions        MapDims;
      };

      friend class CDLODQuadTree;

      unsigned short    X;
      unsigned short    Y;
      unsigned short    Size;

      unsigned short    Level;            // Caution: highest bit here is used to mark leaf nodes

      unsigned short    MinZ;
      unsigned short    MaxZ;

      // When IsLeaf() these can be reused for something else (currently they store float heights for ray triangle test 
      // but that could be stored in a matrix without 4x redundancy like here)
      // Also, these could/should be indices into CDLODQuadTree::m_allNodesBuffer - no additional memory will then be used
      // if compiled for 64bit, and they could also be unsigned short-s if having less 65535 nodes
      Node *            SubTL;
      Node *            SubTR;
      Node *            SubBL;
      Node *            SubBR;

      // Level 0 is a root node, and level 'LodLevel-1' is a leaf node. So the actual 
      // LOD level equals 'LODLevelCount - 1 - Node::GetLevel()'.
      unsigned short    GetLevel() const        { return (unsigned short)(Level & 0x7FFF); }
      bool              IsLeaf() const          { return (Level & 0x8000) != 0; }

      // To save memory we can calculate this at runtime, but no point in doing that right now...
      void              GetWorldMinMaxX( float & minX, float & maxX, int rasterSizeX, const MapDimensions & mapDims ) const;
      void              GetWorldMinMaxY( float & minY, float & maxY, int rasterSizeY, const MapDimensions & mapDims ) const;
      void              GetWorldMinMaxZ( float & minZ, float & maxZ, const MapDimensions & mapDims ) const;

      void              GetAABB( AABB & aabb, int rasterSizeX, int rasterSizeY, const MapDimensions & mapDims ) const;
      void              GetBSphere( D3DXVECTOR3 & sphereCenter, float & sphereRadiusSq, const CDLODQuadTree & quadTree ) const;

      void              DebugDrawAABB( unsigned int penColor, const CDLODQuadTree & quadTree ) const;
      void              FillSubNodes( Node * nodes[4], int & count ) const;

   private:
      void              Create( int x, int y, int size, int level, const CreateDesc & createDesc, Node * allNodesBuffer, int & allNodesBufferLastIndex );

      LODSelectResult   LODSelect( LODSelectInfo & lodSelectInfo, bool parentCompletelyInFrustum ) const;
      void              GetAreaMinMaxHeight( int fromX, int fromY, int toX, int toY, float & minZ, float & maxZ, const CDLODQuadTree & quadTree ) const;

      bool              IntersectRay( const D3DXVECTOR3 & rayOrigin, const D3DXVECTOR3 & rayDirection, float maxDistance, D3DXVECTOR3 & hitPoint, const CDLODQuadTree & quadTree ) const;
   };

                                   
private:

   CreateDesc        m_desc;

   Node *            m_allNodesBuffer;
   int               m_allNodesCount;

   //int               m_nodeMinSize;

   Node ***          m_topLevelNodes;
   int               m_topNodeSize;
   int               m_topNodeCountX;
   int               m_topNodeCountY;

   int               m_leafNodeSize;    // leaf node size

   int               m_rasterSizeX;
   int               m_rasterSizeY;

   float             m_leafNodeWorldSizeX;
   float             m_leafNodeWorldSizeY;

   float             m_LODLevelNodeDiagSizes[c_maxLODLevels];

public:
   CDLODQuadTree();
   virtual ~CDLODQuadTree();

   bool                    Create( const CreateDesc & desc );
   void                    Clean( );

   int                     GetLODLevelCount( ) const                    { return m_desc.LODLevelCount; }

   void                    DebugDrawAllNodes( ) const;

   void                    LODSelect( LODSelection * selectionObj ) const;

   bool                    IntersectRay( const D3DXVECTOR3 & rayOrigin, const D3DXVECTOR3 & rayDirection, float maxDistance, D3DXVECTOR3 & hitPoint ) const;

   void                    GetAreaMinMaxHeight( float fromX, float fromY, float sizeX, float sizeY, float & minZ, float & maxZ ) const;

   int                     GetRasterSizeX() const                             { return m_rasterSizeX; }
   int                     GetRasterSizeY() const                             { return m_rasterSizeY; }

   float                   GetLODLevelNodeDiagonalSize( int LODLevel ) const  { assert( LODLevel >= 0 && LODLevel < m_desc.LODLevelCount ); return m_LODLevelNodeDiagSizes[LODLevel]; }

   const MapDimensions &   GetWorldMapDims() const                            { return m_desc.MapDims; }

};


//////////////////////////////////////////////////////////////////////////
// Inline
//////////////////////////////////////////////////////////////////////////

void inline CDLODQuadTree::Node::GetBSphere( D3DXVECTOR3 & sphereCenter, float & sphereRadiusSq, const CDLODQuadTree & quadTree ) const
{
   const float scaleX = quadTree.m_desc.MapDims.SizeX / (float)(quadTree.m_rasterSizeX-1);
   const float scaleY = quadTree.m_desc.MapDims.SizeY / (float)(quadTree.m_rasterSizeY-1);
   const float scaleZ = quadTree.m_desc.MapDims.SizeZ / 65535.0f;

   float sizeHalfX = this->Size / 2.0f;
   float sizeHalfY = this->Size / 2.0f;
   float sizeHalfZ = (this->MaxZ - this->MinZ) / 2.0f;

   const float midX = this->X + sizeHalfX;
   const float midY = this->Y + sizeHalfY;
   const float midZ = this->MinZ + sizeHalfZ;

   sphereCenter.x = quadTree.m_desc.MapDims.MinX + midX * scaleX;
   sphereCenter.y = quadTree.m_desc.MapDims.MinY + midY * scaleY;
   sphereCenter.z = quadTree.m_desc.MapDims.MinZ + midZ * scaleZ;

   sizeHalfX = sizeHalfX * scaleX;
   sizeHalfY = sizeHalfY * scaleY;
   sizeHalfZ = sizeHalfZ * scaleZ;

   sphereRadiusSq = sizeHalfX*sizeHalfX + sizeHalfY*sizeHalfY + sizeHalfZ * sizeHalfZ;
}
//
void inline CDLODQuadTree::Node::GetWorldMinMaxX( float & minX, float & maxX, int rasterSizeX, const MapDimensions & mapDims ) const
{
   minX   = mapDims.MinX + this->X * mapDims.SizeX / (float)(rasterSizeX-1);
   maxX   = mapDims.MinX + (this->X+this->Size) * mapDims.SizeX / (float)(rasterSizeX-1);
}
//
void inline CDLODQuadTree::Node::GetWorldMinMaxY( float & minY, float & maxY, int rasterSizeY, const MapDimensions & mapDims ) const
{
   minY   = mapDims.MinY + this->Y * mapDims.SizeY / (float)(rasterSizeY-1);
   maxY   = mapDims.MinY + (this->Y+this->Size) * mapDims.SizeY / (float)(rasterSizeY-1);
}
//
void inline CDLODQuadTree::Node::GetWorldMinMaxZ( float & minZ, float & maxZ, const MapDimensions & mapDims ) const
{
   minZ   = mapDims.MinZ + this->MinZ * mapDims.SizeZ / 65535.0f;
   maxZ   = mapDims.MinZ + this->MaxZ * mapDims.SizeZ / 65535.0f;
}
//
void inline CDLODQuadTree::Node::GetAABB( AABB & aabb, int rasterSizeX, int rasterSizeY, const MapDimensions & mapDims ) const
{
   GetWorldMinMaxX( aabb.Min.x, aabb.Max.x, rasterSizeX, mapDims );
   GetWorldMinMaxY( aabb.Min.y, aabb.Max.y, rasterSizeY, mapDims );
   GetWorldMinMaxZ( aabb.Min.z, aabb.Max.z, mapDims );
}
//
void inline CDLODQuadTree::SelectedNode::GetAABB( AABB & aabb, int rasterSizeX, int rasterSizeY, const MapDimensions & mapDims ) const
{
   aabb.Min.x = mapDims.MinX + this->X * mapDims.SizeX / (float)(rasterSizeX-1);
   aabb.Max.x = mapDims.MinX + (this->X+this->Size) * mapDims.SizeX / (float)(rasterSizeX-1);
   aabb.Min.y = mapDims.MinY + this->Y * mapDims.SizeY / (float)(rasterSizeY-1);
   aabb.Max.y = mapDims.MinY + (this->Y+this->Size) * mapDims.SizeY / (float)(rasterSizeY-1);
   aabb.Min.z = mapDims.MinZ + this->MinZ * mapDims.SizeZ / 65535.0f;
   aabb.Max.z = mapDims.MinZ + this->MaxZ * mapDims.SizeZ / 65535.0f;
}
//
inline CDLODQuadTree::SelectedNode::SelectedNode( const Node * node, int LODLevel, bool tl, bool tr, bool bl, bool br )
   : LODLevel(LODLevel), TL(tl), TR(tr), BL(bl), BR(br)
{
   this->X = node->X;
   this->Y = node->Y;
   this->Size = node->Size;
   this->MinZ = node->MinZ;
   this->MaxZ = node->MaxZ;
}
//