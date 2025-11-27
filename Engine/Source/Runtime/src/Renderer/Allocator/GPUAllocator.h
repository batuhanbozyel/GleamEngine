#pragma once
#include "Renderer/Heap.h"
#include "Container/Hash.h"

namespace Gleam {

class GraphicsDevice;

struct GPUAllocationBlock
{
	Heap heap;
	void* handle = nullptr;
	MemoryType memoryType = MemoryType::GPU;
	TArray<void*> allocations;

	bool IsValid() const
	{
		return heap.IsValid() && handle != nullptr;
	}
};

struct GPUAllocation
{
	NativeGraphicsHandle resource = nullptr;
	GPUAllocationBlock* block = nullptr;
	void* handle = nullptr;
	uint64_t offset = 0;
	uint64_t size = 0;
	uint64_t alignment = 0;

	bool IsValid() const
	{
		return block != nullptr && handle != nullptr;
	}
};

struct GPUAllocatorDescriptor
{
	TString name;
	uint64_t cpuBlockSize = 128 * 1024 * 1024;        // 128 MB
	uint64_t gpuBlockSize = 512 * 1024 * 1024;        // 512 MB
};

class GPUAllocator
{
public:

	GPUAllocator(GraphicsDevice* device, const GPUAllocatorDescriptor& descriptor);
	~GPUAllocator();

	GPUAllocation Allocate(const MemoryRequirements& memory);
	void Free(const GPUAllocation& allocation);

	void AddAllocation(NativeGraphicsHandle resource, const GPUAllocation& allocation);
	const GPUAllocation& GetAllocation(NativeGraphicsHandle resource) const;

private:

	GPUAllocationBlock* AllocateHeap(const HeapDescriptor& descriptor);
	void FreeHeap(GPUAllocationBlock* block);

	TString mName;
	GraphicsDevice* mDevice;
	uint64_t mCurrentAllocationInBytes = 0;
	uint64_t mPeakAllocationInBytes = 0;
	uint64_t mBlockSizes[(uint32_t)MemoryType::GPU + 1];
	TArray<GPUAllocationBlock*> mBlocks[(uint32_t)MemoryType::GPU + 1];
	HashMap<NativeGraphicsHandle, GPUAllocation> mAllocations;
};

} // namespace Gleam
