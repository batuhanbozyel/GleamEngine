#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/ConstantBuffer.h"
#include "MetalDevice.h"
#include "MetalUtils.h"

using namespace Gleam;

ConstantBuffer::ConstantBuffer(GraphicsDevice* device, size_t size)
	: mAlignment(4)
	, mCapacity(Utils::AlignUp(size, 4))
{
	id<MTLBuffer> mtlBuffer = [device->GetHandle() newBufferWithLength:size options:MTLResourceStorageModeShared];
    [mtlBuffer setLabel:TO_NSSTRING("ConstantBuffer")];
    mContents = [mtlBuffer contents];
	mHandle = mtlBuffer;
}

ConstantBuffer::~ConstantBuffer()
{
	mHandle = nil;
}
#endif
