/* 
Avi.h

A c++ class for creating avi files

Copyright (c) 2002 Lucian Wischik.  (99% of avi.h is based on code from
Lucian Wischik)

Copyright (c) 2004,2005 René Nyffenegger (adding class/c++ features)

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

#include <windows.h>
#include <vfw.h>
#include <string>

#ifndef avi_h__
#define avi_h__

class Avi {
public:
   // Avi - use constructor to start the creation of the avi file.
   // The period is the number of ms between each bitmap frame.
   // The waveformat can be null if you're not going to add any audio,
   // or if you're going to add audio from a file.
   Avi(const char * file_name, int frameperiod, const WAVEFORMATEX* wfx);
   ~Avi();

   // add_frame - adds this bitmap to the AVI file. hbm must point be a DIBSection.
   // It is the callers responsibility to free the hbm.
   // AddAviAudio - adds this junk of audio. The format of audio was as specified in the
   // wfx parameter to CreateAVI. This fails if NULL was given.
   // Both return S_OK if okay, otherwise one of the AVI errors.
   HRESULT add_frame(HBITMAP);

   HRESULT add_audio(void* dat, unsigned long numbytes);

   // compression - allows compression of the video. If compression is desired,
   // then this function must have been called before any bitmap frames had been added.
   // The bitmap hbm must be a DIBSection (so that Avi knows what format/size you're giving it),
   // but won't actually be added to the movie.
   // This function can display a dialog box to let the user choose compression. In this case,
   // set ShowDialog to true and specify the parent window. If opts is non-NULL and its
   // dwFlags property includes AVICOMPRESSF_VALID, then opts will be used to give initial
   // values to the dialog. If opts is non-NULL then the chosen options will be placed in it.
   // This function can also be used to choose a compression without a dialog box. In this
   // case, set ShowDialog to false, and hparent is ignored, and the compression specified
   // in 'opts' is used, and there's no need to call GotAviVideoCompression afterwards.
   HRESULT compression(HBITMAP, AVICOMPRESSOPTIONS *opts, bool ShowDialog, HWND hparent);

   // add_wav - a convenient way to add an entire wave file to the Avi.
   // The wav file may be in in memory (in which case flags=SND_MEMORY)
   // or a file on disk (in which case flags=SND_FILENAME).
   // This function requires that either a null WAVEFORMATEX was passed to CreateAvi,
   // or that the wave file now being added has the same format as was
   // added earlier.
   HRESULT add_wav(const char* src, DWORD flags);

private:
   HANDLE* avi_;
};

unsigned int FormatAviMessage(HRESULT code, char *buf,unsigned int len);
// FormatAviMessage - given an error code, formats it as a string.
// It returns the length of the error message. If buf/len points
// to a real buffer, then it also writes as much as possible into there.

#endif
