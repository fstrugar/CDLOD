///////////////////////////////////////////////////////////////////////////////////////////////////
// VertexAsylum Codebase, Copyright (c) Filip Strugar.
// Contents of this file are distributed under Zlib license.
// (http://en.wikipedia.org/wiki/Zlib_License)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Core/vaCore.h"

#include "vaCommonWin.h"

namespace VertexAsylum
{
   typedef CRITICAL_SECTION      vaCriticalSectionObjectPlatformType;

   typedef HANDLE                vaThreadingSyncObjectHandle;

   typedef HANDLE                vaThreadObjectPlatformType;
}