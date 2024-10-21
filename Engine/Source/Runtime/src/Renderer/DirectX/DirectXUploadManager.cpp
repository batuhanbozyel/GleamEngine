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
	
	size_t uploadBufferOffset = 0;
	TArray<uint8_t, UploadHeapSize> uploadBuffer;
	
	void* CopyUploadData(const void* data, size_t size)
	{
		if (uploadBufferOffset + size < uploadBuffer.size())
		{
			auto dst = OffsetPointer(uploadBuffer.data(), uploadBufferOffset);
			memcpy(dst, data, size);
			uploadBufferOffset += size;
			return dst;
		}
		return nullptr;
	}
};

UploadManager::UploadManager(GraphicsDevice* device)
	: mHandle(CreateScope<Impl>())
	, mDevice(device)
{
	DX_CHECK(DStorageGetFactory(IID_PPV_ARGS(&mHandle->factory)));
	mHandle->factory->SetStagingBufferSize(UploadHeapSize);
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
	mHandle->fileFence->Release();
	mHandle->fileQueue->Release();

	mHandle->memoryFence->Release();
	mHandle->memoryQueue->Release();

	mHandle->factory->Release();
}

void UploadManager::Commit()
{
	mHandle->fileQueue->EnqueueSignal(mHandle->fileFence, mHandle->fileFenceValue);
	mHandle->fileQueue->Submit();

	mHandle->memoryQueue->EnqueueSignal(mHandle->memoryFence, mHandle->memoryFenceValue);
	mHandle->memoryQueue->Submit();

	mUploadBufferOffset = 0;
}

void UploadManager::CommitUpload(const Buffer& buffer, const void* data, size_t size, size_t offset)
{
	auto bufferContents = buffer.GetContents();
	if (bufferContents == nullptr)
	{
		auto dstBuffer = static_cast<ID3D12Resource*>(buffer.GetHandle());
		auto size32 = static_cast<uint32_t>(size);
		auto srcData = mHandle->CopyUploadData(data, size);

		if (srcData)
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

			request.UncompressedSize = size32;
			mHandle->memoryQueue->EnqueueRequest(&request);
		}
	}
	else
	{
		memcpy(OffsetPointer(bufferContents, offset), data, size);
	}
}

void UploadManager::CommitUpload(const Texture& texture, const void* data, size_t size)
{
	auto dstTexture = static_cast<ID3D12Resource*>(texture.GetHandle());
	auto size32 = static_cast<uint32_t>(size);
	auto srcData = mHandle->CopyUploadData(data, size);

	if (srcData)
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
}

#endif
