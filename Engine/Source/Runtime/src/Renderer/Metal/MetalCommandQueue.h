#pragma once
#ifdef USE_METAL_RENDERER
#include "Container/Queue.h"
#include "Container/Hash.h"
#include "Container/String.h"

#import <Metal/Metal.h>

namespace Gleam {

class MetalDevice;
class MetalCommandQueue;

struct MetalCommandPool
{
	id<MTLDevice> mDevice = nil;
	MetalCommandQueue* mQueue = nullptr;
	id<MTL4CommandAllocator> mAllocator = nil;
	Deque<void*> mUsedCommandBuffers = {};
	Deque<void*> mFreeCommandBuffers = {};
	uint64_t mFenceValue = 0;

	id<MTL4CommandBuffer> AllocateCommandBuffer(const TStringView debugName);

	void Reset();
	void Release();
};

class MetalCommandQueue final
{
	friend class MetalDevice;
	friend struct MetalCommandPool;
public:

	void Release();

	MetalCommandPool* AcquirePool();

	void Commit(id<MTL4CommandBuffer>* cmdBuffers, uint32_t count);

	void Signal(id<MTLSharedEvent> event, uint64_t value) const;

	id<MTL4CommandQueue> GetHandle() const;

private:

	uint64_t mFenceValue = 0;
	id<MTLDevice> mDevice = nil;
	id<MTL4CommandQueue> mHandle = nil;
	id<MTLSharedEvent> mFence = nil;
	Deque<MetalCommandPool> mPools = {};
	HashMap<void*, MetalCommandPool*> mCommandBufferToPoolMap = {};
};

} // namespace Gleam
#endif // USE_METAL_RENDERER
