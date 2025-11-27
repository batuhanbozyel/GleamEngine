#include "gpch.h"
#include "GPUAllocator.h"
#include "Renderer/GraphicsDevice.h"

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>

using namespace Gleam;

GPUAllocator::GPUAllocator(GraphicsDevice* device, const GPUAllocatorDescriptor& descriptor)
	: mDevice(device)
	, mName(descriptor.name)
{
	mBlockSizes[(uint32_t)MemoryType::CPU] = descriptor.cpuBlockSize;
	mBlockSizes[(uint32_t)MemoryType::GPU] = descriptor.gpuBlockSize;
}

GPUAllocator::~GPUAllocator()
{
	GLEAM_ASSERT(mCurrentAllocationInBytes == 0, "GPU allocator must be empty, some resources are leaking");
}

GPUAllocation GPUAllocator::Allocate(const MemoryRequirements& memory)
{
	mCurrentAllocationInBytes += memory.size;
	if (mCurrentAllocationInBytes > mPeakAllocationInBytes)
	{
		mPeakAllocationInBytes = mCurrentAllocationInBytes;
	}

	VmaVirtualAllocationCreateInfo createInfo = {};
	createInfo.size = memory.size;
	createInfo.alignment = memory.alignment;
	createInfo.flags = VmaVirtualAllocationCreateFlagBits::VMA_VIRTUAL_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT;

	VmaVirtualAllocation allocation = VK_NULL_HANDLE;
	uint64_t offset = 0;

	auto& blocks = mBlocks[(uint32_t)memory.type];
	for (auto block : blocks)
	{
		VkResult res = vmaVirtualAllocate(static_cast<VmaVirtualBlock>(block->handle), &createInfo, &allocation, &offset);
		if (res == VK_SUCCESS)
		{
			block->allocations.push_back(allocation);
			return GPUAllocation
			{
				.block = block,
				.handle = allocation,
				.offset = offset,
				.size = memory.size
			};
		}
	}

	HeapDescriptor heapDesc;
	heapDesc.memoryType = memory.type;
	heapDesc.size = mBlockSizes[(uint32_t)memory.type];
	heapDesc.name = mName + "::" + Utils::MemoryTypeToString(memory.type);
	auto block = AllocateHeap(heapDesc);
	GLEAM_AFFIRM(vmaVirtualAllocate(static_cast<VmaVirtualBlock>(block->handle), &createInfo, &allocation, &offset) == VK_SUCCESS, "GPU allocator virtual allocation failed");
	block->allocations.push_back(allocation);
	return GPUAllocation
	{
		.resource = nullptr, // not registered yet
		.block = block,
		.handle = allocation,
		.offset = offset,
		.size = memory.size,
		.alignment = memory.alignment
	};
}

void GPUAllocator::Free(const GPUAllocation& allocation)
{
	GLEAM_ASSERT(allocation.IsValid(), "Allocation is not valid");

	// Free allocation
	{
		auto& allocations = allocation.block->allocations;
		auto it = eastl::find(allocations.begin(), allocations.end(), allocation.handle);
		GLEAM_ASSERT(it != allocations.end(), "Allocation already freed");
		allocations.erase(it);

		vmaVirtualFree(static_cast<VmaVirtualBlock>(allocation.block->handle), static_cast<VmaVirtualAllocation>(allocation.handle));
		if (allocations.empty())
		{
			FreeHeap(allocation.block);
		}
		mCurrentAllocationInBytes -= allocation.size;
	}

	// Remove resource from allocations
	{
		auto it = mAllocations.find(allocation.resource);
		GLEAM_ASSERT(it != mAllocations.end(), "Allocation is not registered");
		mAllocations.erase(it);
	}
}

void GPUAllocator::AddAllocation(NativeGraphicsHandle resource, const GPUAllocation& allocation)
{
	GLEAM_ASSERT(mAllocations.find(resource) == mAllocations.end(), "Allocation already registered");
	auto it = mAllocations.emplace_hint(mAllocations.end(), resource, allocation);
	it->second.resource = resource;
}

const GPUAllocation& GPUAllocator::GetAllocation(NativeGraphicsHandle resource) const
{
	auto it = mAllocations.find(resource);
	GLEAM_ASSERT(it != mAllocations.end(), "Allocation is not registered");
	return it->second;
}

GPUAllocationBlock* GPUAllocator::AllocateHeap(const HeapDescriptor& descriptor)
{
	VmaVirtualBlockCreateInfo blockCreateInfo = {};
	blockCreateInfo.flags = 0;
	blockCreateInfo.pAllocationCallbacks = nullptr;
	blockCreateInfo.size = descriptor.size;

	VmaVirtualBlock virtualBlock = VK_NULL_HANDLE;
	GLEAM_AFFIRM(vmaCreateVirtualBlock(&blockCreateInfo, &virtualBlock) == VK_SUCCESS, "GPU allocator virtual block allocation failed");

	auto block = new GPUAllocationBlock
	{
		.heap = mDevice->CreateHeap(descriptor),
		.handle = virtualBlock,
		.memoryType = descriptor.memoryType
	};
	mBlocks[(uint32_t)descriptor.memoryType].push_back(block);
	return block;
}

void GPUAllocator::FreeHeap(GPUAllocationBlock* block)
{
	GLEAM_ASSERT(block && block->IsValid(), "Allocation block is not valid");
	auto& blocks = mBlocks[(uint32_t)block->memoryType];
	blocks.erase(eastl::find(blocks.begin(), blocks.end(), block));

	vmaDestroyVirtualBlock(static_cast<VmaVirtualBlock>(block->handle));
	mDevice->Dispose(block->heap);
	block->handle = VK_NULL_HANDLE;
	delete block;
}