#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Texture.h"
#include "MetalUtils.h"

using namespace Gleam;

Texture::Texture()
    : mDescriptor({
        .type = TextureType::RenderTexture,
        .size = MetalDevice::GetSwapchain().GetSize(),
        .format = MetalDevice::GetSwapchain().GetFormat()}),
    mMipMapLevels(1)
{
    
}

Texture::Texture(const TextureDescriptor& descriptor)
    : mDescriptor(descriptor), mMipMapLevels(descriptor.useMipMap ? CalculateMipLevels(descriptor.size) : 1)
{
    MTLTextureDescriptor* textureDesc;
    if (mDescriptor.type == TextureType::TextureCube)
    {
        float size = Math::Min(mDescriptor.size.width, mDescriptor.size.height);
        mDescriptor.size.width = size;
        mDescriptor.size.height = size;
        textureDesc = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:TextureFormatToMTLPixelFormat(mDescriptor.format) size:size mipmapped:mDescriptor.useMipMap];
    }
    else
    {
        textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:TextureFormatToMTLPixelFormat(mDescriptor.format) width:mDescriptor.size.width height:mDescriptor.size.height mipmapped:mDescriptor.useMipMap];
    }
    textureDesc.mipmapLevelCount = mMipMapLevels;
    textureDesc.sampleCount = 1;
    textureDesc.usage = mDescriptor.type == TextureType::RenderTexture ? TextureUsageToMTLTextureUsage(mDescriptor.usage) | MTLTextureUsageRenderTarget : TextureUsageToMTLTextureUsage(mDescriptor.usage);
    textureDesc.storageMode = MTLStorageModePrivate;
    mHandle = [MetalDevice::GetHandle() newTextureWithDescriptor:textureDesc];
    
    id<MTLTexture> baseTexture = mHandle;
    mView = [baseTexture newTextureViewWithPixelFormat:baseTexture.pixelFormat
                                           textureType:mDescriptor.type == TextureType::TextureCube ? MTLTextureTypeCubeArray : MTLTextureType2DArray
                                                levels:NSMakeRange(0, mMipMapLevels)
                                                slices:NSMakeRange(0, 1)];
    
    if (mDescriptor.sampleCount > 1)
    {
        MTLTextureDescriptor* msaaTextureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:baseTexture.pixelFormat width:mDescriptor.size.width height:mDescriptor.size.height mipmapped:false];
        msaaTextureDesc.textureType = MTLTextureType2DMultisample;
        msaaTextureDesc.mipmapLevelCount = 1;
        msaaTextureDesc.sampleCount = mDescriptor.sampleCount;
        msaaTextureDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderWrite;
        msaaTextureDesc.storageMode = MTLStorageModePrivate; // TODO: Switch to memoryless msaa render targets when Tile shading is supported
        mMultisampleHandle = [MetalDevice::GetHandle() newTextureWithDescriptor:msaaTextureDesc];
        mMultisampleView = mMultisampleHandle;
    }
}

void Texture::SetPixels(const void* pixels) const
{
    MTLRegion region = MTLRegionMake2D(0, 0, mDescriptor.size.width, mDescriptor.size.height);
    [id<MTLTexture>(mHandle) replaceRegion:region mipmapLevel:0 withBytes:pixels bytesPerRow:mDescriptor.size.width * Utils::GetTextureFormatSize(mDescriptor.format)];
    
    if (mMipMapLevels > 1)
    {
        id<MTLCommandBuffer> commandBuffer = [MetalDevice::GetCommandPool() commandBuffer];
        id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
        
        [commandEncoder generateMipmapsForTexture:id<MTLTexture>(mHandle)];
        [commandEncoder endEncoding];
        [commandBuffer commit];
    }
}

void Texture::Dispose()
{
    if (mHandle != nil)
    {
        mHandle = nil;
        mView = nil;
    }
    
    if (mMultisampleHandle != nil)
    {
        mMultisampleHandle = nil;
        mMultisampleView = nil;
    }
}

#endif
