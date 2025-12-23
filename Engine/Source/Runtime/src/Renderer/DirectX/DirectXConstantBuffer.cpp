#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/ConstantBuffer.h"
#include "DirectXDevice.h"
#include "DirectXUtils.h"

using namespace Gleam;

ConstantBuffer::ConstantBuffer(GraphicsDevice* device, size_t size)
	: mAlignment(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)
	, mCapacity(Utils::AlignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT))
	, mDevice(device)
{
	D3D12_HEAP_PROPERTIES heapProperties = {
		.Type = D3D12_HEAP_TYPE_UPLOAD,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 0,
		.VisibleNodeMask = 0
	};

	D3D12_RESOURCE_DESC1 resourceDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment = 0,
		.Width = mCapacity,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc = {.Count = 1, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = D3D12_RESOURCE_FLAG_NONE
	};

	ID3D12Resource* resource = nullptr;
	DX_CHECK(static_cast<ID3D12Device10*>(device->GetHandle())->CreateCommittedResource3(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_BARRIER_LAYOUT_UNDEFINED,
		nullptr,
		nullptr,
		0,
		nullptr,
		IID_PPV_ARGS(&resource)
	));
	resource->SetName(L"ConstantBuffer");
	DX_CHECK(resource->Map(0, nullptr, &mContents));
	mHandle = resource;
}

ConstantBuffer::~ConstantBuffer()
{
	static_cast<ID3D12Resource*>(mHandle)->Release();
}
#endif
