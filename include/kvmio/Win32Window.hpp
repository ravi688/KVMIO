#pragma once

#include <kvmio/Window.hpp>
#include <kvmio/Win32/Win32.hpp>

#include <common/Event.hpp>

#include <bufferlib/buffer.h>

#ifdef PLATFORM_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <Windows.h>
#endif // PLATFORM_WINDOWS

#include <unordered_map>
#include <vector>
#include <atomic>
#include <memory>

namespace kvmio
{
	#ifdef PLATFORM_WINDOWS
		typedef HWND Internal_WindowHandle;
		typedef MSG Internal_MSG;
		typedef HHOOK Internal_HookHandle;
		typedef LRESULT (*Internal_HookCallback)(int code, WPARAM wParam, LPARAM lParam);
	#endif // PLATFORM_WINDOWS

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	class KVMIO_API Win32Window : public Window
	{
		friend LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	private:
		typedef std::vector<Win32::KeyCode> KeyComb;
		typedef std::vector<Win32::KeyboardInput> KeyInputComb;
	private:
		Internal_WindowHandle m_handle;
		Internal_MSG m_msg;
		std::unique_ptr<Win32::Win32DrawSurface> m_drawSurface;
		bool m_isMessageAvailable;
		u32 m_width;
		u32 m_height;
		u32 m_clientWidth;
		u32 m_clientHeight;
		buffer_t m_rawInputBuffer;
		RECT m_saveClipRect;
		RECT m_newClipRect;
		bool m_isFullScreen;
		struct
		{
			BOOL isZoomed;
			LONG style;
			LONG exStyle;
			RECT windowRect;
		} m_beforeFullScreenInfo;

		std::unordered_map<u32, Win32::KeyStatus> m_pressedKeys;
		std::vector<std::pair<KeyComb, com::Event<com::no_publish_ptr_t, KeyInputComb>>> m_keyCombs;
		std::vector<Win32::KeyboardInput> m_curKeyComb;
		bool m_isLocked;
		PaintCallback m_paintCallback;
		FrameFormat m_frameFormat;

		com::Event<com::no_publish_ptr_t, Win32::MouseInput> m_mouseEvent;
		com::Event<com::no_publish_ptr_t, Win32::KeyboardInput>  m_keyboardEvent;

	public:
		typedef Internal_HookHandle HookHandle;
		typedef Internal_HookCallback HookCallback;
		enum class HookType
		{
			Keyboard,
			KeyboardLowLevel,
			Mouse,
			MouseLowLevel
		};

		typedef HRGN RegionHandle;

	public:
		Win32Window(u32 width, u32 height, std::string_view title);

		// Not copyable and not movable
		Win32Window(Win32Window&) = delete;
		Win32Window(Win32Window&&) = delete;

		~Win32Window();

		// Implementation of Window
		virtual bool isShouldClose() override { return shouldClose(); }
		virtual void setFrameFormat(FrameFormat frameFormat) override { m_frameFormat = frameFormat; }
		virtual void setFrameDisplayCallback(const PaintCallback& callback) override { m_paintCallback = callback; }
		virtual void show() override;
		virtual void runGameLoop() override;
		virtual void runGameLoop(u32 frameRate, const Predicate& isLoop = [] { return true; }) override;
	
		Internal_WindowHandle getNativeHandle() { return m_handle; }
	

		bool isLocked() const noexcept { return m_isLocked; }
		bool isFullScreen() const noexcept { return m_isFullScreen; }
		bool shouldClose(bool isBlock = true);
		void lock(bool isLock) { showCursor(!isLock); }
		void setFullScreen(bool isFullScreen);
		void showCursor(bool isShow);
		void pollEvents();
		void setMouseCapture();
		void releaseMouseCapture();
		void setSize(u32 width, u32 height);
		std::pair<u32, u32> getSize() const;
		u32 getWidth() const { return getSize().first; }
		u32 getHeight() const { return getSize().second; }
		std::pair<u32, u32> getClientSize() const;
		u32 getClientWidth() const { return getClientSize().first; }
		u32 getClientHeight() const { return getClientSize().second; }
		void setPosition(s32 x, s32 y);
		void setSizeAndPosition(s32 x, s32 y, u32 width, u32 height);
		void setZOrder(HWND insertAfter);
		void invalidateRect(const RECT* rect = NULL, bool isEraseBackground = false);
		void update();
		void redraw(RegionHandle& regionHandle, s32 x, s32 y, s32 width, s32 height);

		HookHandle installLocalHook(HookType hookType, HookCallback callback);
		void uninstallLocalHook(HookHandle hookHandle);

		decltype(auto) getMouseEvent() { return m_mouseEvent; }
		decltype(auto) getKeyboardEvent() { return m_keyboardEvent; }
		com::Event<com::no_publish_ptr_t, KeyInputComb>& createKeyCombinationEvent(const KeyComb& keyComb);
	};
}
