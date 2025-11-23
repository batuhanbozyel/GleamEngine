#pragma once
#include "Renderer/Heap.h"

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
	GPUAllocationBlock* block = nullptr;
	void* handle = nullptr;
	uint64_t offset = 0;
	uint64_t size = 0;

	bool IsValid() const
	{
		return block != nullptr && handle != nullptr;
	}
};

struct GPUAllocatorDescriptor
{
	TString name;
	uint64_t cpuBlockSize = 0;
	uint64_t sharedBlockSize = 0;
	uint64_t gpuBlockSize = 0;
	uint64_t transientBlockSize = 0;
};

class GPUAllocator
{
public:

	GPUAllocator(GraphicsDevice* device, const GPUAllocatorDescriptor& descriptor);
	~GPUAllocator();

	GPUAllocation Allocate(const MemoryRequirements& memory);
	void Free(const GPUAllocation& allocation);

private:

	GPUAllocationBlock* AllocateHeap(const HeapDescriptor& descriptor);
	void FreeHeap(GPUAllocationBlock* block);

	TString mName;
	GraphicsDevice* mDevice;
	uint64_t mCurrentAllocationInBytes = 0;
	uint64_t mPeakAllocationInBytes = 0;
	uint64_t mBlockSizes[(uint32_t)MemoryType::Transient + 1];
	TArray<GPUAllocationBlock*> mBlocks[(uint32_t)MemoryType::Transient + 1];
};

} // namespace Gleam
