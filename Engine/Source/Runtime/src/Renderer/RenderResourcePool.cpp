#include "gpch.h"
#include "RenderResourcePool.h"

#include "Swapchain.h"
#include "GraphicsDevice.h"
#include "ResourceReleaseQueue.h"

using namespace Gleam;

RenderResourcePool::RenderResourcePool(GraphicsDevice* device,
									   RenderSurface* surface,
									   ResourceReleaseQueue* releaseQueue)
	: mDevice(device)
	, mSurface(surface)
	, mReleaseQueue(releaseQueue)
{

}

RenderResourcePool::~RenderResourcePool()
{
	Clear();
}

void RenderResourcePool::Clear()
{
	for (auto& texture : mFreeTextures)
	{
		mDevice->Dispose(texture);
	}
	mFreeTextures.clear();

	for (auto& heap : mFreeHeaps)
	{
		mDevice->Dispose(heap);
	}
	mFreeHeaps.clear();
}

Heap RenderResourcePool::Allocate(const HeapDescriptor& descriptor, bool destroyOnResize)
{
	auto it = std::find_if(mFreeHeaps.begin(), mFreeHeaps.end(), [&](const Heap& heap) -> bool
	{
		return heap.GetDescriptor().memoryType == descriptor.memoryType
			&& heap.GetDescriptor().size >= descriptor.size;
	});

	if (it != mFreeHeaps.end())
	{
		auto heap = *it;
		GLEAM_ASSERT(heap.IsValid());
		mFreeHeaps.erase(it);
		return heap;
	}
	return mDevice->CreateHeap(descriptor);
}

Texture RenderResourcePool::Allocate(const TextureDescriptor& descriptor, bool destroyOnResize)
{
	auto it = std::find_if(mFreeTextures.begin(), mFreeTextures.end(), [&](const Texture& texture) -> bool
	{
		return texture.GetDescriptor() == descriptor;
	});

	if (it != mFreeTextures.end())
	{
		auto texture = *it;
		GLEAM_ASSERT(texture.IsValid());
		mFreeTextures.erase(it);
		return texture;
	}
	return mDevice->CreateTexture(descriptor);
}

void RenderResourcePool::Release(const Heap& heap)
{
	GLEAM_ASSERT(heap.IsValid());

	auto swapchain = static_cast<Swapchain*>(mSurface);
	mReleaseQueue->AddResource([this, heap = heap]() mutable
	{
		heap.Reset();
		mFreeHeaps.push_back(heap);
	}, swapchain->GetFrameIndex());
}

void RenderResourcePool::Release(const Texture& texture)
{
	GLEAM_ASSERT(texture.IsValid());

	auto swapchain = static_cast<Swapchain*>(mSurface);
	mReleaseQueue->AddResource([this, texture = texture]() mutable
	{
		mFreeTextures.push_back(texture);
	}, swapchain->GetFrameIndex());
}
