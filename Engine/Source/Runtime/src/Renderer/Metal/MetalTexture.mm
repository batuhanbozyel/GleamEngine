#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Texture.h"
#include "MetalUtils.h"

using namespace Gleam;

Texture2D::Texture2D(const TextureDescriptor& descriptor)
    : Texture(descriptor)
{
    MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:TextureFormatToMTLPixelFormat(descriptor.format) width:descriptor.size.width height:descriptor.size.height mipmapped:descriptor.useMipMap];
    textureDesc.usage = TextureUsageToMTLTextureUsage(descriptor.usage);
    textureDesc.mipmapLevelCount = mMipMapLevels;
    mHandle = [MetalDevice::GetHandle() newTextureWithDescriptor:textureDesc];
    
    id<MTLTexture> baseTexture = mHandle;
    mView = [baseTexture newTextureViewWithPixelFormat:baseTexture.pixelFormat
                                           textureType:MTLTextureType2DArray
                                                levels:NSMakeRange(0, mMipMapLevels)
                                                slices:NSMakeRange(0, 1)];
}

Texture2D::~Texture2D()
{
    mHandle = nil;
    mView = nil;
}

void Texture2D::SetPixels(const void* pixels) const
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

TextureCube::TextureCube(const TextureDescriptor& descriptor)
    : Texture(descriptor)
{
    float size = Math::Min(mDescriptor.size.width, mDescriptor.size.height);
    mDescriptor.size.width = size;
    mDescriptor.size.height = size;
    MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:TextureFormatToMTLPixelFormat(mDescriptor.format) size:size mipmapped:mDescriptor.useMipMap];
    textureDesc.usage = TextureUsageToMTLTextureUsage(descriptor.usage);
    textureDesc.mipmapLevelCount = mMipMapLevels;
    mHandle = [MetalDevice::GetHandle() newTextureWithDescriptor:textureDesc];
    
    id<MTLTexture> baseTexture = mHandle;
    mView = [baseTexture newTextureViewWithPixelFormat:baseTexture.pixelFormat
                                           textureType:MTLTextureTypeCubeArray
                                                levels:NSMakeRange(0, mMipMapLevels)
                                                slices:NSMakeRange(0, 1)];
}

TextureCube::~TextureCube()
{
    mHandle = nil;
    mView = nil;
}

RenderTexture::RenderTexture()
    : Texture(TextureDescriptor({.size = MetalDevice::GetSwapchain().GetSize(),
                                 .format = MetalDevice::GetSwapchain().GetFormat()}))
{
    
}

RenderTexture::RenderTexture(const TextureDescriptor& descriptor)
    : Texture(descriptor)
{
    MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:TextureFormatToMTLPixelFormat(descriptor.format) width:descriptor.size.width height:descriptor.size.height mipmapped:descriptor.useMipMap];
    textureDesc.mipmapLevelCount = mMipMapLevels;
    textureDesc.sampleCount = 1;
    textureDesc.usage = TextureUsageToMTLTextureUsage(descriptor.usage) | MTLTextureUsageRenderTarget;
    textureDesc.storageMode = MTLStorageModePrivate;
    mHandle = [MetalDevice::GetHandle() newTextureWithDescriptor:textureDesc];
    
    id<MTLTexture> baseTexture = mHandle;
    mView = [baseTexture newTextureViewWithPixelFormat:baseTexture.pixelFormat
                                           textureType:MTLTextureType2DArray
                                                levels:NSMakeRange(0, mMipMapLevels)
                                                slices:NSMakeRange(0, 1)];
    
    if (descriptor.sampleCount > 1)
    {
        MTLTextureDescriptor* msaaTextureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:textureDesc.pixelFormat width:textureDesc.width height:textureDesc.height mipmapped:false];
        msaaTextureDesc.textureType = MTLTextureType2DMultisample;
        msaaTextureDesc.mipmapLevelCount = 1;
        msaaTextureDesc.sampleCount = descriptor.sampleCount;
        msaaTextureDesc.usage = MTLTextureUsageRenderTarget;
        msaaTextureDesc.storageMode = MTLStorageModePrivate; // TODO: Switch to memoryless msaa render targets when Tile shading is supported
        mMultisampleHandle = [MetalDevice::GetHandle() newTextureWithDescriptor:msaaTextureDesc];
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

#endif
