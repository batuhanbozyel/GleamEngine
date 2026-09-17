#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/CopyCommandBuffer.h"
#include "Renderer/RenderSystem.h"
#include "Core/Engine.h"
#include "DirectXDevice.h"
#include "DirectXUtils.h"

#include <dstorage.h>

using namespace Gleam;

struct CopyCommandBuffer::Impl
{
	DirectXCommandPool* commandPool = nullptr;
	IDStorageFactory* factory = nullptr;

	IDStorageQueue* memoryQueue = nullptr;
	ID3D12Fence* memoryFence = nullptr;

	IDStorageQueue* fileQueue = nullptr;
	ID3D12Fence* fileFence = nullptr;
	uint64_t fenceValue = 0;
	
	size_t stagingBufferOffset = 0;
	TArray<uint8_t> stagingBuffer;

	TArray<Buffer> stagingBuffers;
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

CopyCommandBuffer::CopyCommandBuffer(GraphicsDevice* device)
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
	DX_CHECK(static_cast<ID3D12Device10*>(mDevice->GetHandle())->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mHandle->fileFence)));

	queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
	queueDesc.Name = "DStorage Memory Queue";
	DX_CHECK(mHandle->factory->CreateQueue(&queueDesc, IID_PPV_ARGS(&mHandle->memoryQueue)));
	DX_CHECK(static_cast<ID3D12Device10*>(mDevice->GetHandle())->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mHandle->memoryFence)));
}

CopyCommandBuffer::~CopyCommandBuffer()
{
	WaitUntilCompleted();
	
	mHandle->fileFence->Release();
	mHandle->fileQueue->Release();

	mHandle->memoryFence->Release();
	mHandle->memoryQueue->Release();

	mHandle->factory->Release();
}

void CopyCommandBuffer::Barrier(const CommandBuffer* cmd) const
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

void CopyCommandBuffer::Execute() const
{
	TArray<D3D12_TEXTURE_BARRIER> textureBarriers;
	textureBarriers.reserve(mHandle->textureCopies.size());

	for (uint32_t i = 0; i < mHandle->textureCopies.size(); ++i)
	{
		D3D12_TEXTURE_BARRIER barrier = {};
		barrier.SyncBefore = D3D12_BARRIER_SYNC_NONE;
		barrier.SyncAfter = D3D12_BARRIER_SYNC_NONE;
		barrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
		barrier.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
		barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_UNDEFINED;
		barrier.LayoutAfter = D3D12_BARRIER_LAYOUT_COMMON;
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

	if (not textureBarriers.empty())
	{
		D3D12_BARRIER_GROUP barrierGroup = {};
		barrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
		barrierGroup.NumBarriers = static_cast<UINT32>(textureBarriers.size());
		barrierGroup.pTextureBarriers = textureBarriers.data();

		mHandle->commandPool = static_cast<DirectXDevice*>(mDevice)->GetCopyQueue().AcquirePool();
		auto cmd = mHandle->commandPool->AllocateCommandList(L"CopyCommandBuffer::PreCopyBarriers");
		cmd->Barrier(1, &barrierGroup);
		cmd->Close();
		static_cast<DirectXDevice*>(mDevice)->GetCopyQueue().ExecuteCommandLists(1, &cmd);
	}

	++mHandle->fenceValue;
	mHandle->fileQueue->EnqueueSignal(mHandle->fileFence, mHandle->fenceValue);
	mHandle->fileQueue->Submit();

	mHandle->memoryQueue->EnqueueSignal(mHandle->memoryFence, mHandle->fenceValue);
	mHandle->memoryQueue->Submit();
}

void CopyCommandBuffer::WaitUntilCompleted() const
{
	WaitForID3D12Fence(mHandle->fileFence, mHandle->fenceValue);
	WaitForID3D12Fence(mHandle->memoryFence, mHandle->fenceValue);

	mHandle->stagingBufferOffset = 0;
	if (not mHandle->stagingBuffers.empty())
	{
		static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
		for (auto& stagingBuffer : mHandle->stagingBuffers)
		{
			mDevice->Dispose(renderSystem->GetAllocator(), stagingBuffer, BarrierStage::None);
		}
		mHandle->stagingBuffers.clear();
	}
}

StorageFile CopyCommandBuffer::OpenFile(const Path& path) const
{
	IDStorageFile* file = nullptr;
	DX_CHECK(mHandle->factory->OpenFile(path.Native().c_str(), IID_PPV_ARGS(&file)));
	return StorageFile(file);
}

void CopyCommandBuffer::CloseFile(StorageFile& file) const
{
	if (not file.IsValid())
	{
		return;
	}

	auto handle = static_cast<IDStorageFile*>(file.GetHandle());
	handle->Close();
	handle->Release();
	file = StorageFile();
}

void CopyCommandBuffer::Commit(const Buffer& buffer, const void* data, size_t size, size_t offset) const
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
			static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
			BufferDescriptor stagingDesc;
			stagingDesc.name = "CopyCommandBuffer::StagingBuffer";
			stagingDesc.memoryType = MemoryType::CPU;
			stagingDesc.size = size;
			auto uploadBuffer = mDevice->CreateBuffer(renderSystem->GetAllocator(), stagingDesc);
			memcpy(uploadBuffer.GetContents(), data, size);
			mHandle->stagingBuffers.push_back(uploadBuffer);

			DSTORAGE_REQUEST request = {};
			request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
			request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;
			request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;

			request.Source.Memory.Source = uploadBuffer.GetContents();
			request.Source.Memory.Size = size32;

			request.Destination.Buffer.Resource = dstBuffer;
			request.Destination.Buffer.Offset = offset;
			request.Destination.Buffer.Size = size32;

			request.UncompressedSize = 0;
			mHandle->memoryQueue->EnqueueRequest(&request);
		}

		auto it = eastl::find_if(mHandle->bufferCopies.begin(), mHandle->bufferCopies.end(), [&](const Buffer& b) { return b.GetHandle() == buffer.GetHandle(); });
		if (it == mHandle->bufferCopies.end())
		{
			mHandle->bufferCopies.push_back(buffer);
		}
	}
	else
	{
		memcpy(OffsetPointer(bufferContents, offset), data, size);
	}
}

void CopyCommandBuffer::Commit(const Texture& texture, const void* data, size_t size, uint32_t mip, uint32_t slice) const
{
	auto dstTexture = static_cast<ID3D12Resource*>(texture.GetHandle());
	auto device = static_cast<ID3D12Device10*>(mDevice->GetHandle());
	const auto& texDesc = texture.GetDescriptor();

	const uint32_t subresourceIndex = texture.GetSubresourceIndex(mip, slice);
	const auto dstDesc = dstTexture->GetDesc();
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
	UINT numRows = 0;
	UINT64 rowSizeInBytes = 0;
	UINT64 totalBytes = 0;
	device->GetCopyableFootprints(&dstDesc, subresourceIndex, 1, 0, &footprint, &numRows, &rowSizeInBytes, &totalBytes);
	
	D3D12_BOX region = {};
	region.right = Math::Max(static_cast<uint32_t>(texDesc.size.width) >> mip, 1u);
	region.bottom = Math::Max(static_cast<uint32_t>(texDesc.size.height) >> mip, 1u);
	region.back = (texDesc.dimension == TextureDimension::Texture3D) ? Math::Max(texDesc.depth >> mip, 1u) : 1;

	const UINT srcRowPitch = static_cast<UINT>(rowSizeInBytes);
	const UINT dstRowPitch = footprint.Footprint.RowPitch;

	const void* srcData = nullptr;
	if (srcRowPitch == dstRowPitch)
	{
		if (auto stagingData = mHandle->CopyUploadData(data, size); stagingData)
		{
			srcData = stagingData;
		}
		else
		{
			static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
			BufferDescriptor stagingDesc;
			stagingDesc.name = "CopyCommandBuffer::StagingBuffer";
			stagingDesc.memoryType = MemoryType::CPU;
			stagingDesc.size = size;
			auto uploadBuffer = mDevice->CreateBuffer(renderSystem->GetAllocator(), stagingDesc);
			memcpy(uploadBuffer.GetContents(), data, size);
			mHandle->stagingBuffers.push_back(uploadBuffer);
			srcData = uploadBuffer.GetContents();
		}
	}
	else
	{
		static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
		BufferDescriptor stagingDesc;
		stagingDesc.name = "CopyCommandBuffer::StagingBuffer";
		stagingDesc.memoryType = MemoryType::CPU;
		stagingDesc.size = totalBytes;
		auto uploadBuffer = mDevice->CreateBuffer(renderSystem->GetAllocator(), stagingDesc);
		mHandle->stagingBuffers.push_back(uploadBuffer);

		auto stagingBufferPtr = uploadBuffer.GetContents();
		for (UINT depth = 0; depth < region.back; ++depth)
		{
			for (UINT row = 0; row < numRows; ++row)
			{
				UINT srcOffset = (depth * numRows + row) * srcRowPitch;
				UINT dstOffset = (depth * numRows + row) * dstRowPitch;
				memcpy(
					static_cast<uint8_t*>(stagingBufferPtr) + dstOffset,
					static_cast<const uint8_t*>(data) + srcOffset,
					srcRowPitch);
			}
		}
		srcData = stagingBufferPtr;
	}

	DSTORAGE_REQUEST request = {};
	request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
	request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION;
	request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;

	request.Source.Memory.Source = srcData;
	request.Source.Memory.Size = static_cast<uint32_t>(totalBytes);

	request.Destination.Texture.Resource = dstTexture;
	request.Destination.Texture.SubresourceIndex = texture.GetSubresourceIndex(mip, slice);
	request.Destination.Texture.Region = region;

	request.UncompressedSize = 0;
	mHandle->memoryQueue->EnqueueRequest(&request);

	auto it = eastl::find_if(mHandle->textureCopies.begin(), mHandle->textureCopies.end(), [&](const Texture& t) { return t.GetHandle() == texture.GetHandle(); });
	if (it == mHandle->textureCopies.end())
	{
		mHandle->textureCopies.push_back(texture);
	}
}

void CopyCommandBuffer::Commit(const Buffer& buffer, const StorageFile& file, const BufferRange& range, size_t offset) const
{
	GLEAM_ASSERT(buffer.GetContents() == nullptr, "CopyCommandBuffer: file upload destination must be a GPU buffer.");

	DSTORAGE_REQUEST request = {};
	request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
	request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;
	request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;

	request.Source.File.Source = static_cast<IDStorageFile*>(file.GetHandle());
	request.Source.File.Offset = range.offset;
	request.Source.File.Size = static_cast<uint32_t>(range.size);

	request.Destination.Buffer.Resource = static_cast<ID3D12Resource*>(buffer.GetHandle());
	request.Destination.Buffer.Offset = offset;
	request.Destination.Buffer.Size = static_cast<uint32_t>(range.size);

	request.UncompressedSize = 0;
	mHandle->fileQueue->EnqueueRequest(&request);

	auto it = eastl::find_if(mHandle->bufferCopies.begin(), mHandle->bufferCopies.end(), [&](const Buffer& b) { return b.GetHandle() == buffer.GetHandle(); });
	if (it == mHandle->bufferCopies.end())
	{
		mHandle->bufferCopies.push_back(buffer);
	}
}

#endif
