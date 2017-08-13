//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Common.h"

class CascadedVolumeMap
{
public:

   enum SettingsFlags
   {
      SF_IgnoreZ				= (1 << 0),
   };

   struct Settings
   {
      float				      VolumeExtensionMul;				// create bigger volume to accommodate some amount of movement before data needs to be recreated
      float				      MovementPredictionOffsetMul;	// how much of extended space to use (defined by VolumeExtensionMul) to accommodate for the predicted future observer movement when recalculating the volume
      float				      AllowedUpdatesPerFrame;			// how many layers can we update during one frame
      float				      FadeInTime;
      unsigned int		   Flags;


      Settings()			
      { 
         VolumeExtensionMul			   = 1.15f;
         MovementPredictionOffsetMul   = 0.5f;
         AllowedUpdatesPerFrame		   = 2.0f;
         FadeInTime					      = 0.5f;
         Flags						         = 0;
      }
   };

   // single cascade layer
   struct Layer
   {
      D3DXVECTOR3			   BoxMin;
      D3DXVECTOR3			   BoxMax;

      D3DXVECTOR3			   PrevChangeObsPos;
      D3DXVECTOR3			   AvgObsMovementDir;

      Layer()	            { BoxMin = D3DXVECTOR3(0, 0, 0); BoxMax = D3DXVECTOR3(0, 0, 0); PrevChangeObsPos = D3DXVECTOR3(0, 0, 0); AvgObsMovementDir = D3DXVECTOR3(0, 0, 0); }

      virtual bool		   Update( const D3DXVECTOR3 & _observerPos, float newVisibilityRange, CascadedVolumeMap * parent, bool forceUpdate = false )
      {
         D3DXVECTOR3 observerPos = _observerPos;

         const Settings & settings = parent->m_settings;

         D3DXVECTOR3		halfRange = D3DXVECTOR3(newVisibilityRange, newVisibilityRange, newVisibilityRange);
         D3DXVECTOR3		newBoxMin = observerPos - halfRange;
         D3DXVECTOR3		newBoxMax = observerPos + halfRange;

         // if new box is inside current one, everything is fine then, return false
         if( !forceUpdate
            && (newBoxMin.x >= BoxMin.x) && (newBoxMax.x <= BoxMax.x) 
            && (newBoxMin.y >= BoxMin.y) && (newBoxMax.y <= BoxMax.y) 
            && ( ((newBoxMin.z >= BoxMin.z) && (newBoxMax.z <= BoxMax.z)) || (settings.Flags & SF_IgnoreZ) != 0 ) )
            return false;

         AvgObsMovementDir	= AvgObsMovementDir + (observerPos - PrevChangeObsPos);
         D3DXVec3Normalize( &AvgObsMovementDir, &AvgObsMovementDir );
         PrevChangeObsPos	= observerPos;

         observerPos += ((settings.VolumeExtensionMul - 1.0f) * newVisibilityRange * settings.MovementPredictionOffsetMul) * AvgObsMovementDir;

         float a = newVisibilityRange * settings.VolumeExtensionMul;
         halfRange = D3DXVECTOR3(a, a, a);
         BoxMin = observerPos - halfRange;
         BoxMax = observerPos + halfRange;

         return true;
      }
   };

protected:

   Settings				      m_settings;
   Layer **				      m_layersArray;
   int					      m_layerCount;

   double				      m_currentTime;
   float					      m_updateFrameBudget;

   int                     m_lowestUpdated;

   CascadedVolumeMap()		
   { 
      m_layersArray		   = NULL; 
      m_layerCount		   = 0; 
      m_currentTime		   = 0.0f; 
      m_updateFrameBudget  = 1.0f;
      m_lowestUpdated      = 0xFF;
   }
   ~CascadedVolumeMap()	
   { 
      assert( m_layersArray == NULL );
   }

public:

   double                  GetCurrentTime() const           { return m_currentTime; }

protected:

   // Resets all layers to 0 bounding boxes
   virtual void         Reset( )
   {
      for( int i = m_layerCount-1; i >= 0 && m_updateFrameBudget >= 1.0f; i-- )
      {
         Layer * layer = m_layersArray[i];
         layer->BoxMin += layer->BoxMax;
         layer->BoxMin *= 0.5;
         layer->BoxMax = layer->BoxMin;
      }
   }

   virtual int			      Update( float deltaTime, const D3DXVECTOR3 & observerPos, const float visibilityRanges[] )
   {
      m_currentTime += deltaTime;

      m_updateFrameBudget += m_settings.AllowedUpdatesPerFrame;
      if( m_updateFrameBudget > m_settings.AllowedUpdatesPerFrame ) m_updateFrameBudget = m_settings.AllowedUpdatesPerFrame;

#if _DEBUG
      for( int i = 0; i < m_layerCount-1; i++ )
         assert( visibilityRanges[i] < visibilityRanges[i+1] );
      assert( m_settings.VolumeExtensionMul >= 1.0f );
      assert( m_settings.MovementPredictionOffsetMul >= 0.0f && m_settings.MovementPredictionOffsetMul < 1.0f );
      assert( m_settings.AllowedUpdatesPerFrame > 0.0f );
      assert( m_settings.FadeInTime > 0.0f );
#endif


      int numberUpdated = 0;

      // Update layers if needed
      for( int i = m_layerCount-1; i >= 0 && m_updateFrameBudget >= 1.0f; i-- )
      {
         Layer * layer = m_layersArray[i];

         if( layer->Update( observerPos, visibilityRanges[i], this ) )
         {
            numberUpdated ++;
            m_updateFrameBudget -= 1.0f;
         }
         m_lowestUpdated = i;
      }

      return numberUpdated;
   }

};
