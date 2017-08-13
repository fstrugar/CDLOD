/* 
Avi.cpp

A C++ class for creating AVI files

Copyright (c) 2002 Lucian Wischik.  (99% of Avi.cpp is based on code from
Lucian Wischik)

Copyright (c) 2004, 2005 René Nyffenegger (adding C++ class features)

This source code is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this source code must not be misrepresented; you must not
claim that you wrote the original source code. If you use this source code
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original source code.

3. This notice may not be removed or altered from any source distribution.

Lucian Wischik. 
René Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/

#pragma warning (disable : 4201)

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vfw.h>
#include "Avi.h"

#include <assert.h>

//#include "win32_LastError.h"

#include <stdio.h>

#pragma warning (disable : 4996)
#pragma comment( lib, "Vfw32.lib" )

// First, we'll define the WAV file format.
#include <pshpack1.h>
struct FmtChunk { 
   char id[4];            //="fmt "
   unsigned long size;              //=16
   short wFormatTag;       //=WAVE_FORMAT_PCM=1
   unsigned short wChannels;        //=1 or 2 for mono or stereo
   unsigned long  dwSamplesPerSec;  //=11025 or 22050 or 44100
   unsigned long  dwAvgBytesPerSec; //=wBlockAlign * dwSamplesPerSec
   unsigned short wBlockAlign;      //=wChannels * (wBitsPerSample==8?1:2)
   unsigned short wBitsPerSample;   //=8 or 16, for bits per sample
};

struct DataChunk { 
   char id[4];   //="data"
   unsigned long size;    //=datsize, size of the following array
   unsigned char data[1]; //=the raw data goes here
};

struct WavChunk { 
   char id[4];   //="RIFF"
   unsigned long size;    //=datsize+8+16+4
   char type[4]; //="WAVE"
   FmtChunk      fmt;
   DataChunk     dat;
};

#include <poppack.h>

struct TAviUtil { 
   IAVIFile*     pfile;         // created by CreateAvi
   WAVEFORMATEX  wfx;           // as given to CreateAvi (.nChanels=0 if none was given). Used when audio stream is first created.
   int           period;        // specified in CreateAvi, used when the video stream is first created
   IAVIStream*   as;            // audio stream, initialised when audio stream is first created
   IAVIStream*   ps; 
   IAVIStream*   psCompressed;  // video stream, when first created
   unsigned long nframe, nsamp; // which frame will be added next, which sample will be added next
   bool          iserr;         // if true, then no function will do anything
};

Avi::Avi(const char * file_name, int frame_period, const WAVEFORMATEX* wfx) { 
   IAVIFile *pfile;

   ::AVIFileInit();
   HRESULT hr = ::AVIFileOpenA(&pfile, file_name, OF_WRITE|OF_CREATE, 0);

   if (hr != AVIERR_OK)
   {
      ::AVIFileExit(); 
      throw "AVIERR_OK";
   }

   TAviUtil *au = new TAviUtil;
   au->pfile = pfile;
   if (wfx==0) ::ZeroMemory(&au->wfx,sizeof(WAVEFORMATEX)); else ::CopyMemory(&au->wfx,wfx,sizeof(WAVEFORMATEX));
   au->period = frame_period;
   au->as=0; 
   au->ps=0; 
   au->psCompressed=0;
   au->nframe=0; 
   au->nsamp=0;
   au->iserr=false;

   avi_ = (HANDLE*) au;
}

Avi::~Avi() {
   if (avi_ == 0) return;

   TAviUtil *au = (TAviUtil*)avi_;

   if (au->as!=0) AVIStreamRelease(au->as); au->as=0;
   if (au->psCompressed!=0) AVIStreamRelease(au->psCompressed); au->psCompressed=0;
   if (au->ps!=0) AVIStreamRelease(au->ps); au->ps=0;
   if (au->pfile!=0) AVIFileRelease(au->pfile); au->pfile=0;
   AVIFileExit();
   delete au;
}

HRESULT Avi::compression(HBITMAP hbmp, AVICOMPRESSOPTIONS *opts, bool ShowDialog, HWND hparent) { 
   if (avi_==0) return AVIERR_BADHANDLE;

   DIBSECTION dibs; 
   //::GetObject(hbmp,sizeof(dibs),&dibs);
   printf("Avi::compression going to GetObject 1\n");
   if( ! ::GetObject(hbmp, sizeof(dibs), &dibs)) {
      //printf("GetObject: %s\n", Win32_LastError().c_str());
      assert( false );
      //return -1;
   }
   printf("Avi::compression went to GetObject 1\n");

   TAviUtil *au = (TAviUtil*)avi_;
   if (au->iserr) return AVIERR_ERROR;
   if (au->psCompressed!=0) return AVIERR_COMPRESSOR;

   // create the stream, if it wasn't there before
   if (au->ps==0) { 
      AVISTREAMINFO strhdr; 
      ::ZeroMemory(&strhdr,sizeof(strhdr));
      strhdr.fccType                = streamtypeVIDEO;
      strhdr.fccHandler             = 0; 
      strhdr.dwScale                = au->period;
      strhdr.dwRate                 = 1000;
      strhdr.dwSuggestedBufferSize  = dibs.dsBmih.biSizeImage;
      SetRect(&strhdr.rcFrame, 0, 0, dibs.dsBmih.biWidth, dibs.dsBmih.biHeight);
      HRESULT hr                    = ::AVIFileCreateStream(au->pfile, &au->ps, &strhdr);
      if (hr!=AVIERR_OK) {au->iserr=true; return hr;}
   }
   // set the compression, prompting dialog if necessary
   if (au->psCompressed==0) { 

      AVICOMPRESSOPTIONS myopts; 
      ::ZeroMemory(&myopts,sizeof(myopts));
      AVICOMPRESSOPTIONS *aopts[1];

      if  (opts!=0) aopts[0]=opts; 
      else          aopts[0]=&myopts;

      if (ShowDialog) { 
         BOOL res = (BOOL) ::AVISaveOptions(hparent,0,1,&au->ps,aopts);
         if (!res) {
            ::AVISaveOptionsFree(1,aopts); 
            au->iserr=true; 
            return AVIERR_USERABORT;
         }
      }

      printf("fccType:         %d\n",static_cast<int>(myopts.fccType));
      printf("fccHandler:      %d\n",static_cast<int>(myopts.fccHandler));
      printf("dwKeyFrameEvery: %d\n",static_cast<int>(myopts.dwKeyFrameEvery));
      printf("dwQuality:       %d\n",static_cast<int>(myopts.dwQuality));
      printf("dwFlags          %d\n",static_cast<int>(myopts.dwFlags));
      printf("dwBytesPerSecond %d\n",static_cast<int>(myopts.dwBytesPerSecond));
      //printf("lpFormat:        %d\n",static_cast<int>(myopts.lpFormat));
      printf("cbFormat:        %d\n",static_cast<int>(myopts.cbFormat));
      //printf("lpParms:         %d\n",static_cast<int>(myopts.lpParms ));
      printf("cbParms:         %d\n",static_cast<int>(myopts.cbParms ));
      printf("dwInterleaveEvery%d\n",static_cast<int>(myopts.dwInterleaveEvery ));

      HRESULT hr = ::AVIMakeCompressedStream(&au->psCompressed, au->ps, aopts[0], 0);
      if (hr != AVIERR_OK) {
         au->iserr=true; 
         return hr;
      }

      ::AVISaveOptionsFree(1,aopts);
      printf("Avi::compression after AVISaveOptionsFree\n");

      /*
      DIBSECTION dibs; 
      printf("Avi::compression going to GetObject\n");
      if( !::GetObject(hbmp, sizeof(dibs), &dibs)) {
      printf("GetObject: %s\n", Win32_LastError());
      return -1;
      }
      */

      hr = ::AVIStreamSetFormat(au->psCompressed, 0, &dibs.dsBmih, dibs.dsBmih.biSize+dibs.dsBmih.biClrUsed*sizeof(RGBQUAD));
      if( hr != AVIERR_OK ) {
         au->iserr=true; 
         return hr;
      }
   }
   return AVIERR_OK;
}

HRESULT Avi::add_frame(HBITMAP hBitmap) { 
   if (avi_ == 0) return AVIERR_BADHANDLE;

   DIBSECTION dibs; 
   int sbm = ::GetObject(hBitmap, sizeof(dibs), &dibs);

   if( sbm!=sizeof(DIBSECTION) ) 
      return AVIERR_BADPARAM;

   TAviUtil *au = (TAviUtil*)avi_;

   if( au->iserr ) return AVIERR_ERROR;

   // TODO... Get Width and Height from Bitmap...
   if( au->ps == 0 ) { 
      AVISTREAMINFO strhdr; 
      ::ZeroMemory(&strhdr,sizeof(strhdr));

      strhdr.fccType    = streamtypeVIDEO;
      strhdr.fccHandler = 0; 
      strhdr.dwScale    = au->period;
      strhdr.dwRate     = 1000;
      strhdr.dwSuggestedBufferSize  = dibs.dsBmih.biSizeImage;
      SetRect(&strhdr.rcFrame, 0, 0, dibs.dsBmih.biWidth, dibs.dsBmih.biHeight);

      HRESULT hr= ::AVIFileCreateStream(au->pfile, &au->ps, &strhdr);
      if (hr!=AVIERR_OK) {
         au->iserr=true; 
         return hr;
      }
   }

   // create an empty compression, if the user hasn't set any
   if (au->psCompressed==0) { 
      AVICOMPRESSOPTIONS opts; 
      ::ZeroMemory(&opts,sizeof(opts));
      opts.fccHandler=mmioFOURCC('D','I','B',' '); 
      HRESULT hr = AVIMakeCompressedStream(&au->psCompressed, au->ps, &opts, 0);
      if (hr != AVIERR_OK) {
         au->iserr=true; return hr;
      }

      hr = ::AVIStreamSetFormat(au->psCompressed, 0, &dibs.dsBmih, dibs.dsBmih.biSize+dibs.dsBmih.biClrUsed*sizeof(RGBQUAD));

      if (hr!=AVIERR_OK) {au->iserr=true; return hr;}
   }

   //Now we can add the frame
   HRESULT hr = ::AVIStreamWrite(au->psCompressed, au->nframe, 1, dibs.dsBm.bmBits, dibs.dsBmih.biSizeImage, AVIIF_KEYFRAME, 0, 0);
   if (hr!=AVIERR_OK) {
      au->iserr=true; 
      return hr;
   }
   au->nframe++; return S_OK;
}

HRESULT Avi::add_audio(void *dat, unsigned long numbytes) { 
   if (avi_==0) return AVIERR_BADHANDLE;
   if (dat==0 || numbytes==0) return AVIERR_BADPARAM;
   TAviUtil *au = (TAviUtil*)avi_;
   if (au->iserr) return AVIERR_ERROR;
   if (au->wfx.nChannels==0) return AVIERR_BADFORMAT;
   unsigned long numsamps = numbytes*8 / au->wfx.wBitsPerSample;
   if ((numsamps*au->wfx.wBitsPerSample/8)!=numbytes) return AVIERR_BADPARAM;

   if (au->as==0) { 
      AVISTREAMINFO ahdr; 
      ZeroMemory(&ahdr,sizeof(ahdr));
      ahdr.fccType=streamtypeAUDIO;
      ahdr.dwScale=au->wfx.nBlockAlign;
      ahdr.dwRate=au->wfx.nSamplesPerSec*au->wfx.nBlockAlign; 
      ahdr.dwSampleSize=au->wfx.nBlockAlign;
      ahdr.dwQuality=(DWORD)-1;
      HRESULT hr = ::AVIFileCreateStream(au->pfile, &au->as, &ahdr);
      if (hr!=AVIERR_OK) {au->iserr=true; return hr;}
      hr = ::AVIStreamSetFormat(au->as,0,&au->wfx,sizeof(WAVEFORMATEX));
      if (hr!=AVIERR_OK) {au->iserr=true; return hr;}
   }

   // now we can write the data
   HRESULT hr = ::AVIStreamWrite(au->as,au->nsamp,numsamps,dat,numbytes,0,0,0);
   if (hr!=AVIERR_OK) {
      au->iserr=true; 
      return hr;
   }
   au->nsamp+=numsamps; 
   return S_OK;
}

HRESULT Avi::add_wav(const char* src, DWORD flags) {
   if (avi_==0) return AVIERR_BADHANDLE;
   if (flags!=SND_MEMORY && flags!=SND_FILENAME) return AVIERR_BADFLAGS;
   if (src==0) return AVIERR_BADPARAM;
   TAviUtil *au = (TAviUtil*)avi_;
   if (au->iserr) return AVIERR_ERROR;

   char *buf=0; WavChunk *wav = (WavChunk*)src;
   if (flags==SND_FILENAME)
   { HANDLE hf=CreateFileA(src,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
   if (hf==INVALID_HANDLE_VALUE) {au->iserr=true; return AVIERR_FILEOPEN;}
   DWORD size = GetFileSize(hf,0);
   buf = new char[size];
   DWORD red; ReadFile(hf,buf,size,&red,0);
   CloseHandle(hf);
   wav = (WavChunk*)buf;
   }

   // check that format doesn't clash
   bool badformat=false;
   if (au->wfx.nChannels==0) { 
      au->wfx.wFormatTag=wav->fmt.wFormatTag;
      au->wfx.cbSize=0;
      au->wfx.nAvgBytesPerSec=wav->fmt.dwAvgBytesPerSec;
      au->wfx.nBlockAlign=wav->fmt.wBlockAlign;
      au->wfx.nChannels=wav->fmt.wChannels;
      au->wfx.nSamplesPerSec=wav->fmt.dwSamplesPerSec;
      au->wfx.wBitsPerSample=wav->fmt.wBitsPerSample;
   }
   else { 
      if (au->wfx.wFormatTag!=wav->fmt.wFormatTag) badformat=true;
      if (au->wfx.nAvgBytesPerSec!=wav->fmt.dwAvgBytesPerSec) badformat=true;
      if (au->wfx.nBlockAlign!=wav->fmt.wBlockAlign) badformat=true;
      if (au->wfx.nChannels!=wav->fmt.wChannels) badformat=true;
      if (au->wfx.nSamplesPerSec!=wav->fmt.dwSamplesPerSec) badformat=true;
      if (au->wfx.wBitsPerSample!=wav->fmt.wBitsPerSample) badformat=true;
   }
   if (badformat) {if (buf!=0) delete[] buf; return AVIERR_BADFORMAT;}
   //
   if (au->as==0) // create the stream if necessary
   { AVISTREAMINFO ahdr; ZeroMemory(&ahdr,sizeof(ahdr));
   ahdr.fccType=streamtypeAUDIO;
   ahdr.dwScale=au->wfx.nBlockAlign;
   ahdr.dwRate=au->wfx.nSamplesPerSec*au->wfx.nBlockAlign; 
   ahdr.dwSampleSize=au->wfx.nBlockAlign;
   ahdr.dwQuality=(DWORD)-1;
   HRESULT hr = AVIFileCreateStream(au->pfile, &au->as, &ahdr);
   if (hr!=AVIERR_OK) {if (buf!=0) delete[] buf; au->iserr=true; return hr;}
   hr = AVIStreamSetFormat(au->as,0,&au->wfx,sizeof(WAVEFORMATEX));
   if (hr!=AVIERR_OK) {if (buf!=0) delete[] buf; au->iserr=true; return hr;}
   }

   // now we can write the data
   unsigned long numbytes = wav->dat.size;
   unsigned long numsamps = numbytes*8 / au->wfx.wBitsPerSample;
   HRESULT hr = AVIStreamWrite(au->as,au->nsamp,numsamps,wav->dat.data,numbytes,0,0,0);
   if (buf!=0) delete[] buf;
   if (hr!=AVIERR_OK) {au->iserr=true; return hr;}
   au->nsamp+=numsamps; return S_OK;
}

unsigned int FormatAviMessage(HRESULT code, char *buf,unsigned int len) { 
   const char *msg="unknown avi result code";
   switch (code)
   { case S_OK:                  msg="Success"; break;
   case AVIERR_BADFORMAT:      msg="AVIERR_BADFORMAT: corrupt file or unrecognized format"; break;
   case AVIERR_MEMORY:         msg="AVIERR_MEMORY: insufficient memory"; break;
   case AVIERR_FILEREAD:       msg="AVIERR_FILEREAD: disk error while reading file"; break;
   case AVIERR_FILEOPEN:       msg="AVIERR_FILEOPEN: disk error while opening file"; break;
   case REGDB_E_CLASSNOTREG:   msg="REGDB_E_CLASSNOTREG: file type not recognised"; break;
   case AVIERR_READONLY:       msg="AVIERR_READONLY: file is read-only"; break;
   case AVIERR_NOCOMPRESSOR:   msg="AVIERR_NOCOMPRESSOR: a suitable compressor could not be found"; break;
   case AVIERR_UNSUPPORTED:    msg="AVIERR_UNSUPPORTED: compression is not supported for this type of data"; break;
   case AVIERR_INTERNAL:       msg="AVIERR_INTERNAL: internal error"; break;
   case AVIERR_BADFLAGS:       msg="AVIERR_BADFLAGS"; break;
   case AVIERR_BADPARAM:       msg="AVIERR_BADPARAM"; break;
   case AVIERR_BADSIZE:        msg="AVIERR_BADSIZE"; break;
   case AVIERR_BADHANDLE:      msg="AVIERR_BADHANDLE"; break;
   case AVIERR_FILEWRITE:      msg="AVIERR_FILEWRITE: disk error while writing file"; break;
   case AVIERR_COMPRESSOR:     msg="AVIERR_COMPRESSOR"; break;
   case AVIERR_NODATA:         msg="AVIERR_READONLY"; break;
   case AVIERR_BUFFERTOOSMALL: msg="AVIERR_BUFFERTOOSMALL"; break;
   case AVIERR_CANTCOMPRESS:   msg="AVIERR_CANTCOMPRESS"; break;
   case AVIERR_USERABORT:      msg="AVIERR_USERABORT"; break;
   case AVIERR_ERROR:          msg="AVIERR_ERROR"; break;
   }
   unsigned int mlen=(unsigned int)strlen(msg);
   if (buf==0 || len==0) return mlen;
   unsigned int n=mlen; if (n+1>len) n=len-1;
   strncpy(buf,msg,n); buf[n]=0;
   return mlen;
}
