//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// CDLODQuadTree below is the main class for working with CDLOD quadtree. 
// This version has implicit nodes and MinMax maps and uses ~10% of memory
// used by the 'regular' one.
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common.h"

#include "MiniMath.h"

namespace VertexAsylum{ class vaStream; }

//////////////////////////////////////////////////////////////////////////
// Used to store min/max data for quadtree nodes
//////////////////////////////////////////////////////////////////////////
class CDLODMinMaxMap
{
private:
   unsigned short *        m_data;
   unsigned int            m_dataSizeInBytes;
   unsigned char *         m_dataCmp;
   unsigned int            m_dataCmpSizeInBytes;
   unsigned int            m_elementsX;
   unsigned int            m_elementsY;

public:
   CDLODMinMaxMap()         { m_data = NULL; m_dataCmp = NULL; m_dataSizeInBytes = 0; m_dataCmpSizeInBytes = 0; m_elementsX = 0; m_elementsY = 0; }
   ~CDLODMinMaxMap()        { Deinitialize(); }

   void                    Initialize( int elementsX, int elementsY, bool compressed = false );
   void                    Deinitialize();

   void                    GenerateFromHigherDetailed( const CDLODMinMaxMap & source );
   void                    CompressBasedOnLowerDetailed( const CDLODMinMaxMap & source );

   unsigned short *        GetData()                     { return m_data; }
   const unsigned short *  GetData() const               { return m_data; }
   unsigned int            GetDataSizeInBytes() const    { return m_dataSizeInBytes; }
   unsigned char *         GetDataCmp()                  { return m_dataCmp; }
   const unsigned char *   GetDataCmp() const            { return m_dataCmp; }
   unsigned int            GetDataCmpSizeInBytes() const { return m_dataCmpSizeInBytes; }
   unsigned int            GetSizeX() const              { return m_elementsX; }
   unsigned int            GetSizeY() const              { return m_elementsY; }
   bool                    IsCompressed() const          { return m_dataCmp != NULL; }
   
   void                    GetMinMax( int x, int y, unsigned short & outMinZ, unsigned short & outMaxZ ) const;
   void                    GetSubNodesExist( unsigned int parentX, unsigned int parentY, bool & subTL, bool & subTR, bool & subBL, bool & subBR ) const;

   void                    GetMinMaxCmp( int x, int y, unsigned short parentMinZ, unsigned short parentSizeZ, unsigned short & outMinZ, unsigned short & outMaxZ ) const;

   bool                    Save( VertexAsylum::vaStream * outStream );
   bool                    Load( VertexAsylum::vaStream * inStream );
};

inline void CDLODMinMaxMap::GetMinMax( int x, int y, unsigned short & outMinZ, unsigned short & outMaxZ ) const
{
   assert( m_data != NULL );

   outMinZ = m_data[ (x + y * m_elementsX)*2 + 0 ];
   outMaxZ = m_data[ (x + y * m_elementsX)*2 + 1 ];
}

inline void CDLODMinMaxMap::GetSubNodesExist( unsigned int parentX, unsigned int parentY, bool & subTL, bool & subTR, bool & subBL, bool & subBR ) const
{
   unsigned int x = parentX * 2;
   unsigned int y = parentY * 2;

   // for now there are no holes in the quadtree except in the edges, but there could be
   subTL = true;
   subTR = (x+1) < m_elementsX;
   subBL = (y+1) < m_elementsY;
   subBR = ((x+1) < m_elementsX) && ((y+1) < m_elementsY);
}

inline void CDLODMinMaxMap::GetMinMaxCmp( int x, int y, unsigned short parentMinZ, unsigned short parentSizeZ, unsigned short & outMinZ, unsigned short & outMaxZ ) const
{
   assert( m_dataCmp != NULL );

   outMinZ = parentMinZ + m_dataCmp[ (x + y * m_elementsX)*2 + 0 ] * parentSizeZ / 255;
   outMaxZ = parentMinZ + m_dataCmp[ (x + y * m_elementsX)*2 + 1 ] * parentSizeZ / 255;
}

//////////////////////////////////////////////////////////////////////////
// Main class for storing and working with CDLOD quadtree
//////////////////////////////////////////////////////////////////////////
class CDLODQuadTree
{
   // Constants

public:
   static const int     c_maxLODLevels    = 15;

#define CDLODQUADTREE_DEFAULT_MORPH_START_RATIO  (0.70f)

   // Types

public:

   struct CreateDesc
   {
      int               RasterSizeX;
      int               RasterSizeY;
      int               LeafRenderNodeSize;
      int               LODLevelCount;
      float             LODLevelDistanceRatio;

      // Heightmap world dimensions
      MapDimensions     MapDims;

      // an array of LODLevelCount CDLODMinMaxMap-s; this data will not be copied so it must stay alive for the lifetime of CDLODQuadTree object.
      CDLODMinMaxMap*    MinMaxMaps; 
   };

   struct SelectedNode
   {
      unsigned int   X;
      unsigned int   Y;
      unsigned short Size;
      unsigned short MinZ;
      unsigned short MaxZ;

      // these can be flags and can be combined into LODLevel
      bool           TL;
      bool           TR;
      bool           BL;
      bool           BR;
      int            LODLevel;

      float          MinDistToCamera;     // this field is valid only if LODSelection::m_sortByDistance is enabled

      // This is filled in by the streaming system, used to set/retrieve streamed data used for rendering. Unset default value is -1.
      int            StreamingDataID;

      SelectedNode()      {}
      SelectedNode( unsigned int x, unsigned int y, unsigned short size, unsigned short minZ, unsigned short maxZ, int LODLevel, bool tl, bool tr, bool bl, bool br );

      void           GetAABB( AABB & aabb, int rasterSizeX, int rasterSizeY, const MapDimensions & mapDims ) const;
   };

   class QuadEnumeratorCallback
   {
   public:
      virtual bool   OnQuadTouched( const int rasterX, const int rasterY, const int size, const int LODLevel, const unsigned short minZ, const unsigned short maxZ, const AABB & boundingBox ) = 0;
   };

   struct Node;
   class LODSelection
   {
   public:
      enum Flags
      {
         SortByDistance          = ( 1 << 0 ),
         IncludeAllNodesInRange  = ( 1 << 1 ),     // for selection used by streaming
      };

   private:
      friend class CDLODQuadTree;
      friend struct CDLODQuadTree::Node;

      // Input
      SelectedNode *       m_selectionBuffer;
      int                  m_maxSelectionCount;
      D3DXVECTOR3          m_observerPos;
      float                m_visibilityDistance;
      D3DXPLANE            m_frustumPlanes[6];
      float                m_morphStartRatio;                  // [0, 1] - when to start morphing to the next (lower-detailed) LOD level; default is 0.667 - first 0.667 part will not be morphed, and the morph will go from 0.667 to 1.0
      int                  m_stopAtLODLevel;
      Flags                m_flags;
      bool                 m_useFrustumCull;
      float                m_visibilityDistanceFrom;

      // Output 
      const CDLODQuadTree * m_quadTree;
      float                m_visibilityRanges[c_maxLODLevels];
      float                m_morphEnd[c_maxLODLevels];
      float                m_morphStart[c_maxLODLevels];
      int                  m_selectionCount;
      bool                 m_visDistTooSmall;
      int                  m_minSelectedLODLevel;
      int                  m_maxSelectedLODLevel;

      uint32               m_streamingDataHandler;

   public:
      LODSelection( SelectedNode * selectionBuffer, int maxSelectionCount, const D3DXVECTOR3 & observerPos, float visibilityDistance, 
                     D3DXPLANE frustumPlanes[6], float visibilityDistanceFrom = 0, int stopAtLODLevel = 0, 
                     float morphStartRatio = CDLODQUADTREE_DEFAULT_MORPH_START_RATIO, Flags flags = (Flags)0 );
      ~LODSelection();

      const CDLODQuadTree *          GetQuadTree() const              { return m_quadTree; }

      void                          GetMorphConsts( int LODLevel, float consts[4] ) const;

      inline const SelectedNode *   GetSelection() const             { return m_selectionBuffer; }
      inline SelectedNode *         GetSelection()                   { return m_selectionBuffer; }
      inline int                    GetSelectionCount() const        { return m_selectionCount; }

      float                         GetLODDistanceRatio() const      { return m_quadTree->GetLODDistanceRatio(); }

      const float *                 GetLODLevelRanges() const        { return m_morphEnd; }

      // Ugly brute-force mechanism - could be replaced by deterministic one possibly
      bool                          IsVisDistTooSmall() const        { return m_visDistTooSmall; }

      int                           GetMinSelectedLevel() const      { return m_minSelectedLODLevel; }
      int                           GetMaxSelectedLevel() const      { return m_maxSelectedLODLevel; }

      void                          SetStreamingDataHandler( uint32 streamingDataHandler );
      uint32                        GetStreamingDataHandler()        { return m_streamingDataHandler; }
   };

   template< int maxSelectionCount >
   class LODSelectionOnStack : public LODSelection
   {
      SelectedNode      m_selectionBufferOnStack[maxSelectionCount];

   public:
      LODSelectionOnStack( const D3DXVECTOR3 & observerPos, float visibilityDistance, D3DXPLANE frustumPlanes[6], 
                           float visibilityDistanceFrom = 0, int stopAtLODLevel = 0, 
                           float morphStartRatio = CDLODQUADTREE_DEFAULT_MORPH_START_RATIO, Flags flags = (Flags)0 )
         : LODSelection( m_selectionBufferOnStack, maxSelectionCount, observerPos, visibilityDistance, frustumPlanes, 
                           visibilityDistanceFrom, stopAtLODLevel, morphStartRatio, flags )
      { }
   };


   //////////////////////////////////////////////////////////////////////////
   // Quadtree node
   //////////////////////////////////////////////////////////////////////////
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
         int                  LODLevelCount;
         int                  RasterSizeX;
         int                  RasterSizeY;
         MapDimensions        MapDims;
         CDLODMinMaxMap *      MinMaxMap;
      };

      friend class CDLODQuadTree;

      static void       GetWorldMinMaxZ( unsigned short rminZ, unsigned short rmaxZ, float & wminZ, float & wmaxZ, const MapDimensions & mapDims );
      static void       GetAABB( AABB & aabb, int rasterSizeX, int rasterSizeY, const MapDimensions & mapDims, unsigned int x, unsigned int y, unsigned short size, unsigned short minZ, unsigned short maxZ );
      static void       GetBSphere( D3DXVECTOR3 & sphereCenter, float & sphereRadiusSq, const CDLODQuadTree & quadTree, unsigned int x, unsigned int y, unsigned short size, unsigned short minZ, unsigned short maxZ );

   private:
      static LODSelectResult
                        LODSelect( LODSelectInfo & lodSelectInfo, bool parentCompletelyInFrustum, unsigned int x, unsigned int y, unsigned short size, int LODLevel, unsigned short parentMinZ, unsigned short parentSizeZ );

      static void       GetAreaMinMaxHeight( unsigned int fromX, unsigned int fromY, unsigned int toX, unsigned int toY, float & minZ, float & maxZ, const CDLODQuadTree & quadTree, unsigned int x, unsigned int y, unsigned short size, int LODLevel );
      static bool       CoarseIntersectRay( const D3DXVECTOR3 & rayOrigin, const D3DXVECTOR3 & rayDirection, float maxDistance, D3DXVECTOR3 & hitPoint, const CDLODQuadTree & quadTree, unsigned int x, unsigned int y, unsigned short size, int LODLevel, unsigned short parentMinZ, unsigned short parentSizeZ );
      static void       EnumerateQuadsRecursive( QuadEnumeratorCallback * callback, const CDLODQuadTree & quadTree, unsigned int x, unsigned int y, unsigned short size, int LODLevel, unsigned short parentMinZ, unsigned short parentSizeZ );
      static void       EnumerateQuadsRecursive( QuadEnumeratorCallback * callback, const CDLODQuadTree & quadTree, unsigned int x, unsigned int y, 
                                                   unsigned short size, int LODLevel, unsigned short parentMinZ, unsigned short parentSizeZ,
                                                   const D3DXVECTOR3 & boundingSpherePos, float boundingSphereSize );

      Node()            {}
   };

                                   
private:

   CreateDesc        m_desc;

   unsigned short    m_topNodeSize;
   unsigned int      m_topNodeCountX;
   unsigned int      m_topNodeCountY;

   int               m_leafNodeSize;

   int               m_rasterSizeX;
   int               m_rasterSizeY;

   float             m_leafNodeWorldSizeX;
   float             m_leafNodeWorldSizeY;

   float             m_LODLevelNodeDiagSizes[c_maxLODLevels];
   float             m_LODVisRangeDistRatios[c_maxLODLevels];

   CDLODMinMaxMap*    m_MinMaxMaps;

public:
   CDLODQuadTree();
   virtual ~CDLODQuadTree();

   bool                    Create( const CreateDesc & desc );
   void                    Clean( );

   int                     GetLODLevelCount( ) const                    { return m_desc.LODLevelCount; }
   float                   GetLODDistanceRatio() const                  { return m_desc.LODLevelDistanceRatio; }

   const float *           GetLODVisRangeDistRatios( ) const            { return m_LODVisRangeDistRatios; }

   //void                    DebugDrawAllNodes( ) const;

   void                    LODSelect( LODSelection * selectionObj ) const;

   bool                    CoarseIntersectRay( const D3DXVECTOR3 & rayOrigin, const D3DXVECTOR3 & rayDirection, float maxDistance, D3DXVECTOR3 & hitPoint ) const;

   void                    GetAreaMinMaxHeight( float fromX, float fromY, float sizeX, float sizeY, float & minZ, float & maxZ ) const;

   // Heightmap size
   int                     GetRasterSizeX() const                             { return m_rasterSizeX; }
   int                     GetRasterSizeY() const                             { return m_rasterSizeY; }

   float                   GetLODLevelNodeDiagonalSize( int LODLevel ) const  { assert( LODLevel >= 0 && LODLevel < m_desc.LODLevelCount ); return m_LODLevelNodeDiagSizes[LODLevel]; }

   const MapDimensions &   GetWorldMapDims() const                            { return m_desc.MapDims; }

   void                    EnumerateQuadsRecursive( QuadEnumeratorCallback * callback, 
                                                      const int rasterX, const int rasterY, const int rasterSizeX, const int rasterSizeY );
   void                    EnumerateQuadsRecursive( QuadEnumeratorCallback * callback, const D3DXVECTOR3 & boundingSpherePos, float boundingSphereSize );

   unsigned short          GetTopNodeSize() const                             { return m_topNodeSize; }
   unsigned int            GetTopNodeCountX() const                           { return m_topNodeCountX; }
   unsigned int            GetTopNodeCountY() const                           { return m_topNodeCountY; }
   
private:

};


//////////////////////////////////////////////////////////////////////////
// Inline
//////////////////////////////////////////////////////////////////////////

void inline CDLODQuadTree::Node::GetBSphere( D3DXVECTOR3 & sphereCenter, float & sphereRadiusSq, const CDLODQuadTree & quadTree, unsigned int x, unsigned int y, unsigned short size, unsigned short minZ, unsigned short maxZ )
{
   const float scaleX = quadTree.m_desc.MapDims.SizeX / (float)(quadTree.m_rasterSizeX-1);
   const float scaleY = quadTree.m_desc.MapDims.SizeY / (float)(quadTree.m_rasterSizeY-1);
   const float scaleZ = quadTree.m_desc.MapDims.SizeZ / 65535.0f;

   float sizeHalfX = size / 2.0f;
   float sizeHalfY = size / 2.0f;
   float sizeHalfZ = (maxZ - minZ) / 2.0f;

   const float midX = x + sizeHalfX;
   const float midY = y + sizeHalfY;
   const float midZ = minZ + sizeHalfZ;

   sphereCenter.x = quadTree.m_desc.MapDims.MinX + midX * scaleX;
   sphereCenter.y = quadTree.m_desc.MapDims.MinY + midY * scaleY;
   sphereCenter.z = quadTree.m_desc.MapDims.MinZ + midZ * scaleZ;

   sizeHalfX = sizeHalfX * scaleX;
   sizeHalfY = sizeHalfY * scaleY;
   sizeHalfZ = sizeHalfZ * scaleZ;

   sphereRadiusSq = sizeHalfX*sizeHalfX + sizeHalfY*sizeHalfY + sizeHalfZ * sizeHalfZ;
}
//
void inline CDLODQuadTree::Node::GetWorldMinMaxZ( unsigned short rminZ, unsigned short rmaxZ, float & wminZ, float & wmaxZ, const MapDimensions & mapDims )
{
   wminZ   = mapDims.MinZ + rminZ * mapDims.SizeZ / 65535.0f;
   wmaxZ   = mapDims.MinZ + rmaxZ * mapDims.SizeZ / 65535.0f;
}
//
void inline CDLODQuadTree::Node::GetAABB( AABB & aabb, int rasterSizeX, int rasterSizeY, const MapDimensions & mapDims, unsigned int x, unsigned int y, unsigned short size, unsigned short minZ, unsigned short maxZ )
{
   aabb.Min.x   = mapDims.MinX + x * mapDims.SizeX / (float)(rasterSizeX-1);
   aabb.Max.x   = mapDims.MinX + (x+size) * mapDims.SizeX / (float)(rasterSizeX-1);
   aabb.Min.y   = mapDims.MinY + y * mapDims.SizeY / (float)(rasterSizeY-1);
   aabb.Max.y   = mapDims.MinY + (y+size) * mapDims.SizeY / (float)(rasterSizeY-1);

   GetWorldMinMaxZ( minZ, maxZ, aabb.Min.z, aabb.Max.z, mapDims );
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
inline CDLODQuadTree::SelectedNode::SelectedNode( unsigned int x, unsigned int y, unsigned short size, unsigned short minZ, unsigned short maxZ, int LODLevel, bool tl, bool tr, bool bl, bool br )
   : X(x), Y(y), Size((unsigned short)size), MinZ(minZ), MaxZ(maxZ), LODLevel(LODLevel), TL(tl), TR(tr), BL(bl), BR(br)
{
   assert( size < 65535 );
   StreamingDataID = -1;
}
//
