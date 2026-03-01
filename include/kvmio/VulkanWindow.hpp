#pragma once

#include <kvmio/NativeWindow.hpp>

#include <kvmio/VulkanPresentEngine.hpp>

#include <memory> // for std::unique_ptr<>

namespace kvmio
{
	class KVMIO_API VulkanWindow : public NativeWindow
	{
	private:
		std::unique_ptr<VulkanPresentEngine> m_vkPresentEngine;
	public:
		VulkanWindow(u32 width, u32 height, std::string_view title);
		~VulkanWindow();

		// Overrides
		virtual void runGameLoop() override;
		virtual void runGameLoop(u32 frameRate, const Predicate& isLoop = [] { return true; }) override;
	};

}
