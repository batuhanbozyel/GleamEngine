#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "MetalCommandQueue.h"

using namespace Gleam;

id<MTL4CommandBuffer> MetalCommandPool::AllocateCommandBuffer(const TStringView debugName)
{
	id<MTL4CommandBuffer> commandBuffer = nil;
	if (mFreeCommandBuffers.empty())
	{
		commandBuffer = [mDevice newCommandBuffer];
	}
	else
	{
		commandBuffer = (id<MTL4CommandBuffer>)CFBridgingRelease(mFreeCommandBuffers.front());
		mFreeCommandBuffers.pop_front();
	}
	mQueue->mCommandBufferToPoolMap[(__bridge void*)commandBuffer] = this;
	mUsedCommandBuffers.push_back((__bridge_retained void*)commandBuffer);
	[commandBuffer beginCommandBufferWithAllocator:mAllocator];
	commandBuffer.label = [NSString stringWithUTF8String:debugName.data()];
	return commandBuffer;
}

void MetalCommandPool::Reset()
{
	[mAllocator reset];
	mFreeCommandBuffers.insert(mFreeCommandBuffers.end(), mUsedCommandBuffers.begin(), mUsedCommandBuffers.end());
	mUsedCommandBuffers.clear();
}

void MetalCommandPool::Release()
{
	for (auto cmdBuffer : mUsedCommandBuffers)
	{
		CFRelease(cmdBuffer);
	}
	mUsedCommandBuffers.clear();

	for (auto cmdBuffer : mFreeCommandBuffers)
	{
		CFRelease(cmdBuffer);
	}
	mFreeCommandBuffers.clear();

	mAllocator = nil;
}

void MetalCommandQueue::Release()
{
	mCommandBufferToPoolMap.clear();
	for (auto& pool : mPools)
	{
		pool.Release();
	}
	mPools.clear();

	mFence = nil;
	mHandle = nil;
	mDevice = nil;
}

MetalCommandPool* MetalCommandQueue::AcquirePool()
{
	uint64_t completed = mFence.signaledValue;
	for (auto& pool : mPools)
	{
		if (pool.mFenceValue <= completed)
		{
			pool.Reset();
			return &pool;
		}
	}

	auto& pool = mPools.emplace_back();
	pool.mDevice = mDevice;
	pool.mQueue = this;

	MTL4CommandAllocatorDescriptor* descriptor = [MTL4CommandAllocatorDescriptor new];
	__autoreleasing NSError* error = nil;
	pool.mAllocator = [mDevice newCommandAllocatorWithDescriptor:descriptor error:&error];
	GLEAM_ASSERT(pool.mAllocator, "Metal: Command allocator creation failed.");
	return &pool;
}

void MetalCommandQueue::Commit(id<MTL4CommandBuffer>* cmdBuffers, uint32_t count)
{
	[mHandle commit:cmdBuffers count:count];

	uint64_t fenceValue = ++mFenceValue;
	[mHandle signalEvent:mFence value:fenceValue];

	for (uint32_t i = 0; i < count; ++i)
	{
		mCommandBufferToPoolMap[(__bridge void*)cmdBuffers[i]]->mFenceValue = fenceValue;
	}
}

void MetalCommandQueue::Signal(id<MTLSharedEvent> event, uint64_t value) const
{
	[mHandle signalEvent:event value:value];
}

id<MTL4CommandQueue> MetalCommandQueue::GetHandle() const
{
	return mHandle;
}

#endif // USE_METAL_RENDERER
