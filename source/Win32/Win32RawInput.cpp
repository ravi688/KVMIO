#include <kvmio/Win32/Win32RawInput.hpp>
#include <kvmio/ErrorHandling.hpp>
#include <cstdarg>
#include <iostream>

#include <common/defines.hpp>.
#ifdef ASSERT
#undef ASSERT
#endif
#ifdef _ASSERT
#	undef _ASSERT
#endif
#include <spdlog/spdlog.h>
#include <libassert/assert.hpp>

namespace kvmio::Win32
{

	KVMIO_API KeyboardInput DecodeRawKeyboardInput(RAWKEYBOARD* rawKeyboard)
	{
		KeyboardInput input = { };
		input.makeCode = rawKeyboard->MakeCode;
		input.virtualKey = rawKeyboard->VKey;
		input.keyStatus = ((rawKeyboard->Flags & RI_KEY_BREAK) == RI_KEY_BREAK) ? KeyStatus::Released : KeyStatus::Pressed;
		ASSERT(((rawKeyboard->Flags & RI_KEY_MAKE) == RI_KEY_MAKE) || ((rawKeyboard->Flags & RI_KEY_BREAK) == RI_KEY_BREAK));
		input.isExtended0 = (rawKeyboard->Flags & RI_KEY_E0) == RI_KEY_E0;
		input.isExtended1 = (rawKeyboard->Flags & RI_KEY_E1) == RI_KEY_E1;
		input.isAltorF10 = rawKeyboard->Message == WM_SYSKEYDOWN;

		if(input.isExtended0)
			input.makeCode |= (static_cast<u32>(0xE0) << 8);
		else if(input.isExtended1)
			input.makeCode |= (static_cast<u32>(0xE0) << 16);

		DEBUG_ASSERT(rawKeyboard->MakeCode != KEYBOARD_OVERRUN_MAKE_CODE);

		return input;
	}

	KVMIO_API void DumpKeyboardInput(const KeyboardInput* keyboardInput)
	{
		spdlog::info("KeyboardInput {{ makeCode: {}, vkey: {}, status: {}, e0: {}, e1: {}, AltOrF10: {} }}",
						keyboardInput->makeCode, keyboardInput->virtualKey, static_cast<u8>(keyboardInput->keyStatus), keyboardInput->isExtended0, keyboardInput->isExtended1, keyboardInput->isAltorF10);
	}

	KVMIO_API MouseInput DecodeRawMouseInput(RAWMOUSE* rawMouse)
	{
		MouseInput input = { };
		auto buttonFlags = rawMouse->usButtonFlags;

		input.isWheelX = HAS_FLAG(buttonFlags, RI_MOUSE_WHEEL);
		/* Warning: Horizontal wheel is not supported on Windows XP and 2000 */
		input.isWheelY = HAS_FLAG(buttonFlags, RI_MOUSE_HWHEEL);
		input.isMoveRelative = HAS_FLAG(rawMouse->usFlags, MOUSE_MOVE_RELATIVE);
		input.isMoveAbsolute = HAS_FLAG(rawMouse->usFlags, MOUSE_MOVE_ABSOLUTE);
		input.isVirtualDesktop = HAS_FLAG(rawMouse->usFlags, MOUSE_VIRTUAL_DESKTOP);
		
		bool isMiddleDown = HAS_FLAG(buttonFlags, RI_MOUSE_MIDDLE_BUTTON_DOWN);
		bool isLeftDown = HAS_FLAG(buttonFlags, RI_MOUSE_LEFT_BUTTON_DOWN);
		bool isRightDown = HAS_FLAG(buttonFlags, RI_MOUSE_RIGHT_BUTTON_DOWN);
		bool isButton4Down = HAS_FLAG(buttonFlags, RI_MOUSE_BUTTON_4_DOWN);
		bool isButton5Down = HAS_FLAG(buttonFlags, RI_MOUSE_BUTTON_5_DOWN);
		bool isMiddleUp = HAS_FLAG(buttonFlags, RI_MOUSE_MIDDLE_BUTTON_UP);
		bool isLeftUp = HAS_FLAG(buttonFlags, RI_MOUSE_LEFT_BUTTON_UP);
		bool isRightUp = HAS_FLAG(buttonFlags, RI_MOUSE_RIGHT_BUTTON_UP);
		bool isButton4Up = HAS_FLAG(buttonFlags, RI_MOUSE_BUTTON_4_UP);
		bool isButton5Up = HAS_FLAG(buttonFlags, RI_MOUSE_BUTTON_5_UP);

		bool isAnyButtonDown = isMiddleDown || isLeftDown || isRightDown || isButton4Down || isButton5Down;
		input.isAnyButton = isAnyButtonDown || isMiddleUp || isLeftUp || isRightUp || isButton4Up || isButton5Up;

		if(input.isAnyButton)
		{
			input.leftButton.isTransition = isLeftDown | isLeftUp;
			input.leftButton.status = isLeftDown ? Win32::KeyStatus::Pressed : Win32::KeyStatus::Released;
			
			input.rightButton.isTransition = isRightDown | isRightUp;
			input.rightButton.status = isRightDown ? Win32::KeyStatus::Pressed : Win32::KeyStatus::Released;
			
			input.middleButton.isTransition = isMiddleDown | isMiddleUp;
			input.middleButton.status = isMiddleDown ? Win32::KeyStatus::Pressed : Win32::KeyStatus::Released;
			
			input.browseForwardButton.isTransition = isButton4Down | isButton4Up;
			input.browseForwardButton.status = isButton4Down ? Win32::KeyStatus::Pressed : Win32::KeyStatus::Released;
		
			input.browseBackwardButton.isTransition = isButton5Down | isButton5Up;
			input.browseBackwardButton.status = isButton5Down ? Win32::KeyStatus::Pressed : Win32::KeyStatus::Released;
		}

		input.movement.x = static_cast<s16>(rawMouse->lLastX);
		input.movement.y = static_cast<s16>(rawMouse->lLastY);
		if(input.isWheelX)
			input.wheel.x = static_cast<s16>(rawMouse->usButtonData) / WHEEL_DELTA;
		else if(input.isWheelY)
			input.wheel.y = static_cast<s16>(rawMouse->usButtonData) / WHEEL_DELTA;
		return input;
	}

	KVMIO_API void DumpMouseInput(const MouseInput* mouseInput)
	{
		spdlog::info("MouseInput {{ ({}, {}), Rel: {}, Abs: {}, wheelX: ({}, {}), wheelY: ({}, {}), MB: {}, LB: {}, RB: {}, BFB: {}, BBB: {} }}",
						mouseInput->movement.x, 
						mouseInput->movement.y,
						mouseInput->isMoveRelative, 
						mouseInput->isMoveAbsolute, 
						mouseInput->isWheelX, 
						mouseInput->wheel.x, 
						mouseInput->isWheelY, 
						mouseInput->wheel.y, 
						com::EnumClassToInt(mouseInput->middleButton.status), 
						com::EnumClassToInt(mouseInput->leftButton.status), 
						com::EnumClassToInt(mouseInput->rightButton.status), 
						com::EnumClassToInt(mouseInput->browseForwardButton.status), 
						com::EnumClassToInt(mouseInput->browseBackwardButton.status));
	}

	static USHORT getHIDUsagePageID(RawInputDeviceType deviceType)
	{
		switch(deviceType)
		{
			case RawInputDeviceType::Mouse:
			case RawInputDeviceType::Keyboard: return 0x01; /* Generic Desktop Controls, see: https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/hid-usages */
			default:
				spdlog::critical("Unable to get HID Usage Page ID, Invalid Win32::RawInputDeviceType: {}", com::to_underlying(deviceType));
				exit(EXIT_FAILURE);
		}
	}

	static USHORT getHIDUsageID(RawInputDeviceType deviceType)
	{
		switch(deviceType)
		{
			case RawInputDeviceType::Mouse: return 0x02;
			case RawInputDeviceType::Keyboard: return 0x06;
			default:
				spdlog::critical("Unable to get HID Usage ID, Invalid Win32::RawInputDeviceType: {}", com::to_underlying(deviceType));
				exit(EXIT_FAILURE);
		}
	}

	static DWORD getHIDFlags(RawInputDeviceType deviceType)
	{
		switch(deviceType)
		{
			case RawInputDeviceType::Mouse: return 0;
			case RawInputDeviceType::Keyboard: return 0;
			default:
				spdlog::critical("Unable to get HID Flags, Invalid Win32::RawInputDeviceType: {}", com::to_underlying(deviceType));
				exit(EXIT_FAILURE);
		}
	}

	static const char* getDeviceTypeStr(DWORD dwType)
	{
		switch(dwType)
		{
			case RIM_TYPEHID: return "Not a keyboard or Mouse HID";
			case RIM_TYPEMOUSE: return "Mouse HID";
			case RIM_TYPEKEYBOARD: return "Keyboard HID";
			default: return "Unkown";
		}
	}

	KVMIO_API void DisplayRawInputDeviceList()
	{
		UINT numDevices;
		if(GetRawInputDeviceList(NULL, &numDevices, sizeof(RAWINPUTDEVICELIST)) == static_cast<UINT>(-1))
			kvmio_Internal_ErrorExit("GetRawInputDeviceList");

		RAWINPUTDEVICELIST* rawInputDeviceListBuffer = new RAWINPUTDEVICELIST[numDevices];

		if(GetRawInputDeviceList(rawInputDeviceListBuffer, &numDevices, sizeof(RAWINPUTDEVICELIST)) == static_cast<UINT>(-1))
			kvmio_Internal_ErrorExit("GetRawInputDeviceList");

		spdlog::info("Raw Input Device Count: {}", numDevices);
		for(u32 i = 0; i < numDevices; i++)
		{
			auto& deviceList = rawInputDeviceListBuffer[i];
			spdlog::info("\tDevice Handle: {}\n"
						   "\t\tType: {}", deviceList.hDevice, getDeviceTypeStr(deviceList.dwType));
		}
	}

	KVMIO_API void RegisterRawInputDevices(std::vector<RawInputDeviceType> deviceTypes)
	{
		std::vector<RAWINPUTDEVICE> rawDevices;
		rawDevices.reserve(deviceTypes.size());
		for(RawInputDeviceType deviceType: deviceTypes)
		{
			RAWINPUTDEVICE rawDevice;
			rawDevice.usUsagePage = getHIDUsagePageID(deviceType);          // HID_USAGE_PAGE_GENERIC
			rawDevice.usUsage = getHIDUsageID(deviceType);              // HID_USAGE_GENERIC_MOUSE
			rawDevice.dwFlags = getHIDFlags(deviceType);    // adds mouse and also ignores legacy mouse messages
			rawDevice.hwndTarget = 0; /* follow the keyboard focus */
			rawDevices.push_back(rawDevice);
		}

		if (RegisterRawInputDevices(rawDevices.data(), rawDevices.size(), sizeof(RAWINPUTDEVICE)) == FALSE)
			kvmio_Internal_ErrorExit("RegisterRawInputDevices");
	}
}
