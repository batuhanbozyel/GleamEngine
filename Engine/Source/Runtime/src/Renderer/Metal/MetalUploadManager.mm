#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/UploadManager.h"
#include "MetalDevice.h"
#include "MetalUtils.h"

using namespace Gleam;

struct UploadManager::Impl
{
    id<MTLIOCommandQueue> fileCommandPool{ nil };
    id<MTLIOCommandBuffer> fileCommandBuffer{ nil };
    id<MTLCommandBuffer> memoryCommandBuffer{ nil };
    
    id<MTLBuffer> stagingBuffer{ nil };
    void* stagingBufferPtr = nullptr;
    size_t stagingBufferOffset = 0;
    
    TArray<id<MTLBuffer>> tempBuffers;
    
    id<MTLCommandBuffer> CreateMemoryCommandBuffer(GraphicsDevice* device)
    {
#ifdef GDEBUG
        MTLCommandBufferDescriptor* descriptor = [MTLCommandBufferDescriptor new];
        descriptor.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
        return [static_cast<MetalDevice*>(device)->GetCommandPool() commandBufferWithDescriptor:descriptor];
#else
        return [static_cast<MetalDevice*>(device)->GetCommandPool() commandBuffer];
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

UploadManager::UploadManager(GraphicsDevice* device)
    : mHandle(CreateScope<Impl>())
    , mDevice(device)
{
    MTLIOCommandQueueDescriptor* ioQueueDescriptor = [MTLIOCommandQueueDescriptor new];
    ioQueueDescriptor.type = MTLIOCommandQueueTypeConcurrent;
    ioQueueDescriptor.priority = MTLIOPriorityNormal;
    ioQueueDescriptor.maxCommandsInFlight = 0;
    
    __autoreleasing NSError* error = nil;
    mHandle->fileCommandPool = [mDevice->GetHandle() newIOCommandQueueWithDescriptor:ioQueueDescriptor error:&error];
    GLEAM_ASSERT(mHandle->fileCommandPool, "Metal: UploadManager command pool creation failed.");
    
    mHandle->stagingBuffer = [mDevice->GetHandle() newBufferWithLength:UploadHeapSize * mDevice->GetFramesInFlight() options:MTLResourceStorageModeShared];
    mHandle->stagingBufferPtr = [mHandle->stagingBuffer contents];
    mHandle->tempBuffers.resize(mDevice->GetFramesInFlight());
}

UploadManager::~UploadManager()
{
    mHandle->tempBuffers.clear();
    mHandle->stagingBuffer = nil;
    mHandle->fileCommandPool = nil;
    mHandle->fileCommandBuffer = nil;
    mHandle->memoryCommandBuffer = nil;
}

void UploadManager::Commit() const
{
    if (mHandle->fileCommandBuffer != nil)
    {
        [mHandle->fileCommandBuffer commit];
        mHandle->fileCommandBuffer = nil;
    }
    
    if (mHandle->memoryCommandBuffer != nil)
    {
        [mHandle->memoryCommandBuffer commit];
        mHandle->memoryCommandBuffer = nil;
    }
}

void UploadManager::Flush() const
{
    mHandle->stagingBufferOffset = 0;
    mHandle->tempBuffers.clear();
}

void UploadManager::CommitUpload(const Buffer& buffer, const void* data, size_t size, size_t offset) const
{
    auto bufferContents = buffer.GetContents();
    if (bufferContents == nullptr)
    {
        if (mHandle->memoryCommandBuffer == nil)
        {
            mHandle->memoryCommandBuffer = mHandle->CreateMemoryCommandBuffer(mDevice);
        }
        
        id<MTLBuffer> dstBuffer = buffer.GetHandle();
        id<MTLBlitCommandEncoder> blitCommandEncoder = [mHandle->memoryCommandBuffer blitCommandEncoder];
        [blitCommandEncoder setLabel:TO_NSSTRING("UploadManager::CommitUpload")];
        
        size_t srcOffset = mHandle->stagingBufferOffset;
        if (mHandle->CopyUploadData(data, size))
        {
            [blitCommandEncoder copyFromBuffer:mHandle->stagingBuffer sourceOffset:srcOffset toBuffer:dstBuffer destinationOffset:offset size:size];
        }
        else
        {
            id<MTLBuffer> srcBuffer = [mDevice->GetHandle() newBufferWithBytes:data length:size options:MTLResourceStorageModeShared];
            mHandle->tempBuffers.push_back(srcBuffer);
            
            [blitCommandEncoder copyFromBuffer:srcBuffer sourceOffset:0 toBuffer:dstBuffer destinationOffset:offset size:size];
        }
        [blitCommandEncoder endEncoding];
    }
    else
    {
        memcpy(OffsetPointer(bufferContents, offset), data, size);
    }
}

void UploadManager::CommitUpload(const Texture& texture, const void* data, size_t size) const
{
    if (mHandle->memoryCommandBuffer == nil)
    {
        mHandle->memoryCommandBuffer = mHandle->CreateMemoryCommandBuffer(mDevice);
    }
    
    id<MTLTexture> dstTexture = texture.GetHandle();
    id<MTLBlitCommandEncoder> blitCommandEncoder = [mHandle->memoryCommandBuffer blitCommandEncoder];
    [blitCommandEncoder setLabel:TO_NSSTRING("UploadManager::CommitUpload")];
    
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
        mHandle->tempBuffers.push_back(srcBuffer);
        
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
