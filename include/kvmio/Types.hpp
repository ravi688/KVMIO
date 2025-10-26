#pragma once

#include <common/defines.h> // for u8

namespace kvmio
{
	enum class FrameFormat : u8
	{
		RGB,
		// YUV 4:2:0
		NV12,
		// YUV 4:2:2
		YUYV
	};

	enum class WindowEventType : u8
	{
		KeyboardInput,
		MouseInput,
		Resize,
		MAX
	};
}

namespace kvmio::Win32
{
	
}
