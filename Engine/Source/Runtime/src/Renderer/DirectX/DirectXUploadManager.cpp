#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/UploadManager.h"
#include "DirectXDevice.h"
#include "DirectXUtils.h"
#include "DirectXTransitionManager.h"

using namespace Gleam;

struct UploadManager::Impl
{
	ID3D12Fence* fence = nullptr;
	uint32_t fenceValue = 0;

	using UploadCommand = std::function<void(ID3D12GraphicsCommandList7* commandList)>;
	TArray<UploadCommand> uploadQueue;
};

UploadManager::UploadManager(GraphicsDevice* device)
	: mHandle(CreateScope<Impl>())
	, mDevice(device)
{
	DX_CHECK(static_cast<ID3D12Device10*>(mDevice->GetHandle())->CreateFence(
		mHandle->fenceValue,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mHandle->fence)
	));
}

UploadManager::~UploadManager()
{
	mHandle->fence->Release();
}

void UploadManager::Commit()
{
	auto commandList = static_cast<DirectXDevice*>(mDevice)->AllocateCommandList(D3D12_COMMAND_LIST_TYPE_COPY);
	for (auto& command : mHandle->uploadQueue)
	{
		command(commandList);
	}
	mHandle->uploadQueue.clear();

	ID3D12CommandList* cmd = commandList;
	static_cast<DirectXDevice*>(mDevice)->GetDirectQueue()->ExecuteCommandLists(1, &cmd);
	static_cast<DirectXDevice*>(mDevice)->GetDirectQueue()->Signal(mHandle->fence, ++mHandle->fenceValue);
}

void UploadManager::CommitUpload(const Buffer& buffer, const void* data, size_t size, size_t offset)
{
	auto bufferContents = buffer.GetContents();
	if (bufferContents == nullptr)
	{
		D3D12_RESOURCE_DESC resourceDesc = {
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
			.Width = size,
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc = {.Count = 1, .Quality = 0 },
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = D3D12_RESOURCE_FLAG_NONE
		};

		D3D12_HEAP_PROPERTIES heapProperties = {
			.Type = D3D12_HEAP_TYPE_UPLOAD,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 0,
			.VisibleNodeMask = 0
		};

		ID3D12Resource* stagingBuffer = nullptr;
		DX_CHECK(static_cast<ID3D12Device10*>(mDevice->GetHandle())->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			nullptr,
			IID_PPV_ARGS(&stagingBuffer)
		));

		void* stagingBufferPtr = nullptr;
		DX_CHECK(stagingBuffer->Map(0, nullptr, &stagingBufferPtr));
		memcpy(stagingBufferPtr, data, size);

		mHandle->uploadQueue.emplace_back([=, this](ID3D12GraphicsCommandList7* cmd)
		{
			auto dstBuffer = static_cast<ID3D12Resource*>(buffer.GetHandle());
			DirectXTransitionManager::TransitionLayout(cmd, dstBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
			cmd->CopyBufferRegion(dstBuffer, offset, stagingBuffer, 0, size);
			DirectXTransitionManager::TransitionLayout(cmd, dstBuffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

			mDevice->AddPooledObject([this, buffer = stagingBuffer]() mutable
			{
				buffer->Release();
			});
		});
		
	}
	else
	{
		memcpy(static_cast<uint8_t*>(bufferContents) + offset, data, size);
	}
}

void UploadManager::CommitUpload(const Texture& texture, const void* data, size_t size, uint32_t dstX, uint32_t dstY, uint32_t dstZ)
{

}

#endif