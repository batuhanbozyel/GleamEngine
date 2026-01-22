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
    
    id<MTLEvent> memoryEvent = nil;
    uint64_t memoryEventValue = 1;
    uint64_t waitMemoryEventValue = 0;
    
    id<MTLBuffer> stagingBuffer{ nil };
    void* stagingBufferPtr = nullptr;
    size_t stagingBufferOffset = 0;
    
    TArray<void*> tempBuffers;

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
    
    mHandle->memoryEvent = [mDevice->GetHandle() newEvent];
    mHandle->memoryEvent.label = @"CopyCommandBuffer::MemoryEvent";
    
    mHandle->stagingBuffer = [mDevice->GetHandle() newBufferWithLength:UploadHeapSize options:MTLResourceStorageModeShared];
    mHandle->stagingBufferPtr = [mHandle->stagingBuffer contents];
}

CopyCommandBuffer::~CopyCommandBuffer()
{
    WaitUntilCompleted();
    for (void* buffer : mHandle->tempBuffers)
    {
        CFRelease(buffer);
    }
    mHandle->tempBuffers.clear();
    
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
    if (mHandle->memoryCommandEncoder)
    {
        [mHandle->memoryCommandEncoder barrierAfterStages:MTLStageBlit beforeQueueStages:MTLStageAll visibilityOptions:MTL4VisibilityOptionDevice];
    }
}

void CopyCommandBuffer::Execute() const
{
    if (mHandle->fileCommandBuffer != nil)
    {
        [mHandle->fileCommandBuffer commit];
    }
    
    if (mHandle->memoryCommandEncoder != nil)
    {
        mHandle->waitMemoryEventValue = mHandle->memoryEventValue;
        
        [mHandle->memoryCommandEncoder endEncoding];
        [mHandle->memoryCommandBuffer endCommandBuffer];
        
        id<MTL4CommandQueue> commandQueue = static_cast<MetalDevice*>(mDevice)->GetCommandQueue();
        [commandQueue commit:&mHandle->memoryCommandBuffer count:1];
        [commandQueue signalEvent:mHandle->memoryEvent value:mHandle->memoryEventValue++];
    }
}

void CopyCommandBuffer::WaitUntilCompleted() const
{
    mHandle->stagingBufferOffset = 0;
    if (mHandle->memoryCommandEncoder != nil)
    {
        [static_cast<MetalDevice*>(mDevice)->GetCommandQueue() waitForEvent:mHandle->memoryEvent value:mHandle->waitMemoryEventValue];
        [mHandle->memoryCommandAllocator reset];
        mHandle->memoryCommandEncoder = nil;
    }
    
    if (mHandle->fileCommandBuffer != nil)
    {
        [mHandle->fileCommandBuffer waitUntilCompleted];
        mHandle->fileCommandBuffer = nil;
    }
    
    for (void* buffer : mHandle->tempBuffers)
    {
        CFRelease(buffer);
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
            [mHandle->memoryCommandEncoder setLabel:TO_NSSTRING("CopyCommandBuffer::Commit")];
        }
        
        id<MTLBuffer> dstBuffer = buffer.GetHandle();
        size_t srcOffset = mHandle->stagingBufferOffset;
        if (mHandle->CopyUploadData(data, size))
        {
            [mHandle->memoryCommandEncoder copyFromBuffer:mHandle->stagingBuffer sourceOffset:srcOffset toBuffer:dstBuffer destinationOffset:offset size:size];
        }
        else
        {
            id<MTLBuffer> srcBuffer = [mDevice->GetHandle() newBufferWithBytes:data length:size options:MTLResourceStorageModeShared];
            mHandle->tempBuffers.push_back((__bridge_retained void*)srcBuffer);
            
            [mHandle->memoryCommandEncoder copyFromBuffer:srcBuffer sourceOffset:0 toBuffer:dstBuffer destinationOffset:offset size:size];
        }
    }
    else
    {
        memcpy(OffsetPointer(bufferContents, offset), data, size);
    }
}

void CopyCommandBuffer::Commit(const Texture& texture, const void* data, size_t size) const
{
    if (mHandle->memoryCommandEncoder == nil)
    {
        [mHandle->memoryCommandBuffer beginCommandBufferWithAllocator:mHandle->memoryCommandAllocator];
        mHandle->memoryCommandEncoder = [mHandle->memoryCommandBuffer computeCommandEncoder];
        [mHandle->memoryCommandEncoder setLabel:TO_NSSTRING("CopyCommandBuffer::Commit")];
    }
    
    id<MTLTexture> dstTexture = texture.GetHandle();
    size_t sourceBytesPerRow = texture.GetDescriptor().size.width * Utils::GetTextureFormatSizeInBytes(texture.GetDescriptor().format);
    size_t sourceBytesPerImage = sourceBytesPerRow * texture.GetDescriptor().size.height;
    MTLSize sourceSize = MTLSizeMake(texture.GetDescriptor().size.width, texture.GetDescriptor().size.height, 1);
    
    size_t srcOffset = mHandle->stagingBufferOffset;
    if (mHandle->CopyUploadData(data, size))
    {
        [mHandle->memoryCommandEncoder copyFromBuffer:mHandle->stagingBuffer
                                         sourceOffset:srcOffset
                                    sourceBytesPerRow:sourceBytesPerRow
                                  sourceBytesPerImage:sourceBytesPerImage
                                           sourceSize:sourceSize
                                            toTexture:dstTexture
                                     destinationSlice:0
                                     destinationLevel:0
                                    destinationOrigin:MTLOriginMake(0, 0, 0)];
    }
    else
    {
        id<MTLBuffer> srcBuffer = [mDevice->GetHandle() newBufferWithBytes:data length:size options:MTLResourceStorageModeShared];
        mHandle->tempBuffers.push_back((__bridge_retained void*)srcBuffer);
        
        [mHandle->memoryCommandEncoder copyFromBuffer:srcBuffer
                                         sourceOffset:0
                                    sourceBytesPerRow:sourceBytesPerRow
                                  sourceBytesPerImage:sourceBytesPerImage
                                           sourceSize:sourceSize
                                            toTexture:dstTexture
                                     destinationSlice:0
                                     destinationLevel:0
                                    destinationOrigin:MTLOriginMake(0, 0, 0)];
    }
}

#endif
