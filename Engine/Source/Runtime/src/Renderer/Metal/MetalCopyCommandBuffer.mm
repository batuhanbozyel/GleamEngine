#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/CopyCommandBuffer.h"
#include "MetalDevice.h"
#include "MetalUtils.h"

using namespace Gleam;

struct CopyCommandBuffer::Impl
{
    id<MTLIOCommandQueue> fileCommandQueue{ nil };
    id<MTLCommandQueue> memoryCommandQueue{ nil };
    
    id<MTLIOCommandBuffer> fileCommandBuffer{ nil };
    id<MTLCommandBuffer> memoryCommandBuffer{ nil };
    
    id<MTLBuffer> stagingBuffer{ nil };
    void* stagingBufferPtr = nullptr;
    size_t stagingBufferOffset = 0;
    
    TArray<void*> tempBuffers;
    
    void AllocateMemoryCommandBuffer()
    {
    #ifdef GDEBUG
        MTLCommandBufferDescriptor* descriptor = [MTLCommandBufferDescriptor new];
        descriptor.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
        memoryCommandBuffer = [memoryCommandQueue commandBufferWithDescriptor:descriptor];
    #else
        memoryCommandBuffer = [memoryCommandQueue commandBuffer];
    #endif
    }

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
    // Create file queue
    MTLIOCommandQueueDescriptor* ioQueueDescriptor = [MTLIOCommandQueueDescriptor new];
    ioQueueDescriptor.type = MTLIOCommandQueueTypeConcurrent;
    ioQueueDescriptor.priority = MTLIOPriorityNormal;
    ioQueueDescriptor.maxCommandsInFlight = 0;
    
    __autoreleasing NSError* error = nil;
    mHandle->fileCommandQueue = [mDevice->GetHandle() newIOCommandQueueWithDescriptor:ioQueueDescriptor error:&error];
    GLEAM_ASSERT(mHandle->fileCommandQueue, "Metal: CopyCommandBuffer file command queue creation failed.");
    
    // Create memory queue
    mHandle->memoryCommandQueue = [mDevice->GetHandle() newCommandQueue];
    GLEAM_ASSERT(mHandle->memoryCommandQueue, "Metal: CopyCommandBuffer memory command queue creation failed.");
    
    mHandle->stagingBuffer = [mDevice->GetHandle() newBufferWithLength:UploadHeapSize options:MTLResourceStorageModeShared];
    mHandle->stagingBufferPtr = [mHandle->stagingBuffer contents];
}

CopyCommandBuffer::~CopyCommandBuffer()
{
    for (void* buffer : mHandle->tempBuffers)
    {
        CFRelease(buffer);
    }
    mHandle->tempBuffers.clear();
    
    mHandle->stagingBuffer = nil;
    mHandle->fileCommandQueue = nil;
    mHandle->fileCommandBuffer = nil;
    mHandle->memoryCommandBuffer = nil;
}

void CopyCommandBuffer::Barrier(const CommandBuffer* cmd) const
{
    // noop
}

void CopyCommandBuffer::Execute() const
{
    if (mHandle->fileCommandBuffer != nil)
    {
        [mHandle->fileCommandBuffer commit];
    }
    
    if (mHandle->memoryCommandBuffer != nil)
    {
        [mHandle->memoryCommandBuffer commit];
    }
}

void CopyCommandBuffer::WaitUntilCompleted() const
{
    if (mHandle->memoryCommandBuffer != nil)
    {
        [mHandle->memoryCommandBuffer waitUntilCompleted];
        mHandle->memoryCommandBuffer = nil;
    }
    
    if (mHandle->fileCommandBuffer != nil)
    {
        [mHandle->fileCommandBuffer waitUntilCompleted];
        mHandle->fileCommandBuffer = nil;
    }
    
    mHandle->stagingBufferOffset = 0;
    
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
        if (mHandle->memoryCommandBuffer == nil)
        {
            mHandle->AllocateMemoryCommandBuffer();
        }
        
        id<MTLBuffer> dstBuffer = buffer.GetHandle();
        id<MTLBlitCommandEncoder> blitCommandEncoder = [mHandle->memoryCommandBuffer blitCommandEncoder];
        [blitCommandEncoder setLabel:TO_NSSTRING("CopyCommandBuffer::Commit")];
        
        size_t srcOffset = mHandle->stagingBufferOffset;
        if (mHandle->CopyUploadData(data, size))
        {
            [blitCommandEncoder copyFromBuffer:mHandle->stagingBuffer sourceOffset:srcOffset toBuffer:dstBuffer destinationOffset:offset size:size];
        }
        else
        {
            id<MTLBuffer> srcBuffer = [mDevice->GetHandle() newBufferWithBytes:data length:size options:MTLResourceStorageModeShared];
            mHandle->tempBuffers.push_back((__bridge_retained void*)srcBuffer);
            
            [blitCommandEncoder copyFromBuffer:srcBuffer sourceOffset:0 toBuffer:dstBuffer destinationOffset:offset size:size];
        }
        [blitCommandEncoder endEncoding];
    }
    else
    {
        memcpy(OffsetPointer(bufferContents, offset), data, size);
    }
}

void CopyCommandBuffer::Commit(const Texture& texture, const void* data, size_t size) const
{
    if (mHandle->memoryCommandBuffer == nil)
    {
        mHandle->AllocateMemoryCommandBuffer();
    }
    
    id<MTLTexture> dstTexture = texture.GetHandle();
    id<MTLBlitCommandEncoder> blitCommandEncoder = [mHandle->memoryCommandBuffer blitCommandEncoder];
    [blitCommandEncoder setLabel:TO_NSSTRING("CopyCommandBuffer::Commit")];
    
    size_t sourceBytesPerRow = texture.GetDescriptor().size.width * Utils::GetTextureFormatSizeInBytes(texture.GetDescriptor().format);
    size_t sourceBytesPerImage = sourceBytesPerRow * texture.GetDescriptor().size.height;
    MTLSize sourceSize = MTLSizeMake(texture.GetDescriptor().size.width, texture.GetDescriptor().size.height, 1);
    
    size_t srcOffset = mHandle->stagingBufferOffset;
    if (mHandle->CopyUploadData(data, size))
    {
        [blitCommandEncoder copyFromBuffer:mHandle->stagingBuffer
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
        
        [blitCommandEncoder copyFromBuffer:srcBuffer
                              sourceOffset:0
                         sourceBytesPerRow:sourceBytesPerRow
                       sourceBytesPerImage:sourceBytesPerImage
                                sourceSize:sourceSize
                                 toTexture:dstTexture
                          destinationSlice:0
                          destinationLevel:0
                         destinationOrigin:MTLOriginMake(0, 0, 0)];
    }
    [blitCommandEncoder endEncoding];
}

#endif
