#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/Heap.h"
#include "Renderer/Buffer.h"

#include "DirectXTransitionManager.h"
#include "DirectXUtils.h"

using namespace Gleam;

Buffer Heap::CreateBuffer(size_t size) const
{
	auto alignedStackPtr = Utils::AlignUp(mStackPtr, mAlignment);
	auto newStackPtr = alignedStackPtr + size;

	if (Utils::AlignUp(mDescriptor.size, mAlignment) < newStackPtr)
	{
		GLEAM_ASSERT(false, "DirectX: Heap is full!");
		return Buffer(nullptr, size, nullptr);
	}
	mStackPtr = newStackPtr;

	auto initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
	auto flags = D3D12_RESOURCE_FLAG_NONE;
	if (mDescriptor.memoryType != MemoryType::CPU)
	{
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}
	else
	{
		initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	D3D12_RESOURCE_DESC resourceDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment = mAlignment,
		.Width = size,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc = {.Count = 1, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = flags
	};

	ID3D12Resource* resource = nullptr;
	DX_CHECK(static_cast<ID3D12Device10*>(mDevice->GetHandle())->CreatePlacedResource(
		static_cast<ID3D12Heap*>(mHandle),
		alignedStackPtr,
		&resourceDesc,
		initialState,
		nullptr,
		IID_PPV_ARGS(&resource)
	));
	DirectXTransitionManager::SetLayout(resource, initialState);

    void* contents = nullptr;
    if (mDescriptor.memoryType != MemoryType::GPU)
	{
		DX_CHECK(resource->Map(0, nullptr, &contents));
	}
	Buffer buffer(resource, size, contents);
	buffer.mResourceView = mDescriptor.memoryType == MemoryType::CPU ? InvalidResourceIndex : static_cast<DirectXDevice*>(mDevice)->CreateResourceView(buffer);
    return buffer;
}

#endif
