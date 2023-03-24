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
    mView = mHandle;
}

Texture2D::~Texture2D()
{
    mHandle = nil;
    mView = nil;
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

RenderTexture::RenderTexture(const TextureDescriptor& descriptor)
    : Texture(descriptor)
{
    MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:TextureFormatToMTLPixelFormat(descriptor.format) width:descriptor.size.width height:descriptor.size.height mipmapped:descriptor.useMipMap];
    textureDesc.mipmapLevelCount = mMipMapLevels;
    textureDesc.sampleCount = 1;
    textureDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    textureDesc.storageMode = MTLStorageModePrivate;
    mHandle = [MetalDevice::GetHandle() newTextureWithDescriptor:textureDesc];
    mView = mHandle;

    if (descriptor.sampleCount > 1)
    {
        MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:TextureFormatToMTLPixelFormat(descriptor.format) width:descriptor.size.width height:descriptor.size.height mipmapped:false];
        textureDesc.textureType = MTLTextureType2DMultisample;
        textureDesc.mipmapLevelCount = 1;
        textureDesc.sampleCount = descriptor.sampleCount;
        textureDesc.usage = MTLTextureUsageRenderTarget;
        textureDesc.storageMode = MTLStorageModeMemoryless;
        mMultisampleHandle = [MetalDevice::GetHandle() newTextureWithDescriptor:textureDesc];
        mMultisampleView = mMultisampleHandle;
    }
}

RenderTexture::~RenderTexture()
{
    mView = nil;
    mHandle = nil;
    mMultisampleView = nil;
    mMultisampleHandle = nil;
}

NativeGraphicsHandle RenderTexture::GetRenderSurface() const
{
    if (mMultisampleHandle != nil)
        return mMultisampleHandle;

    return mHandle;
}

#endif
