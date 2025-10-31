#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/Heap.h"
#include "Renderer/Buffer.h"

#include "DirectXTransitionManager.h"
#include "DirectXUtils.h"

using namespace Gleam;

Buffer Heap::Allocate(const BufferDescriptor& descriptor)
{
	auto alignedStackPtr = Utils::AlignUp(mStackPtr, mAlignment);
	auto newStackPtr = alignedStackPtr + descriptor.size;

	if (Utils::AlignUp(mDescriptor.size, mAlignment) < newStackPtr)
	{
		GLEAM_ASSERT(false, "DirectX: Heap is full.");
		return Buffer(descriptor);
	}
	mStackPtr = newStackPtr;

	auto initialState = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	auto flags = D3D12_RESOURCE_FLAG_NONE;
	if (mDescriptor.memoryType != MemoryType::CPU)
	{
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	D3D12_RESOURCE_DESC resourceDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment = mAlignment,
		.Width = descriptor.size,
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
    
    TStringStream ss;
	ss << mDescriptor.name << "::" << descriptor.name;
	TWString resourceName = ss.str();

	resource->SetName(resourceName.c_str());
	DirectXTransitionManager::SetLayout(resource, initialState);

    void* contents = nullptr;
    if (mDescriptor.memoryType != MemoryType::GPU)
	{
		DX_CHECK(resource->Map(0, nullptr, &contents));
	}
    
    Buffer buffer(descriptor);
    buffer.mHandle = resource;
    buffer.mContents = contents;
	buffer.mResourceView = static_cast<DirectXDevice*>(mDevice)->CreateResourceView(buffer);
    return buffer;
}

void Heap::Free(Buffer& buffer)
{
	DirectXTransitionManager::RemoveResource(static_cast<ID3D12Resource*>(buffer.mHandle));
	static_cast<DirectXDevice*>(mDevice)->ReleaseResourceView(buffer.mResourceView);
	static_cast<ID3D12Resource*>(buffer.mHandle)->Release();
	buffer.mResourceView = InvalidResourceIndex;
	buffer.mContents = nullptr;
	buffer.mHandle = nullptr;
}

#endif
