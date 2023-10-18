//
//  VulkanContext.cpp
//  GleamEngine
//
//  Created by Batuhan Bozyel on 20.03.2023.
//

#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/GraphicsDevice.h"
#include "VulkanDevice.h"

using namespace Gleam;

void RendererContext::WaitDeviceIdle() const
{
	VK_CHECK(vkDeviceWaitIdle(VulkanDevice::GetHandle()));
}

void RendererContext::ConfigureBackend()
{
    VulkanDevice::Init();
}

void RendererContext::DestroyBackend()
{
    VulkanDevice::Destroy();
}

void RendererContext::Configure(const RendererConfig& config)
{
    VulkanDevice::GetSwapchain().Configure(config);
}

void RendererContext::AddPooledObject(std::any object, std::function<void(std::any)> deallocator)
{
    VulkanDevice::GetSwapchain().AddPooledObject(object, deallocator);
}

uint32_t RendererContext::GetFrameIndex() const
{
    return VulkanDevice::GetSwapchain().GetFrameIndex();
}

uint32_t RendererContext::GetFramesInFlight() const
{
    return VulkanDevice::GetSwapchain().GetFramesInFlight();
}

const Gleam::Size& RendererContext::GetDrawableSize() const
{
    return VulkanDevice::GetSwapchain().GetSize();
}

#endif
