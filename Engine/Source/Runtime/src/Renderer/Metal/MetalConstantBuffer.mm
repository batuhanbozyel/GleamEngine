#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/ConstantBuffer.h"
#include "MetalDevice.h"
#include "MetalUtils.h"

using namespace Gleam;

ConstantBuffer::ConstantBuffer(GraphicsDevice* device, size_t size)
	: mAlignment(4)
	, mCapacity(Math::AlignUp(size, (size_t)4u))
    , mDevice(device)
{
	id<MTLBuffer> mtlBuffer = [device->GetHandle() newBufferWithLength:size options:MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined];
    [mtlBuffer setLabel:TO_NSSTRING("ConstantBuffer")];
    mContents = [mtlBuffer contents];
	mHandle = mtlBuffer;
    
    [static_cast<MetalDevice*>(mDevice)->GetResidencySet() addAllocation:mHandle];
}

ConstantBuffer::~ConstantBuffer()
{
    [static_cast<MetalDevice*>(mDevice)->GetResidencySet() removeAllocation:mHandle];
	mHandle = nil;
}
#endif
