#include "gpch.h"
#include "Renderer/GraphicsObject.h"
#include "MetalDevice.h"

using namespace Gleam;

NativeGraphicsHandle MutableGraphicsObject::GetHandle() const
{
	return mHandles[MetalDevice::GetSwapchain().GetFrameIndex()];
}

bool MutableGraphicsObject::IsValid() const
{
	return mHandles[MetalDevice::GetSwapchain().GetFrameIndex()];
}