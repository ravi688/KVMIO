#include <kvmio/Win32Window.hpp>
#include <kvmio/Win32/Win32.hpp>
#include <kvmio/ErrorHandling.hpp>

#include <bufferlib/buffer.h>

#include <libassert/assert.hpp>

#include <chrono>

namespace kvmio
{
	static std::unordered_map<HWND, Win32Window*> gWindowsSelfReferenceRegistry;

	static int GetWin32HookFromHookType(Win32Window::HookType hookType);

	// Returns pointer to a Win32Window instance given its handle
	static Win32Window* getWindowPtr(HWND windowHandle)
	{
		auto it = gWindowsSelfReferenceRegistry.find(windowHandle);
		if(it != gWindowsSelfReferenceRegistry.end())
			return it->second;
		return nullptr;
	}


	Win32Window::Win32Window(u32 width, u32 height, std::string_view name) : 
											m_isMessageAvailable(false),
											m_width(width),
											m_height(height),
											m_isFullScreen(false),
											m_isLocked(false),
											m_frameFormat(FrameFormat::NV12)
	{
		m_handle = Win32::Win32CreateWindow(width, height, std::string { name }.c_str(), WindowProc);
		setSize(width, height);
		setPosition(0, 0);
		setZOrder(HWND_TOP);

		m_drawSurface = std::make_unique<Win32::Win32DrawSurface>(m_handle, width, height, 32u);

		m_rawInputBuffer = buf_create(sizeof(u8), sizeof(RAWINPUT), 0);

		gWindowsSelfReferenceRegistry.insert({ m_handle, this });

		GetClipCursor(&m_saveClipRect);
	}

	Win32Window::~Win32Window()
	{
		lock(false);
		Win32::Win32DestroyWindow(m_handle);
		gWindowsSelfReferenceRegistry.erase(m_handle);
		buf_free(&m_rawInputBuffer);
	}

	
	void Win32Window::runGameLoop()
	{
		while(!shouldClose(false))
		{
			invalidateRect();
			pollEvents();
		}
	}

	void Win32Window::runGameLoop(u32 frameRate, const std::function<bool(void)>& isLoop)
	{
		const f64 deltaTime = 1000.0 / frameRate;
		auto startTime = std::chrono::high_resolution_clock::now();
		while(isLoop() && (!shouldClose(false)))
		{
			auto time = std::chrono::high_resolution_clock::now();
			if(std::chrono::duration_cast<std::chrono::milliseconds>(time - startTime).count() >= deltaTime)
			{
				invalidateRect();	
				startTime = time;
			}
			
			pollEvents();
		}
	}

	bool Win32Window::shouldClose(bool isBlock)
	{
		if(isBlock)
		{
			BOOL result = GetMessage(&m_msg, NULL, 0, 0);
			if(result == 0)
			{
				DestroyWindow(m_handle);
				return true;
			}
			else if(result == -1)
			{
				/* Invoke the error handler here*/
				/* But for now let's exit */
				exit(-1);
			}
			m_isMessageAvailable = true;
		}
		else if(PeekMessage(&m_msg, NULL, 0, 0, PM_REMOVE) != 0)
		{
			m_isMessageAvailable = true;
			if(m_msg.message == WM_QUIT)
			{
				DestroyWindow(m_handle);
				return true;
			}
		}
		return false;
	}

	void Win32Window::show()
	{
		Win32::Win32ShowWindow(m_handle);
		// if(isFullScreen)
		// {
		// 	setFullScreen(true);
		// 	if(!isLocked())
		// 		lock(true);
		// }
	}

	void Win32Window::setFullScreen(bool isFullScreen)
	{
		// Below code is taken from: https://src.chromium.org/viewvc/chrome/trunk/src/ui/views/win/fullscreen_handler.cc?revision=HEAD&view=markup

		// Save current window state if not already fullscreen.
		if (!m_isFullScreen)
		{
			// Save current window information.  We force the window into restored mode
			// before going fullscreen because Windows doesn't seem to hide the
			// taskbar if the window is in the maximized state.
			m_beforeFullScreenInfo.isZoomed = !!::IsZoomed(m_handle);
			if (m_beforeFullScreenInfo.isZoomed)
      			::SendMessage(m_handle, WM_SYSCOMMAND, SC_RESTORE, 0);
    		m_beforeFullScreenInfo.style = GetWindowLong(m_handle, GWL_STYLE);
    		m_beforeFullScreenInfo.exStyle = GetWindowLong(m_handle, GWL_EXSTYLE);
    		GetWindowRect(m_handle, &m_beforeFullScreenInfo.windowRect);
  		}

  		m_isFullScreen = isFullScreen;

  		if (m_isFullScreen)
  		{
    		// Set new window style and size.
    		SetWindowLong(m_handle, GWL_STYLE, m_beforeFullScreenInfo.style & ~(WS_CAPTION | WS_THICKFRAME));
    		SetWindowLong(m_handle, GWL_EXSTYLE, m_beforeFullScreenInfo.exStyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));

			// On expand, if we're given a window_rect, grow to it, otherwise do
			// not resize.
			// if (!for_metro)
			{
				MONITORINFO monitor_info;
				monitor_info.cbSize = sizeof(monitor_info);
				GetMonitorInfo(MonitorFromWindow(m_handle, MONITOR_DEFAULTTONEAREST), &monitor_info);
				RECT rect = monitor_info.rcMonitor;
				SetWindowPos(m_handle, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    		}
		}
		else
		{
			// Reset original window style and size.  The multiple window size/moves
			// here are ugly, but if SetWindowPos() doesn't redraw, the taskbar won't be
			// repainted.  Better-looking methods welcome.
			SetWindowLong(m_handle, GWL_STYLE, m_beforeFullScreenInfo.style);
			SetWindowLong(m_handle, GWL_EXSTYLE, m_beforeFullScreenInfo.exStyle);

			// if (!for_metro)
			{
				// On restore, resize to the previous saved rect size.
				RECT rect = m_beforeFullScreenInfo.windowRect;
				SetWindowPos(m_handle, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
			}

			if (m_beforeFullScreenInfo.isZoomed)
				::SendMessage(m_handle, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
		}
	}

	void Win32Window::showCursor(bool isShow)
	{
		m_isLocked = !isShow;
		if(isShow)
		{
			ClipCursor(&m_saveClipRect);
			ShowCursor(TRUE);
		}
		else
		{
			ShowCursor(FALSE);
			GetClientRect(m_handle, &m_newClipRect);
			RECT winRect;
			GetWindowRect(m_handle, &winRect);

			winRect.right = m_newClipRect.right + winRect.left;
			winRect.bottom = m_newClipRect.bottom + winRect.top;

			ClipCursor(&winRect);
		}
	}

	void Win32Window::pollEvents()
	{
		if(!m_isMessageAvailable)
			return;
		m_isMessageAvailable = false;
		TranslateMessage(&m_msg);
		DispatchMessage(&m_msg);
	}

	void Win32Window::setMouseCapture()
	{
		SetCapture(m_handle);
	}

	void Win32Window::releaseMouseCapture()
	{
		if(ReleaseCapture() == 0)
			kvmio_Internal_ErrorExit("ReleaseCapture");
	}

	void Win32Window::setSize(u32 width, u32 height)
	{
		if(SetWindowPos(m_handle, 0, 0, 0, static_cast<int>(width), static_cast<int>(height), SWP_NOMOVE) == 0)
			kvmio_Internal_ErrorExit("SetWindowPos");
		else
		{
			m_width = width;
			m_height = height;
			RECT rect { };
			BOOL result = GetClientRect(m_handle, &rect);
			if(result == 0)
				kvmio_Internal_ErrorExit("AdjustWindowRect");
			m_clientWidth = rect.right;
			m_clientHeight = rect.bottom;
		}
	}

	std::pair<u32, u32> Win32Window::getSize() const
	{
		return { m_width, m_height };
	}

	void Win32Window::setPosition(s32 x, s32 y)
	{
		if(SetWindowPos(m_handle, 0, x, y, 0, 0, SWP_NOSIZE) == 0)
			kvmio_Internal_ErrorExit("SetWindowPos");
	}

	void Win32Window::setSizeAndPosition(s32 x, s32 y, u32 width, u32 height)
	{
		if(SetWindowPos(m_handle, HWND_TOP, x, y, static_cast<int>(width), static_cast<int>(height), SWP_NOZORDER) == 0)
			kvmio_Internal_ErrorExit("SetWindowPos");
	}

	std::pair<u32, u32> Win32Window::getClientSize() const
	{
		return { m_clientWidth, m_clientHeight };
	}

	void Win32Window::setZOrder(HWND insertAfter)
	{
		if(SetWindowPos(m_handle, insertAfter, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE) == 0)
			kvmio_Internal_ErrorExit("SetWindowPos");
	}

	void Win32Window::invalidateRect(const RECT* rect, bool isEraseBackground)
	{
		Win32::Win32InvalidateRect(m_handle, rect, isEraseBackground);
	}

	void Win32Window::update()
	{
		Win32::Win32UpdateWindow(m_handle);
	}

	void Win32Window::redraw(RegionHandle& regionHandle, s32 x, s32 y, s32 width, s32 height)
	{
		RECT updateRect = { x, y, x + width, y + height };
		if(RedrawWindow(m_handle, &updateRect, regionHandle, 0) == 0)
			kvmio_Internal_ErrorExit("RedrawWindow");
	}

	Win32Window::HookHandle Win32Window::installLocalHook(HookType hookType, HookCallback callback)
	{
		HHOOK hook;
		if((hook = SetWindowsHookExA(GetWin32HookFromHookType(hookType), callback, NULL, GetCurrentThreadId())) == NULL)
			kvmio_Internal_ErrorExit("SetWindowsHookExA");
		return hook;
	}

	void Win32Window::uninstallLocalHook(HookHandle hookHandle)
	{
		if(UnhookWindowsHookEx(static_cast<HHOOK>(hookHandle)) == 0)
			kvmio_Internal_ErrorExit("UnhookWindowsHookEx");
	}

	com::Event<com::no_publish_ptr_t, Win32Window::KeyInputComb>& Win32Window::createKeyCombinationEvent(const KeyComb& keyComb)
	{
		std::size_t index = m_keyCombs.size();
		m_keyCombs.push_back({ keyComb, com::Event<com::no_publish_ptr_t, KeyInputComb>() });
		return m_keyCombs[index].second;
	}

	LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		Win32Window* window = getWindowPtr(hwnd);
		if(!window)
			return DefWindowProc(hwnd, uMsg, wParam, lParam);

		switch (uMsg)
    	{
			case WM_SIZE:
			{
				int width = LOWORD(lParam);  // Macro to get the low-order word.
				int height = HIWORD(lParam); // Macro to get the high-order word.
				window->m_width = width;
				window->m_height = height;
				RECT rect { };
				BOOL result = GetClientRect(hwnd, &rect);
				if(result == 0)
					kvmio_Internal_ErrorExit("AdjustWindowRect");
				window->m_clientWidth = rect.right;
				window->m_clientHeight = rect.bottom;
				if(window->isLocked())
				{
					RECT winRect;
					GetWindowRect(hwnd, &winRect);

					winRect.right = rect.right + winRect.left;
					winRect.bottom = rect.bottom + winRect.top;

					ClipCursor(&winRect); 
				}
				break;
			}

			case WM_INPUT:
			{
				UINT bufferSize;
				if(GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &bufferSize, sizeof(RAWINPUTHEADER)) == ((UINT)-1))
					kvmio_Internal_ErrorExit("GetRawinputData");

				if(buf_get_capacity(&window->m_rawInputBuffer) < bufferSize)
					buf_resize(&window->m_rawInputBuffer, bufferSize);

				if(GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buf_get_ptr(&window->m_rawInputBuffer), &bufferSize, sizeof(RAWINPUTHEADER)) != bufferSize)
					kvmio_Internal_ErrorExit("GetRawInputData");

				RAWINPUT* rawInput = reinterpret_cast<RAWINPUT*>(buf_get_ptr(&window->m_rawInputBuffer));

				switch(rawInput->header.dwType)
				{
					case RIM_TYPEMOUSE:
					{
						RAWMOUSE* rawMouse = &rawInput->data.mouse;
						Win32::MouseInput mouseInput = Win32::DecodeRawMouseInput(rawMouse);
						window->getMouseEvent().publish(mouseInput);
						break;
					}

					case RIM_TYPEKEYBOARD:
					{
						RAWKEYBOARD* rawKeyboard = &rawInput->data.keyboard;
						Win32::KeyboardInput keyboardInput = Win32::DecodeRawKeyboardInput(rawKeyboard);
						std::vector<Win32::KeyboardInput>& curKeyComb = window->m_curKeyComb;
						auto& m_pressedKeys = window->m_pressedKeys;
						if(keyboardInput.keyStatus == Win32::KeyStatus::Pressed)
						{
							if(m_pressedKeys.find(keyboardInput.makeCode) != m_pressedKeys.end())
								/* skip as the key is already pressed */
								break;
							else
								m_pressedKeys.insert({keyboardInput.makeCode, Win32::KeyStatus::Pressed});

							curKeyComb.push_back(keyboardInput);

							bool isKeyComb = false;
							for(auto& keyCombEventPair : window->m_keyCombs)
							{
								std::vector<Win32::KeyCode>& keyComb = keyCombEventPair.first;
								if(keyComb.size() != curKeyComb.size())
									continue;
								else
								{
									bool isMatched = true;
									for(std::size_t i = 0; i < curKeyComb.size(); i++)
									{
										if(com::IntToEnumClass<Win32::KeyCode>(static_cast<u8>(curKeyComb[i].virtualKey)) != keyComb[i])
										{
											isMatched = false;
											break;
										}
									}
									if(!isMatched)
										continue;
									keyCombEventPair.second.publish(curKeyComb);
									isKeyComb = true;
									break;
								}
							}
							if(isKeyComb)
								break;
						}
						else
						{
							auto result = m_pressedKeys.erase(keyboardInput.makeCode);
							DEBUG_ASSERT(result == 1);

							DEBUG_ASSERT(keyboardInput.keyStatus == Win32::KeyStatus::Released);
							Win32::KeyCode virtualKey = com::IntToEnumClass<Win32::KeyCode>(static_cast<u8>(curKeyComb.back().virtualKey));
							if(virtualKey == keyboardInput.virtualKey)
								curKeyComb.pop_back();
							else
								curKeyComb.clear();
						}
						window->getKeyboardEvent().publish(keyboardInput);
						break;
					}

					case RIM_TYPEHID:
					{
						com_debug_log_info("Unknown HID Raw Input");
						break;
					}
				}
				break;
			}

			case WM_PAINT:
			{
				PAINTSTRUCT paintStruct;
				
				// Begin Paint
				if(BeginPaint(hwnd, &paintStruct) == NULL)
					kvmio_Internal_ErrorExit("BeginPaint");

				Win32::WindowPaintInfo paintInfo = { paintStruct.hdc, paintStruct.rcPaint };
				if(window->m_paintCallback)
				{
					std::span<const std::byte> frameData = window->m_paintCallback();
					DEBUG_ASSERT(frameData.size() == window->m_drawSurface->getBufferSize());
					memcpy(window->m_drawSurface->getPixels(), reinterpret_cast<const char*>(frameData.data()), frameData.size());
					auto drawSurfaceSize = window->m_drawSurface->getSize();
					// Do Paint
					BitBlt(paintInfo.deviceContext, 0, 0, drawSurfaceSize.first, drawSurfaceSize.second, window->m_drawSurface->getHDC(), 0, 0, SRCCOPY);
				}

				// End Paint
				EndPaint(hwnd, &paintStruct);
				break;
			}

			case WM_CLOSE:
			{
				PostQuitMessage(0);
				return 0;
			}
		}
    	return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	static int GetWin32HookFromHookType(Win32Window::HookType hookType)
	{
		using HookType = Win32Window::HookType;
		switch(hookType)
		{
			case HookType::Keyboard: return WH_KEYBOARD;
			case HookType::KeyboardLowLevel: return WH_KEYBOARD_LL;
			case HookType::Mouse: return WH_MOUSE;
			case HookType::MouseLowLevel: return WH_MOUSE_LL;
			default:
			{
				com_debug_log_fetal_error("Unrecognized SKVMOIP::Window::HookType: %lu", static_cast<u32>(hookType));
				return -1;
			}
		}
	}

} // namespace kvmio
