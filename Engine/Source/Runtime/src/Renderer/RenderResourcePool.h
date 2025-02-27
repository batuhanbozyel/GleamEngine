#pragma once
#include "Heap.h"
#include "Texture.h"

namespace Gleam {

class RenderSurface;
class GraphicsDevice;
class ResourceReleaseQueue;

class RenderResourcePool
{
public:

	RenderResourcePool(GraphicsDevice* device, RenderSurface* surface, ResourceReleaseQueue* releaseQueue);

	~RenderResourcePool();

	void Clear();

	Heap Allocate(const HeapDescriptor& descriptor, bool destroyOnResize = true);

	Texture Allocate(const TextureDescriptor& descriptor, bool destroyOnResize = true);

	void Release(const Heap& heap);

	void Release(const Texture& texture);

private:

	Deque<Heap> mFreeHeaps;

	Deque<Texture> mFreeTextures;

	GraphicsDevice* mDevice;

	RenderSurface* mSurface;

	ResourceReleaseQueue* mReleaseQueue;

};

} // namespace Gleam
