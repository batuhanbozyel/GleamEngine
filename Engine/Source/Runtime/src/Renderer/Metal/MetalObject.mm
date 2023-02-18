#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/GraphicsObject.h"
#include "MetalDevice.h"

using namespace Gleam;

MutableGraphicsObject::MutableGraphicsObject()
	: mHandles(MetalDevice::GetSwapchain().GetMaxFramesInFlight())
{

}

NativeGraphicsHandle MutableGraphicsObject::GetHandle() const
{
	return mHandles[MetalDevice::GetSwapchain().GetFrameIndex()];
}

bool MutableGraphicsObject::IsValid() const
{
	return mHandles[MetalDevice::GetSwapchain().GetFrameIndex()];
}
#endif