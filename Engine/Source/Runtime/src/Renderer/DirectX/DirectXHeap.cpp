#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/Heap.h"
#include "Renderer/Buffer.h"

#include "DirectXTransitionManager.h"
#include "DirectXDevice.h"
#include "DirectXUtils.h"

using namespace Gleam;

Buffer Heap::CreateBuffer(const BufferDescriptor& descriptor) const
{
	auto alignedStackPtr = Utils::AlignUp(mStackPtr, mAlignment);
	auto newStackPtr = alignedStackPtr + descriptor.size;

	if (Utils::AlignUp(mDescriptor.size, mAlignment) < newStackPtr)
	{
		GLEAM_ASSERT(false, "DirectX: Heap is full!");
		return Buffer(nullptr, descriptor, nullptr);
	}
	mStackPtr = newStackPtr;

	auto initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
	auto flags = D3D12_RESOURCE_FLAG_NONE;

	if (descriptor.usage == BufferUsage::StorageBuffer)
	{
		initialState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	if (mDescriptor.memoryType == MemoryType::GPU)
	{
		initialState |= D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_COPY_SOURCE;
	}

	D3D12_RESOURCE_DESC resourceDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment = 0,
		.Width = descriptor.size,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc = {.Count = 1, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = flags
	};

	ID3D12Resource* buffer = nullptr;
	static_cast<ID3D12Device10*>(mDevice->GetHandle())->CreatePlacedResource(
		static_cast<ID3D12Heap*>(mHandle),
		alignedStackPtr,
		&resourceDesc,
		initialState,
		nullptr,
		IID_PPV_ARGS(&buffer)
	);
	DirectXTransitionManager::SetLayout(buffer, initialState);

    void* contents = nullptr;
    if (mDescriptor.memoryType != MemoryType::GPU)
	{
		DX_CHECK(buffer->Map(0, nullptr, &contents));
	}
    return Buffer(buffer, descriptor, contents);
}

#endif
