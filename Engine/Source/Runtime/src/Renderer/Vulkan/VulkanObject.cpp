#include "gpch.h"
#include "Renderer/GraphicsObject.h"
#include "VulkanDevice.h"

using namespace Gleam;

MutableGraphicsObject::MutableGraphicsObject()
	:	mHandles(VulkanDevice::GetSwapchain().GetMaxFramesInFlight()),
		mMemories(VulkanDevice::GetSwapchain().GetMaxFramesInFlight())
{

}

NativeGraphicsHandle MutableGraphicsObject::GetHandle() const
{
	return mHandles[VulkanDevice::GetSwapchain().GetFrameIndex()];
}

bool MutableGraphicsObject::IsValid() const
{
	return mHandles[VulkanDevice::GetSwapchain().GetFrameIndex()];
}