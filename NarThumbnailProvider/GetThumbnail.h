#pragma once

#include <windows.h>
#include <wincodec.h>

#pragma comment(lib, "windowscodecs.lib")

HBITMAP GetNARThumbnail(IStream* stream);
