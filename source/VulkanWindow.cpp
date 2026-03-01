#include <kvmio/VulkanWindow.hpp>

namespace kvmio
{
	VulkanWindow::VulkanWindow(u32 width, u32 height, std::string_view title) : NativeWindow(width, height, title)
	{
		m_vkPresentEngine = std::make_unique<VulkanPresentEngine>([this](VkInstance& vkInstance) -> VkSurfaceKHR
		{
			return pvkCreateSurface(vkInstance, GetModuleHandle(NULL), this->getNativeHandle());
		});
	}
}