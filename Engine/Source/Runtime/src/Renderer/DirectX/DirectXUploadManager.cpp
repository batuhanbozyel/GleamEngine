#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/UploadManager.h"
#include "DirectXDevice.h"
#include "DirectXUtils.h"
#include "DirectXTransitionManager.h"

#include <dstorage.h>

using namespace Gleam;

struct UploadManager::Impl
{
	IDStorageFactory* factory = nullptr;

	IDStorageQueue* memoryQueue = nullptr;
	ID3D12Fence* memoryFence = nullptr;
	uint32_t memoryFenceValue = 0;

	IDStorageQueue* fileQueue = nullptr;
	ID3D12Fence* fileFence = nullptr;
	uint32_t fileFenceValue = 0;
	
	size_t stagingBufferOffset = 0;
	TArray<uint8_t> stagingBuffer;
	uint32_t frameIdx = 0;

	struct Context
	{
		TArray<ID3D12Resource*> tempBuffers;
		uint32_t waitFenceValue = 0;
	};
	TArray<Context> frameContext;
	
	const void* CopyUploadData(const void* data, size_t size)
	{
		if (stagingBufferOffset + size < UploadHeapSize)
		{
			size_t offset = stagingBufferOffset + frameIdx * UploadHeapSize;
			auto dst = OffsetPointer(stagingBuffer.data(), offset);
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
	mHandle->stagingBuffer.resize(UploadHeapSize * mDevice->GetFramesInFlight());
	mHandle->frameContext.resize(mDevice->GetFramesInFlight());
	
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
		mHandle->fileFenceValue,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mHandle->fileFence)
	));

	queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
	queueDesc.Name = "DStorage Memory Queue";
	DX_CHECK(mHandle->factory->CreateQueue(&queueDesc, IID_PPV_ARGS(&mHandle->memoryQueue)));

	DX_CHECK(static_cast<ID3D12Device10*>(mDevice->GetHandle())->CreateFence(
		mHandle->memoryFenceValue,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mHandle->memoryFence)
	));
}

UploadManager::~UploadManager()
{
	for (auto& frame : mHandle->frameContext)
	{
		for (auto buffer : frame.tempBuffers)
		{
			buffer->Release();
		}
	}
	mHandle->frameContext.clear();
	
	mHandle->fileFence->Release();
	mHandle->fileQueue->Release();

	mHandle->memoryFence->Release();
	mHandle->memoryQueue->Release();

	mHandle->factory->Release();
}

void UploadManager::Commit() const
{
	auto& frame = mHandle->frameContext[mHandle->frameIdx];
	frame.waitFenceValue = mHandle->memoryFenceValue;

	mHandle->fileQueue->EnqueueSignal(mHandle->fileFence, ++mHandle->fileFenceValue);
	mHandle->fileQueue->Submit();

	mHandle->memoryQueue->EnqueueSignal(mHandle->memoryFence, ++mHandle->memoryFenceValue);
	mHandle->memoryQueue->Submit();
}

void UploadManager::Reset(uint32_t frameIdx) const
{
	mHandle->frameIdx = frameIdx;
	mHandle->stagingBufferOffset = 0;

	auto& frame = mHandle->frameContext[frameIdx];
	WaitForID3D12Fence(mHandle->memoryFence, frame.waitFenceValue);

	for (auto buffer : frame.tempBuffers)
	{
		buffer->Release();
	}
	frame.tempBuffers.clear();
}

void UploadManager::CommitUpload(const Buffer& buffer, const void* data, size_t size, size_t offset) const
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
			D3D12_RESOURCE_DESC resourceDesc = {
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
			DX_CHECK(static_cast<ID3D12Device*>(mDevice->GetHandle())->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&stagingBuffer)));
			mHandle->frameContext[mHandle->frameIdx].tempBuffers.push_back(stagingBuffer);

			void* stagingBufferPtr = nullptr;
			DX_CHECK(stagingBuffer->Map(0, nullptr, &stagingBufferPtr));
			memcpy(stagingBufferPtr, data, size);

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
	}
	else
	{
		memcpy(OffsetPointer(bufferContents, offset), data, size);
	}
}

void UploadManager::CommitUpload(const Texture& texture, const void* data, size_t size) const
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
		D3D12_RESOURCE_DESC resourceDesc = {
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
		DX_CHECK(static_cast<ID3D12Device*>(mDevice->GetHandle())->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&stagingBuffer)));
		mHandle->frameContext[mHandle->frameIdx].tempBuffers.push_back(stagingBuffer);

		void* stagingBufferPtr = nullptr;
		DX_CHECK(stagingBuffer->Map(0, nullptr, &stagingBufferPtr));
		memcpy(stagingBufferPtr, data, size);

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
}

#endif
