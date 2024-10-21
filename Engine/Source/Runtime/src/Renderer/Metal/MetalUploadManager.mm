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
    
    mHandle->stagingBuffer = [mDevice->GetHandle() newBufferWithLength:UploadHeapSize options:MTLResourceStorageModeShared];
    mHandle->stagingBufferPtr = [mHandle->stagingBuffer contents];
}

UploadManager::~UploadManager()
{
    mHandle->stagingBuffer = nil;
    mHandle->fileCommandPool = nil;
    mHandle->fileCommandBuffer = nil;
    mHandle->memoryCommandBuffer = nil;
}

void UploadManager::Commit()
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
    mHandle->stagingBufferOffset = 0;
}

void UploadManager::CommitUpload(const Buffer& buffer, const void* data, size_t size, size_t offset)
{
    auto bufferContents = buffer.GetContents();
    if (bufferContents == nullptr)
    {
        void* srcBufferPtr = OffsetPointer(mHandle->stagingBufferPtr, mHandle->stagingBufferOffset);
        memcpy(srcBufferPtr, data, size);
        
        if (mHandle->memoryCommandBuffer == nil)
        {
            mHandle->memoryCommandBuffer = mHandle->CreateMemoryCommandBuffer(mDevice);
        }
        
        id<MTLBuffer> dstBuffer = buffer.GetHandle();
        id<MTLBlitCommandEncoder> blitCommandEncoder = [mHandle->memoryCommandBuffer blitCommandEncoder];
        [blitCommandEncoder setLabel:TO_NSSTRING("UploadManager::CommitUpload")];
        [blitCommandEncoder copyFromBuffer:mHandle->stagingBuffer sourceOffset:mHandle->stagingBufferOffset toBuffer:dstBuffer destinationOffset:offset size:size];
        [blitCommandEncoder endEncoding];
        
        mHandle->stagingBufferOffset += size;
    }
    else
    {
        memcpy(OffsetPointer(bufferContents, offset), data, size);
    }
}

void UploadManager::CommitUpload(const Texture& texture, const void* data, size_t size)
{
    void* srcBufferPtr = OffsetPointer(mHandle->stagingBufferPtr, mHandle->stagingBufferOffset);
    memcpy(srcBufferPtr, data, size);
    
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
    
    [blitCommandEncoder copyFromBuffer:mHandle->stagingBuffer
                          sourceOffset:mHandle->stagingBufferOffset
                     sourceBytesPerRow:sourceBytesPerRow
                   sourceBytesPerImage:sourceBytesPerImage
                            sourceSize:sourceSize
                             toTexture:dstTexture
                      destinationSlice:0
                      destinationLevel:0
                     destinationOrigin:MTLOriginMake(0, 0, 0)];
    
    [blitCommandEncoder endEncoding];
    
    mHandle->stagingBufferOffset += size;
}

#endif
