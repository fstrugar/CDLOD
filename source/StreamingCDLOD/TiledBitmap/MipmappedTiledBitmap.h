//////////////////////////////////////////////////////////////////////
// Copyright (C) 2007 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#ifndef __MipmappedTiledBitmap_H__
#define __MipmappedTiledBitmap_H__

#include <stdio.h>

#include <queue>

namespace VertexAsylum
{

   /*
   /// <summary>
   /// MipMappedTiledBitmap is a collection of TiledBitmap images, representing the main image and its
   /// mip map images. 
   /// </summary>
   public class MipMappedTiledBitmap : IDisposable
   {
      public const int                 FormatVersion     = 1;
      public const int                 TotalHeaderSize   = 256;

      private Stream                   file;

      private bool                     readOnly;

      private TiledBitmap[]            mipLevels;
      //private long                     mipOffsets;

      public readonly TiledBitmapPixelFormat PixelFormat;
      public readonly int              Width;
      public readonly int              Height;

      public TiledBitmap[]             MipLevels      { get { return mipLevels; } }
      public bool                      Closed         { get { return file == null; } }

      protected MipMappedTiledBitmap( Stream file, TiledBitmapPixelFormat pixelFormat, int width, int height, TiledBitmap[] mipLevels, bool readOnly )
      {

         this.file = file;
         this.PixelFormat = pixelFormat;
         //this.BytesPerPixel = GetPixelFormatBPP( pixelFormat );
         this.Width = width;
         this.Height = height;
         this.mipLevels = mipLevels;
         this.readOnly = readOnly;
      }

      ~MipMappedTiledBitmap()
      {
         Close();
      }

      public bool HasAllMipLevels()
      {
         return CalcMaxMipLevels( this.Width, this.Height ) == this.MipLevels.Length;
      }

      private static int CalcMaxMipLevels( int width, int height )
      {
         if( width <= 0 || height <= 0 ) throw new Exception("");
         int levels = 1;
         while( width != 1 && height != 1 )
         {
            levels++;
            width = Math.Max( 1, width / 2 );
            height = Math.Max( 1, height / 2 );
         }
         return levels;
      }

      /// <summary>
      /// Create new MipMappedTiledBitmap
      /// </summary>
      public static MipMappedTiledBitmap Create( string path, TiledBitmapPixelFormat pixelFormat, int width, int height, int mipLevelCount )
      {
         FileStream file;
         file = File.Open( path, FileMode.Create, FileAccess.ReadWrite, FileShare.None );

         if( mipLevelCount == 0 )
            mipLevelCount = CalcMaxMipLevels(width, height);
         else if( mipLevelCount < 0 || mipLevelCount > CalcMaxMipLevels(width, height) )
            throw new Exception("Invalid number of mip levels");

         long size = TotalHeaderSize;
         long[] offsets = new long[mipLevelCount];
         long[] sizes   = new long[mipLevelCount];
         int currWidth  = width; int currHeight = height;
         for( int i = 0; i < mipLevelCount; i++ )
         {
            offsets[i]  = size;
            sizes[i]    = TiledBitmap.CalcSize(pixelFormat, currWidth, currHeight );
            size += sizes[i];
            currWidth   = Math.Max( 1, currWidth / 2 ); currHeight  = Math.Max( 1, currHeight / 2 );
         }

         file.SetLength(size);
         file.Position = 0; 

         TiledBitmap.WriteInt32( file, 1 );       // Write version
         TiledBitmap.WriteInt32( file, (int)pixelFormat );
         TiledBitmap.WriteInt32( file, mipLevelCount );
         TiledBitmap.WriteInt32( file, width );
         TiledBitmap.WriteInt32( file, height );

         TiledBitmap[] mipLevels = new TiledBitmap[mipLevelCount];
         currWidth  = width; currHeight = height;
         for( int i = 0; i < mipLevelCount; i++ )
         {
            SubStream subStream = new SubStream( file, offsets[i], sizes[i] );
            mipLevels[i] = TiledBitmap.Create(subStream, pixelFormat, currWidth, currHeight);
            currWidth   = Math.Max( 1, currWidth / 2 ); currHeight  = Math.Max( 1, currHeight / 2 );
         }

         return new MipMappedTiledBitmap( file, pixelFormat, width, height, mipLevels, false );
      }

      /// <summary>
      /// Open existing MipMappedTiledBitmap
      /// </summary>
      public static MipMappedTiledBitmap Open( string path, bool readOnly )
      {
         FileStream file;
         if( readOnly )
            file = File.Open( path, FileMode.Open, FileAccess.Read, FileShare.Read );
         else
            file = File.Open( path, FileMode.Open, FileAccess.ReadWrite, FileShare.None );

         int version = TiledBitmap.ReadInt32( file );
         if( version != 1 )
            throw new Exception("Invalid version");

         TiledBitmapPixelFormat pixelFormat = (TiledBitmapPixelFormat)TiledBitmap.ReadInt32( file );
         int mipLevelCount                   = TiledBitmap.ReadInt32( file );
         int width                           = TiledBitmap.ReadInt32( file );
         int height                          = TiledBitmap.ReadInt32( file );

         if( mipLevelCount <= 0 || mipLevelCount > 1024 )
            throw new Exception("File corrupt");

         long size = TotalHeaderSize;
         long[] offsets = new long[mipLevelCount];
         long[] sizes = new long[mipLevelCount];
         int currWidth = width; int currHeight = height;
         for( int i = 0; i < mipLevelCount; i++ )
         {
            offsets[i] = size;
            sizes[i] = TiledBitmap.CalcSize( pixelFormat, currWidth, currHeight );
            size += sizes[i];
            currWidth = Math.Max( 1, currWidth / 2 ); currHeight = Math.Max( 1, currHeight / 2 );
         }

         if( file.Length != size )
            throw new Exception("File corrupt");

         TiledBitmap[] mipLevels = new TiledBitmap[mipLevelCount];
         currWidth = width; currHeight = height;
         for( int i = 0; i < mipLevelCount; i++ )
         {
            SubStream subStream = new SubStream( file, offsets[i], sizes[i] );
            mipLevels[i] = TiledBitmap.Open( subStream, readOnly );
            currWidth = Math.Max( 1, currWidth / 2 ); currHeight = Math.Max( 1, currHeight / 2 );
         }

         return new MipMappedTiledBitmap( file, pixelFormat, width, height, mipLevels, readOnly );
      }

      public void Close()
      {
         if( Closed ) return;

         if( mipLevels != null )
         {
            for( int i = 0; i < mipLevels.Length; i++ )
               mipLevels[i].Close();
            mipLevels = null; 
         }

         file.Close();
         file = null;
      }
   }
   */

}


#endif __MipmappedTiledBitmap_H__
