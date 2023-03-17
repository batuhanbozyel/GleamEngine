#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Texture.h"
#include "MetalUtils.h"

using namespace Gleam;

Texture2D::Texture2D(const TextureDescriptor& descriptor)
    : Texture(descriptor)
{
    MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:TextureFormatToMTLPixelFormat(descriptor.format) width:descriptor.size.width height:descriptor.size.height mipmapped:descriptor.useMipMap];
    textureDesc.mipmapLevelCount = mMipMapLevels;
    mHandle = [MetalDevice::GetHandle() newTextureWithDescriptor:textureDesc];
    mImageView = mHandle;
}

Texture2D::~Texture2D()
{
    mHandle = nil;
    mImageView = nil;
}

void Texture2D::SetPixels(const TArray<uint8_t>& pixels) const
{
    MTLRegion region = MTLRegionMake2D(0, 0, mDescriptor.size.width, mDescriptor.size.height);
    [id<MTLTexture>(mHandle) replaceRegion:region mipmapLevel:0 withBytes:pixels.data() bytesPerRow:mDescriptor.size.width * Utils::GetTextureFormatSize(mDescriptor.format)];
    
    id<MTLCommandBuffer> commandBuffer = [MetalDevice::GetCommandPool() commandBuffer];
    id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
    
    [commandEncoder generateMipmapsForTexture:id<MTLTexture>(mHandle)];
    [commandEncoder endEncoding];
    
    [commandBuffer commit];
}

RenderTexture::RenderTexture(const RenderTextureDescriptor& descriptor)
    : Texture(descriptor), mSampleCount(descriptor.sampleCount)
{
    for (uint32_t i = 0; i < MetalDevice::GetSwapchain().GetMaxFramesInFlight(); i++)
    {
        MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:TextureFormatToMTLPixelFormat(descriptor.format) width:descriptor.size.width height:descriptor.size.height mipmapped:descriptor.useMipMap];
        textureDesc.mipmapLevelCount = mMipMapLevels;
        textureDesc.sampleCount = 1;
        textureDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
        textureDesc.storageMode = MTLStorageModePrivate;
        mHandles[i] = [MetalDevice::GetHandle() newTextureWithDescriptor:textureDesc];
        mImageViews[i] = mHandles[i];
    }

    if (mSampleCount > 1)
    {
        MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:TextureFormatToMTLPixelFormat(descriptor.format) width:descriptor.size.width height:descriptor.size.height mipmapped:false];
        textureDesc.textureType = MTLTextureType2DMultisample;
        textureDesc.mipmapLevelCount = 1;
        textureDesc.sampleCount = mSampleCount;
        textureDesc.usage = MTLTextureUsageRenderTarget;
        textureDesc.storageMode = MTLStorageModeMemoryless;
        mMultisampleTexture = [MetalDevice::GetHandle() newTextureWithDescriptor:textureDesc];
    }
}

RenderTexture::~RenderTexture()
{
    for (uint32_t i = 0; i < MetalDevice::GetSwapchain().GetMaxFramesInFlight(); i++)
    {
        mHandles[i] = nil;
        mImageViews[i] = nil;
    }
    mMultisampleTexture = nil;
}

NativeGraphicsHandle RenderTexture::GetImageView() const
{
    if (mMultisampleTexture != nil)
        return mMultisampleTexture;

    return mImageViews[MetalDevice::GetSwapchain().GetFrameIndex()];
}

#endif
