#pragma once

#include <kvmio/defines.hpp>
#include <kvmio/types.hpp> // for kvmio::FrameFormat

#include <span> // for std::span<>
#include <string_view> // for std::string_view
#include <functional> // for std::functicon

namespace kvmio
{
	class KVMIO_API Window
	{
	public:
		typedef std::function<bool(void)> Predicate;

	private:
		FrameFormat m_frameFormat;

	protected:
		FrameFormat getFrameFormat() const { return m_frameFormat; }

	public:
		Window() : m_frameFormat(FrameFormat::NV12) { }
		virtual ~Window() = default;

		virtual void setFrameFormat(FrameFormat frameFormat) { m_frameFormat = frameFormat; }

		// Pure virtual functions
		virtual bool isShouldClose() = 0;
		virtual void setFullScreen(bool isFullScreen) = 0;
		virtual void show() = 0;
		virtual void runGameLoop() = 0;
		virtual void runGameLoop(u32 frameRate, const Predicate& isLoop = [] { return true; }) = 0;
		// Thread-safe, can be called from another thread, i.e. runGameLoop() can be a different thread than this.
		virtual void present(std::span<const u8> frameData) = 0;
	};
}
