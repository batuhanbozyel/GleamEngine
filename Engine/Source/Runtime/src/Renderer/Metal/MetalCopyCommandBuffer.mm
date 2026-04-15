#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/CopyCommandBuffer.h"
#include "MetalDevice.h"
#include "MetalUtils.h"

using namespace Gleam;

struct CopyCommandBuffer::Impl
{
    id<MTLIOCommandQueue> fileCommandQueue{ nil };
    id<MTLIOCommandBuffer> fileCommandBuffer{ nil };
    
    id<MTL4CommandBuffer> memoryCommandBuffer{ nil };
    id<MTL4CommandAllocator> memoryCommandAllocator{ nil };
    id<MTL4ComputeCommandEncoder> memoryCommandEncoder{ nil };
    
    id<MTLSharedEvent> memoryEvent = nil;
    uint64_t memoryEventValue = 0;
    
    id<MTLBuffer> stagingBuffer{ nil };
    void* stagingBufferPtr = nullptr;
    size_t stagingBufferOffset = 0;
    
    TArray<void*> tempBuffers;
    TArray<Buffer> bufferCopies;
    TArray<Texture> textureCopies;

    bool CopyUploadData(const void* data, size_t size)
	{
        if (stagingBufferOffset + size < UploadHeapSize)
		{
			auto dst = OffsetPointer(stagingBufferPtr, stagingBufferOffset);
			memcpy(dst, data, size);

            stagingBufferOffset += size;
			return true;
		}
		return false;
	}
};

CopyCommandBuffer::CopyCommandBuffer(GraphicsDevice* device)
    : mHandle(CreateScope<Impl>())
    , mDevice(device)
{
    __autoreleasing NSError* error = nil;
    
    // Create file queue
    MTLIOCommandQueueDescriptor* ioQueueDescriptor = [MTLIOCommandQueueDescriptor new];
    ioQueueDescriptor.type = MTLIOCommandQueueTypeConcurrent;
    ioQueueDescriptor.priority = MTLIOPriorityNormal;
    ioQueueDescriptor.maxCommandsInFlight = 0;
    mHandle->fileCommandQueue = [mDevice->GetHandle() newIOCommandQueueWithDescriptor:ioQueueDescriptor error:&error];
    GLEAM_ASSERT(mHandle->fileCommandQueue, "Metal: CopyCommandBuffer file command queue creation failed.");
    
    // Create memory command allocator
    MTL4CommandAllocatorDescriptor* allocatorDescriptor = [MTL4CommandAllocatorDescriptor new];
    allocatorDescriptor.label = @"CopyCommandBuffer::MemoryCommandAllocator";
    mHandle->memoryCommandAllocator = [mDevice->GetHandle() newCommandAllocatorWithDescriptor:allocatorDescriptor error:&error];
    GLEAM_ASSERT(mHandle->memoryCommandAllocator, "Metal: CopyCommandBuffer command allocator creation failed.");
    
    // Create memory command buffer
    mHandle->memoryCommandBuffer = [mDevice->GetHandle() newCommandBuffer];
    mHandle->memoryCommandBuffer.label = @"CopyCommandBuffer";
    
    mHandle->memoryEvent = [mDevice->GetHandle() newSharedEvent];
    mHandle->memoryEvent.label = @"CopyCommandBuffer::MemoryEvent";
    
    mHandle->stagingBuffer = [mDevice->GetHandle() newBufferWithLength:UploadHeapSize options:MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined];
    mHandle->stagingBufferPtr = [mHandle->stagingBuffer contents];
    [static_cast<MetalDevice*>(mDevice)->GetResidencySet() addAllocation:mHandle->stagingBuffer];
}

CopyCommandBuffer::~CopyCommandBuffer()
{
    WaitUntilCompleted();
    
    mHandle->stagingBuffer = nil;
    
    mHandle->fileCommandQueue = nil;
    mHandle->fileCommandBuffer = nil;
    
    mHandle->memoryCommandBuffer = nil;
    mHandle->memoryCommandAllocator = nil;
    mHandle->memoryCommandEncoder = nil;
    mHandle->memoryEvent = nil;
}

void CopyCommandBuffer::Barrier(const CommandBuffer* cmd) const
{
    if (mHandle->bufferCopies.empty() && mHandle->textureCopies.empty())
    {
        return;
    }
    
    BarrierGroup barrier;
    barrier.bufferBarriers.reserve(mHandle->bufferCopies.size());
    barrier.textureBarriers.reserve(mHandle->textureCopies.size());

    for (const auto& buffer : mHandle->bufferCopies)
    {
        BufferBarrier bufferBarrier;
        bufferBarrier.resource = buffer.GetHandle();
        bufferBarrier.srcStage = BarrierStage::Copy;
        bufferBarrier.dstStage = BarrierStage::AllShading;
        bufferBarrier.srcAccess = BarrierAccess::CopyDest;
        bufferBarrier.dstAccess = BarrierAccess::ShaderResource;
        barrier.bufferBarriers.push_back(bufferBarrier);
    }

    for (const auto& texture : mHandle->textureCopies)
    {
        TextureBarrier textureBarrier;
        textureBarrier.resource = texture.GetHandle();
        textureBarrier.srcStage = BarrierStage::Copy;
        textureBarrier.dstStage = BarrierStage::AllShading;
        textureBarrier.srcAccess = BarrierAccess::CopyDest;
        textureBarrier.dstAccess = BarrierAccess::ShaderResource;
        textureBarrier.oldLayout = BarrierLayout::Common;
        textureBarrier.newLayout = BarrierLayout::ShaderResource;
        barrier.textureBarriers.push_back(textureBarrier);
    }
    cmd->Barrier(barrier);

    mHandle->bufferCopies.clear();
    mHandle->textureCopies.clear();
}

void CopyCommandBuffer::Execute() const
{
    if (mHandle->fileCommandBuffer != nil)
    {
        [mHandle->fileCommandBuffer commit];
    }
    
    if (mHandle->memoryCommandEncoder != nil)
    {
        [mHandle->memoryCommandEncoder endEncoding];
        [mHandle->memoryCommandBuffer endCommandBuffer];
        
        id<MTL4CommandQueue> commandQueue = static_cast<MetalDevice*>(mDevice)->GetCommandQueue();
        [static_cast<MetalDevice*>(mDevice)->GetResidencySet() commit];
        [commandQueue commit:&mHandle->memoryCommandBuffer count:1];
        [commandQueue signalEvent:mHandle->memoryEvent value:++mHandle->memoryEventValue];
    }
}

void CopyCommandBuffer::WaitUntilCompleted() const
{
    if (mHandle->memoryCommandEncoder != nil)
    {
        WaitForMTLSharedEvent(mHandle->memoryEvent, mHandle->memoryEventValue);
        [mHandle->memoryCommandAllocator reset];
        mHandle->memoryCommandEncoder = nil;
    }
    
    if (mHandle->fileCommandBuffer != nil)
    {
        [mHandle->fileCommandBuffer waitUntilCompleted];
        mHandle->fileCommandBuffer = nil;
    }
    
    mHandle->stagingBufferOffset = 0;
    for (void* buffer : mHandle->tempBuffers)
    {
        id<MTLBuffer> mtlBuffer = (id<MTLBuffer>)CFBridgingRelease(buffer);
        [static_cast<MetalDevice*>(mDevice)->GetResidencySet() removeAllocation:mtlBuffer];
    }
    mHandle->tempBuffers.clear();
}

void CopyCommandBuffer::Commit(const Buffer& buffer, const void* data, size_t size, size_t offset) const
{
    auto bufferContents = buffer.GetContents();
    if (bufferContents == nullptr)
    {
        if (mHandle->memoryCommandEncoder == nil)
        {
            [mHandle->memoryCommandBuffer beginCommandBufferWithAllocator:mHandle->memoryCommandAllocator];
            mHandle->memoryCommandEncoder = [mHandle->memoryCommandBuffer computeCommandEncoder];
            [mHandle->memoryCommandEncoder setLabel:TO_NSSTRING("CopyCommandBuffer::MemoryCommandEncoder")];
        }
        
        mHandle->stagingBufferOffset = Math::AlignUp(mHandle->stagingBufferOffset, (size_t)4u);
        size_t srcOffset = mHandle->stagingBufferOffset;
        
        id<MTLBuffer> dstBuffer = buffer.GetHandle();
        if (mHandle->CopyUploadData(data, size))
        {
            [mHandle->memoryCommandEncoder copyFromBuffer:mHandle->stagingBuffer sourceOffset:srcOffset toBuffer:dstBuffer destinationOffset:offset size:size];
        }
        else
        {
            id<MTLBuffer> srcBuffer = [mDevice->GetHandle() newBufferWithBytes:data length:size options:MTLResourceStorageModeShared];
            [static_cast<MetalDevice*>(mDevice)->GetResidencySet() addAllocation:srcBuffer];
            mHandle->tempBuffers.push_back((__bridge_retained void*)srcBuffer);
            
            [mHandle->memoryCommandEncoder copyFromBuffer:srcBuffer sourceOffset:0 toBuffer:dstBuffer destinationOffset:offset size:size];
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
    if (mHandle->memoryCommandEncoder == nil)
    {
        [mHandle->memoryCommandBuffer beginCommandBufferWithAllocator:mHandle->memoryCommandAllocator];
        mHandle->memoryCommandEncoder = [mHandle->memoryCommandBuffer computeCommandEncoder];
        [mHandle->memoryCommandEncoder setLabel:TO_NSSTRING("CopyCommandBuffer::MemoryCommandEncoder")];
    }
    const auto& texDesc = texture.GetDescriptor();
    
    id<MTLTexture> dstTexture = texture.GetHandle();
    MTLSize sourceSize = MTLSizeMake(Math::Max(static_cast<uint32_t>(texDesc.size.width) >> mip, 1u),
                                     Math::Max(static_cast<uint32_t>(texDesc.size.height) >> mip, 1u),
                                    (texDesc.dimension == TextureDimension::Texture3D) ? Math::Max(texDesc.depth >> mip, 1u) : 1);

    size_t formatSize = Utils::GetTextureFormatSizeInBytes(texDesc.format);
    size_t sourceBytesPerRow = Math::AlignUp(sourceSize.width * formatSize, formatSize);
    size_t sourceBytesPerImage = (texDesc.dimension == TextureDimension::Texture3D) ? sourceBytesPerRow * sourceSize.height : 0;
    
    mHandle->stagingBufferOffset = Math::AlignUp(mHandle->stagingBufferOffset, formatSize);
    size_t srcOffset = mHandle->stagingBufferOffset;
    if (mHandle->CopyUploadData(data, size))
    {
        [mHandle->memoryCommandEncoder copyFromBuffer:mHandle->stagingBuffer
                                         sourceOffset:srcOffset
                                    sourceBytesPerRow:sourceBytesPerRow
                                  sourceBytesPerImage:sourceBytesPerImage
                                           sourceSize:sourceSize
                                            toTexture:dstTexture
                                     destinationSlice:slice
                                     destinationLevel:mip
                                    destinationOrigin:MTLOriginMake(0, 0, 0)];
    }
    else
    {
        id<MTLBuffer> srcBuffer = [mDevice->GetHandle() newBufferWithBytes:data length:size options:MTLResourceStorageModeShared];
        [static_cast<MetalDevice*>(mDevice)->GetResidencySet() addAllocation:srcBuffer];
        mHandle->tempBuffers.push_back((__bridge_retained void*)srcBuffer);
        
        [mHandle->memoryCommandEncoder copyFromBuffer:srcBuffer
                                         sourceOffset:0
                                    sourceBytesPerRow:sourceBytesPerRow
                                  sourceBytesPerImage:sourceBytesPerImage
                                           sourceSize:sourceSize
                                            toTexture:dstTexture
                                     destinationSlice:slice
                                     destinationLevel:mip
                                    destinationOrigin:MTLOriginMake(0, 0, 0)];
    }
    
    auto it = eastl::find_if(mHandle->textureCopies.begin(), mHandle->textureCopies.end(), [&](const Texture& t) { return t.GetHandle() == texture.GetHandle(); });
	if (it == mHandle->textureCopies.end())
	{
		mHandle->textureCopies.push_back(texture);
	}
}

#endif
