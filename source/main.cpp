#include <iostream>

#include <common/platform.h>

#ifdef PLATFORM_WINDOWS
#	include <kvmio/Win32Window.hpp>
#	define WINDOW_IMPLEMENTATION_OBJ kvmio::Win32Window
#endif // PLATFORM_WINDOWS

#include <memory> // for std::shared_ptr<>

int main(int argc, const char** argv)
{
	std::shared_ptr<kvmio::Window> window = std::make_shared<WINDOW_IMPLEMENTATION_OBJ>(800, 800, "My Window");

	window->show();

	window->runGameLoop();

	return 0;
}
