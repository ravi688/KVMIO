#pragma once

#include <common/platform.h>

#ifdef PLATFORM_WINDOWS
#	include <kvmio/Win32Window.hpp>
#	define KVMIO_NATIVE_WINDOW_NAME Win32Window
#	define KVMIO_NATIVE_WINDOW kvmio::KVMIO_NATIVE_WINDOW_NAME
#endif

namespace kvmio
{
	class NativeWindow : public KVMIO_NATIVE_WINDOW_NAME
	{
		using KVMIO_NATIVE_WINDOW_NAME::KVMIO_NATIVE_WINDOW_NAME;
	};
}
