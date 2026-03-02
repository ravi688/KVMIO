# KVMIO
Cross Platform keyboard/mouse input and video output library.

## Building
```
build_master meson setup build --buildtype=debug
build_master meson compile -C build
```
## Running examples
```
./build/kvmio.exe
```

## Example:
```cpp
#include <iostream>

#include <kvmio/NativeWindow.hpp>
#include <common/Utility.hpp>
#include <spdlog/spdlog.h>

#include <memory> // for std::unique_ptr<>
#include <thread>
#include <atomic>
#include <chrono>


std::atomic<bool> gIsPresent = true;

template<typename To, typename From>
std::span<To> span_cast(std::span<From> s)
{
    static_assert(sizeof(To) == sizeof(From));
    return { reinterpret_cast<To*>(s.data()), s.size() };
}

void HandlePresent(kvmio::Window& window)
{
	auto fileData1 = com::LoadBinaryFile("data/picture1.nv12");
	if(!fileData1)
	{
		spdlog::critical("Failed to load file data/picture1.nv12");
		exit(-1);
	}
	auto fileData2 = com::LoadBinaryFile("data/picture2.nv12");
	if(!fileData2)
	{
		spdlog::critical("Failed to load file data/picture2.nv12");
		exit(-1);
	}
	while(gIsPresent)
	{
		std::this_thread::sleep_for(std::chrono::duration<float, std::ratio<1, 1>>(1));
		window.present(span_cast<const u8>(fileData1.span()));
		std::this_thread::sleep_for(std::chrono::duration<float, std::ratio<1, 1>>(1));
		window.present(span_cast<const u8>(fileData2.span()));
	}
	fileData1.destroy();
	fileData2.destroy();
}

int main(int argc, const char** argv)
{
	std::unique_ptr<kvmio::Window> window = std::make_unique<kvmio::NativeWindow>(800, 800, "My Window");

	window->show();

	std::thread presentThread(HandlePresent, std::ref(*window));

	window->runGameLoop();

	if(presentThread.joinable())
	{
		gIsPresent = false;
		spdlog::info("Waiting for the present thread to be finished");
		presentThread.join();
	}
	
	return 0;
}

```
