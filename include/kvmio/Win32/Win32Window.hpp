#pragma once

#include <kvmio/defines.hpp>
#include <common/defines.h> // for u32

#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#	include <Windows.h>
#endif /* Windows include */

namespace kvmio::Win32
{
	struct WindowPaintInfo
	{
		HDC deviceContext;
		RECT paintRect;
	};

	KVMIO_API HWND Win32CreateWindow(u32 width, u32 height, const char* name, WNDPROC callback);
	KVMIO_API void Win32ShowWindow(HWND handle);
	KVMIO_API void Win32DestroyWindow(HWND handle);
	KVMIO_API void Win32UpdateWindow(HWND handle);
	KVMIO_API void Win32InvalidateRect(HWND handle, const RECT* rect = NULL, bool isEraseBackground = false);
}
