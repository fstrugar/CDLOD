//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#pragma once

bool vaProgressWindowClose();
bool vaProgressWindowOpen( const wchar_t * strTitle );
void vaProgressWindowUpdate(float progress);
HWND vaProgressWindowGetHWND();
