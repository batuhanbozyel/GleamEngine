#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/UploadManager.h"
#include "DirectXDevice.h"
#include "DirectXUtils.h"

#include <dstorage.h>

using namespace Gleam;

struct UploadManager::Impl
{
	IDStorageFactory* factory = nullptr;

	IDStorageQueue* memoryQueue = nullptr;
	ID3D12Fence* memoryFence = nullptr;

	IDStorageQueue* fileQueue = nullptr;
	ID3D12Fence* fileFence = nullptr;

	uint64_t fenceValue = 0;
	uint64_t waitFenceValue = 0;
	
	size_t stagingBufferOffset = 0;
	TArray<uint8_t> stagingBuffer;

	TArray<ID3D12Resource*> tempBuffers;
	TArray<Buffer> bufferCopies;
	TArray<Texture> textureCopies;
	
	const void* CopyUploadData(const void* data, size_t size)
	{
		if (stagingBufferOffset + size < UploadHeapSize)
		{
			auto dst = OffsetPointer(stagingBuffer.data(), stagingBufferOffset);
			memcpy(dst, data, size);

			stagingBufferOffset += size;
			return dst;
		}
		return nullptr;
	}
};

UploadManager::UploadManager(GraphicsDevice* device)
	: mHandle(CreateScope<Impl>())
	, mDevice(device)
{
	mHandle->stagingBuffer.resize(UploadHeapSize);
	
	DX_CHECK(DStorageGetFactory(IID_PPV_ARGS(&mHandle->factory)));
	mHandle->factory->SetStagingBufferSize(DSTORAGE_STAGING_BUFFER_SIZE_32MB);
	mHandle->factory->SetDebugFlags(DSTORAGE_DEBUG_SHOW_ERRORS | DSTORAGE_DEBUG_BREAK_ON_ERROR);

	DSTORAGE_QUEUE_DESC queueDesc = {};
	queueDesc.Device = static_cast<ID3D12Device*>(mDevice->GetHandle());
	queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
	queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;

	queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
	queueDesc.Name = "DStorage File Queue";
	DX_CHECK(mHandle->factory->CreateQueue(&queueDesc, IID_PPV_ARGS(&mHandle->fileQueue)));

	DX_CHECK(static_cast<ID3D12Device10*>(mDevice->GetHandle())->CreateFence(
		mHandle->fenceValue,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mHandle->fileFence)
	));

	queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
	queueDesc.Name = "DStorage Memory Queue";
	DX_CHECK(mHandle->factory->CreateQueue(&queueDesc, IID_PPV_ARGS(&mHandle->memoryQueue)));

	DX_CHECK(static_cast<ID3D12Device10*>(mDevice->GetHandle())->CreateFence(
		mHandle->fenceValue,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mHandle->memoryFence)
	));
}

UploadManager::~UploadManager()
{
	WaitForID3D12Fence(mHandle->memoryFence, mHandle->waitFenceValue);
	for (auto buffer : mHandle->tempBuffers)
	{
		buffer->Release();
	}
	mHandle->tempBuffers.clear();
	
	mHandle->fileFence->Release();
	mHandle->fileQueue->Release();

	mHandle->memoryFence->Release();
	mHandle->memoryQueue->Release();

	mHandle->factory->Release();
}

void UploadManager::Barrier(const CommandBuffer* cmd) const
{
	TArray<D3D12_BUFFER_BARRIER> bufferBarriers;
	TArray<D3D12_TEXTURE_BARRIER> textureBarriers;

	bufferBarriers.reserve(mHandle->bufferCopies.size());
	textureBarriers.reserve(mHandle->textureCopies.size());

	for (uint32_t i = 0; i < mHandle->bufferCopies.size(); ++i)
	{
		D3D12_BUFFER_BARRIER barrier = {};
		barrier.SyncBefore = D3D12_BARRIER_SYNC_COPY;
		barrier.SyncAfter = D3D12_BARRIER_SYNC_ALL_SHADING;
		barrier.AccessBefore = D3D12_BARRIER_ACCESS_COPY_DEST;
		barrier.AccessAfter = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
		barrier.pResource = static_cast<ID3D12Resource*>(mHandle->bufferCopies[i].GetHandle());
		barrier.Offset = 0;
		barrier.Size = UINT64_MAX;
		bufferBarriers.emplace_back(barrier);
	}

	for (uint32_t i = 0; i < mHandle->textureCopies.size(); ++i)
	{
		D3D12_TEXTURE_BARRIER barrier = {};
		barrier.SyncBefore = D3D12_BARRIER_SYNC_COPY;
		barrier.SyncAfter = D3D12_BARRIER_SYNC_ALL_SHADING;
		barrier.AccessBefore = D3D12_BARRIER_ACCESS_COPY_DEST;
		barrier.AccessAfter = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
		barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_COMMON;
		barrier.LayoutAfter = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
		barrier.pResource = static_cast<ID3D12Resource*>(mHandle->textureCopies[i].GetHandle());
		barrier.Subresources.IndexOrFirstMipLevel = 0xffffffff;
		barrier.Subresources.NumMipLevels = 0;
		barrier.Subresources.FirstArraySlice = 0;
		barrier.Subresources.NumArraySlices = 0;
		barrier.Subresources.FirstPlane = 0;
		barrier.Subresources.NumPlanes = 0;
		barrier.Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE;
		textureBarriers.emplace_back(barrier);
	}

	TArray<D3D12_BARRIER_GROUP> barrierGroups;
	barrierGroups.reserve(2);
	if (not bufferBarriers.empty())
	{
		D3D12_BARRIER_GROUP bufferGroup = {};
		bufferGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
		bufferGroup.NumBarriers = static_cast<UINT32>(bufferBarriers.size());
		bufferGroup.pBufferBarriers = bufferBarriers.data();
		barrierGroups.emplace_back(bufferGroup);
	}

	if (not textureBarriers.empty())
	{
		D3D12_BARRIER_GROUP textureGroup = {};
		textureGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
		textureGroup.NumBarriers = static_cast<UINT32>(textureBarriers.size());
		textureGroup.pTextureBarriers = textureBarriers.data();
		barrierGroups.emplace_back(textureGroup);
	}

	if (not barrierGroups.empty())
	{
		static_cast<ID3D12GraphicsCommandList7*>(cmd->GetHandle())->Barrier(static_cast<UINT32>(barrierGroups.size()), barrierGroups.data());
	}

	mHandle->bufferCopies.clear();
	mHandle->textureCopies.clear();
}

void UploadManager::Execute() const
{
	mHandle->waitFenceValue = mHandle->fenceValue++;
	mHandle->fileQueue->EnqueueSignal(mHandle->fileFence, mHandle->fenceValue);
	mHandle->fileQueue->Submit();

	mHandle->memoryQueue->EnqueueSignal(mHandle->memoryFence, mHandle->fenceValue);
	mHandle->memoryQueue->Submit();
}

void UploadManager::WaitUntilCompleted() const
{
	mHandle->stagingBufferOffset = 0;
	WaitForID3D12Fence(mHandle->memoryFence, mHandle->waitFenceValue);

	for (auto buffer : mHandle->tempBuffers)
	{
		buffer->Release();
	}
	mHandle->tempBuffers.clear();
}

void UploadManager::Commit(const Buffer& buffer, const void* data, size_t size, size_t offset) const
{
	auto bufferContents = buffer.GetContents();
	if (bufferContents == nullptr)
	{
		auto dstBuffer = static_cast<ID3D12Resource*>(buffer.GetHandle());
		auto size32 = static_cast<uint32_t>(size);
		if (auto srcData = mHandle->CopyUploadData(data, size); srcData)
		{
			DSTORAGE_REQUEST request = {};
			request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
			request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;
			request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;

			request.Source.Memory.Source = srcData;
			request.Source.Memory.Size = size32;

			request.Destination.Buffer.Resource = dstBuffer;
			request.Destination.Buffer.Offset = offset;
			request.Destination.Buffer.Size = size32;

			request.UncompressedSize = 0;
			mHandle->memoryQueue->EnqueueRequest(&request);
		}
		else
		{
			D3D12_RESOURCE_DESC1 resourceDesc = {
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = 0,
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
			DX_CHECK(static_cast<ID3D12Device10*>(mDevice->GetHandle())->CreateCommittedResource3(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_BARRIER_LAYOUT_UNDEFINED,
				nullptr,
				nullptr,
				0,
				nullptr,
				IID_PPV_ARGS(&stagingBuffer)));
			mHandle->tempBuffers.push_back(stagingBuffer);

			void* stagingBufferPtr = nullptr;
			DX_CHECK(stagingBuffer->Map(0, nullptr, &stagingBufferPtr));
			memcpy(stagingBufferPtr, data, size);
			stagingBuffer->Unmap(0, nullptr);

			DSTORAGE_REQUEST request = {};
			request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
			request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;
			request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;

			request.Source.Memory.Source = stagingBufferPtr;
			request.Source.Memory.Size = size32;

			request.Destination.Buffer.Resource = dstBuffer;
			request.Destination.Buffer.Offset = offset;
			request.Destination.Buffer.Size = size32;

			request.UncompressedSize = 0;
			mHandle->memoryQueue->EnqueueRequest(&request);
		}
		mHandle->bufferCopies.push_back(buffer);
	}
	else
	{
		memcpy(OffsetPointer(bufferContents, offset), data, size);
	}
}

void UploadManager::Commit(const Texture& texture, const void* data, size_t size) const
{
	auto dstTexture = static_cast<ID3D12Resource*>(texture.GetHandle());
	auto size32 = static_cast<uint32_t>(size);

	if (auto srcData = mHandle->CopyUploadData(data, size); srcData)
	{
		DSTORAGE_REQUEST request = {};
		request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;

		request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES;
		request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;

		request.Source.Memory.Source = srcData;
		request.Source.Memory.Size = size32;

		request.Destination.MultipleSubresources.Resource = dstTexture;
		request.Destination.MultipleSubresources.FirstSubresource = 0;

		request.UncompressedSize = 0;
		mHandle->memoryQueue->EnqueueRequest(&request);
	}
	else
	{
		D3D12_RESOURCE_DESC1 resourceDesc = {
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = 0,
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
		DX_CHECK(static_cast<ID3D12Device10*>(mDevice->GetHandle())->CreateCommittedResource3(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_BARRIER_LAYOUT_UNDEFINED,
			nullptr,
			nullptr,
			0,
			nullptr,
			IID_PPV_ARGS(&stagingBuffer)));
		mHandle->tempBuffers.push_back(stagingBuffer);

		void* stagingBufferPtr = nullptr;
		DX_CHECK(stagingBuffer->Map(0, nullptr, &stagingBufferPtr));
		memcpy(stagingBufferPtr, data, size);
		stagingBuffer->Unmap(0, nullptr);

		DSTORAGE_REQUEST request = {};
		request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;

		request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES;
		request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;

		request.Source.Memory.Source = stagingBufferPtr;
		request.Source.Memory.Size = size32;

		request.Destination.MultipleSubresources.Resource = dstTexture;
		request.Destination.MultipleSubresources.FirstSubresource = 0;

		request.UncompressedSize = 0;
		mHandle->memoryQueue->EnqueueRequest(&request);
	}
	mHandle->textureCopies.push_back(texture);
}

#endif
