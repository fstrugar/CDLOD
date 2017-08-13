//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#pragma once


#include "Common.h"


enum IntersectType
{
   IT_Outside,
   IT_Intersect,
   IT_Inside
};

struct AABB
{
   D3DXVECTOR3       Min;
   D3DXVECTOR3       Max;
   D3DXVECTOR3       Center()   { return (Min + Max) * 0.5f; }
   D3DXVECTOR3       Size()     { return Max - Min; }


   AABB() {}
   AABB( const D3DXVECTOR3 & min, const D3DXVECTOR3 & max ) : Min(min), Max(max)   { }

   void GetCornerPoints( D3DXVECTOR3 corners [] )
   {
      corners[0].x = Min.x;
      corners[0].y = Min.y;
      corners[0].z = Min.z;

      corners[1].x = Min.x;
      corners[1].y = Max.y;
      corners[1].z = Min.z;

      corners[2].x = Max.x;
      corners[2].y = Min.y;
      corners[2].z = Min.z;

      corners[3].x = Max.x;
      corners[3].y = Max.y;
      corners[3].z = Min.z;

      corners[4].x = Min.x;
      corners[4].y = Min.y;
      corners[4].z = Max.z;

      corners[5].x = Min.x;
      corners[5].y = Max.y;
      corners[5].z = Max.z;

      corners[6].x = Max.x;
      corners[6].y = Min.y;
      corners[6].z = Max.z;

      corners[7].x = Max.x;
      corners[7].y = Max.y;
      corners[7].z = Max.z;
   }

   bool Intersects( const AABB & other )
   {
      return !( (other.Max.x < this->Min.x) || (other.Min.x > this->Max.x) 
         || (other.Max.y < this->Min.y) || (other.Min.y > this->Max.y) 
         || (other.Max.z < this->Min.z) || (other.Min.z > this->Max.z) );
   }

   IntersectType TestInBoundingPlanes( const D3DXPLANE planes[] )
   {
      D3DXVECTOR3 corners[9];
      GetCornerPoints(corners);
      corners[8] = Center();

      D3DXVECTOR3 boxSize = Size();
      float size = D3DXVec3Length( &boxSize );

      // test box's bounding sphere against all planes - removes many false tests, adds one more check
      for(int p = 0; p < 6; p++) 
      {
         float centDist = D3DXPlaneDotCoord( &planes[p], &corners[8] );
         if( centDist < -size/2 )
            return IT_Outside;
      }

      int totalIn = 0;
      size /= 6.0f; //reduce size to 1/4 (half of radius) for more precision!! // tweaked to 1/6, more sensible

      // test all 8 corners and 9th center point against the planes
      // if all points are behind 1 specific plane, we are out
      // if we are in with all points, then we are fully in
      for(int p = 0; p < 6; p++) 
      {

         int inCount = 9;
         int ptIn = 1;

         for(int i = 0; i < 9; ++i) 
         {

            // test this point against the planes
            float distance = D3DXPlaneDotCoord( &planes[p], &corners[i] );
            if (distance < -size) 
            {
               ptIn = 0;
               inCount--;
            }
         }

         // were all the points outside of plane p?
         if (inCount == 0) 
         {
            //assert( completelyIn == false );
            return IT_Outside;
         }

         // check if they were all on the right side of the plane
         totalIn += ptIn;
      }

      if( totalIn == 6 )
         return IT_Inside;

      return IT_Intersect;
   }

   float MinDistanceFromPointSq( const D3DXVECTOR3 & point )
   {
      float dist = 0.0f;

      if (point.x < Min.x)
      {
         float d=point.x-Min.x;
         dist+=d*d;
      }
      else
         if (point.x > Max.x)
         {
            float d=point.x-Max.x;
            dist+=d*d;
         }

         if (point.y < Min.y)
         {
            float d=point.y-Min.y;
            dist+=d*d;
         }
         else
            if (point.y > Max.y)
            {
               float d=point.y-Max.y;
               dist+=d*d;
            }

            if (point.z < Min.z)
            {
               float d=point.z-Min.z;
               dist+=d*d;
            }
            else
               if (point.z > Max.z)
               {
                  float d=point.z-Max.z;
                  dist+=d*d;
               }

               return dist;
   }

   float MaxDistanceFromPointSq( const D3DXVECTOR3 & point )
   {
      float dist = 0.0f;
      float k;

      k = ::max( fabsf( point.x - Min.x ), fabsf( point.x - Max.x ) );
      dist += k * k;

      k = ::max( fabsf( point.y - Min.y ), fabsf( point.y - Max.y ) );
      dist += k * k;

      k = ::max( fabsf( point.z - Min.z ), fabsf( point.z - Max.z ) );
      dist += k * k;

      return dist;
   }

   bool	IntersectSphereSq( const D3DXVECTOR3 & center, float radiusSq )
   {
      return MinDistanceFromPointSq( center ) <= radiusSq;
   }

   bool IsInsideSphereSq( const D3DXVECTOR3 & center, float radiusSq )
   {
      return MaxDistanceFromPointSq( center ) <= radiusSq;
   }

   bool IntersectRay( const D3DXVECTOR3 & rayOrigin, const D3DXVECTOR3 & rayDirection, float & distance )
   {
      float tmin = -FLT_MAX;        // set to -FLT_MAX to get first hit on line
      float tmax = FLT_MAX;		   // set to max distance ray can travel

      float _rayOrigin []     = { rayOrigin.x, rayOrigin.y, rayOrigin.z };
      float _rayDirection []  = { rayDirection.x, rayDirection.y, rayDirection.z };
      float _min []           = { Min.x, Min.y, Min.z };
      float _max []           = { Max.x, Max.y, Max.z };

      const float EPSILON = 1e-5f;

      for (int i = 0; i < 3; i++) 
      {
         if ( fabsf(_rayDirection[i]) < EPSILON) 
         {
            // Parallel to the plane
            if( _rayOrigin[i] < _min[i] || _rayOrigin[i] > _max[i] )
            {
               //assert( !k );
               //hits1_false++;
               return false;
            }
         } 
         else 
         {
            float ood = 1.0f / _rayDirection[i];
            float t1 = (_min[i] - _rayOrigin[i]) * ood;
            float t2 = (_max[i] - _rayOrigin[i]) * ood;

            if (t1 > t2) ::swap( t1, t2 );

            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;

            if (tmin > tmax)
            {
               //assert( !k );
               //hits1_false++;
               return false;
            }

         }
      }

      distance = tmin;

      return true;
   }

   static AABB   Enclosing( const AABB & a, const AABB & b )
   {
      D3DXVECTOR3 bmin, bmax;

      bmin.x = ::min( a.Min.x, b.Min.x );
      bmin.y = ::min( a.Min.y, b.Min.y );
      bmin.z = ::min( a.Min.z, b.Min.z );

      bmax.x = ::max( a.Max.x, b.Max.x );
      bmax.y = ::max( a.Max.y, b.Max.y );
      bmax.z = ::max( a.Max.z, b.Max.z );

      return AABB( bmin, bmax );
   }

   bool operator == ( const AABB & b )
   {
      return Min == b.Min && Max == b.Max;
   }

   float BoundingSphereRadius()
   {
      D3DXVECTOR3 size = Size();
      return D3DXVec3Length( &size ) * 0.5f;
   }

   void Expand( float percentage )
   {
      D3DXVECTOR3 offset = Size() * percentage;
      Min -= offset;
      Max += offset;
   }
};


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
inline bool IntersectTri(const D3DXVECTOR3 & _orig, const D3DXVECTOR3 & _dir, const D3DXVECTOR3 & _vert0, const D3DXVECTOR3 & _vert1, 
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