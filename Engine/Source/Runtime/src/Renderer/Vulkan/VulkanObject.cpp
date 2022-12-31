#include "gpch.h"
#include "Renderer/GraphicsObject.h"
#include "VulkanDevice.h"

using namespace Gleam;

NativeGraphicsHandle MutableGraphicsObject::GetHandle() const
{
	return mHandles[VulkanDevice::GetSwapchain().GetFrameIndex()];
}

bool MutableGraphicsObject::IsValid() const
{
	return mHandles[VulkanDevice::GetSwapchain().GetFrameIndex()];
}