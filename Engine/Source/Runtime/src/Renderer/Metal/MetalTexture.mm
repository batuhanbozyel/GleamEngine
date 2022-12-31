#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Texture.h"
#include "MetalUtils.h"

using namespace Gleam;

Texture2D::Texture2D(const Size& size, TextureFormat format, bool useMipMap)
    : Texture(size, format, useMipMap)
{
    MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:TextureFormatToMTLPixelFormat(format) width:mSize.width height:mSize.height mipmapped:useMipMap];
    textureDesc.mipmapLevelCount = mMipMapCount;
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
    MTLRegion region = MTLRegionMake2D(0, 0, mSize.width, mSize.height);
    [id<MTLTexture>(mHandle) replaceRegion:region mipmapLevel:0 withBytes:pixels.data() bytesPerRow:mSize.width * GetTextureFormatSize(mFormat)];
    
    id<MTLCommandBuffer> commandBuffer = [MetalDevice::GetCommandPool() commandBuffer];
    id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
    
    [commandEncoder generateMipmapsForTexture:id<MTLTexture>(mHandle)];
    [commandEncoder endEncoding];
    
    [commandBuffer commit];
}

RenderTexture::RenderTexture(const Size& size, TextureFormat format, uint32_t sampleCount, bool useMipMap)
    : Texture(size, format, useMipMap), mSampleCount(sampleCount)
{
    for (uint32_t i = 0; i < MetalDevice::GetSwapchain().GetMaxFramesInFlight(); i++)
    {
        MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:TextureFormatToMTLPixelFormat(format) width:mSize.width height:mSize.height mipmapped:useMipMap];
        textureDesc.mipmapLevelCount = mMipMapCount;
        textureDesc.sampleCount = 1;
        textureDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
        textureDesc.storageMode = MTLStorageModePrivate;
        mHandles[i] = [MetalDevice::GetHandle() newTextureWithDescriptor:textureDesc];
        mImageViews[i] = mHandles[i];
    }

    if (mSampleCount > 1)
    {
        MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:TextureFormatToMTLPixelFormat(format) width:mSize.width height:mSize.height mipmapped:false];
        textureDesc.textureType = MTLTextureType2DMultisample;
        textureDesc.mipmapLevelCount = mMipMapCount;
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
