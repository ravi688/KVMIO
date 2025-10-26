#pragma once

#include <kvmio/defines.hpp>
#include <kvmio/types.hpp> // for kvmio::FrameFormat

#include <cstddef> // for std::byte
#include <span> // for std::span<>
#include <string_view> // for std::string_view
#include <functional> // for std::functicon

namespace kvmio
{
	class KVMIO_API Window
	{
	public:
		typedef std::function<std::span<const std::byte>(void)> PaintCallback;
		typedef std::function<bool(void)> Predicate;

	public:
		virtual ~Window() = default;

		virtual bool isShouldClose() = 0;
		virtual void setFrameFormat(FrameFormat frameFormat) = 0;
		virtual void setFrameDisplayCallback(const PaintCallback& callback) = 0;
		virtual void setFullScreen(bool isFullScreen) = 0;
		virtual void show() = 0;
		virtual void runGameLoop() = 0;
		virtual void runGameLoop(u32 frameRate, const Predicate& isLoop = [] { return true; }) = 0;
	};
}
