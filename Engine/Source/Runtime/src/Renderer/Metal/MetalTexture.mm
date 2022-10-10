#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Texture.h"
#include "MetalUtils.h"

using namespace Gleam;

Texture::~Texture()
{
    mHandle = nil;
    mImageView = nil;
}

Texture2D::Texture2D(const Size& size, TextureFormat format, bool useMipMap)
    : Texture(size, format, useMipMap)
{
    MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:TextureFormatToMTLPixelFormat(format) width:mSize.width height:mSize.height mipmapped:useMipMap];
    textureDesc.mipmapLevelCount = mMipMapCount;
    mHandle = [MetalDevice newTextureWithDescriptor:textureDesc];
}

RenderTexture::RenderTexture(const Size& size, TextureFormat format, uint32_t sampleCount, bool useMipMap)
    : Texture(size, format, useMipMap), mSampleCount(sampleCount)
{
    MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:TextureFormatToMTLPixelFormat(format) width:mSize.width height:mSize.height mipmapped:useMipMap];
    textureDesc.mipmapLevelCount = mMipMapCount;
    textureDesc.sampleCount = mSampleCount;
    textureDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    textureDesc.storageMode = MTLStorageModePrivate;
    mHandle = [MetalDevice newTextureWithDescriptor:textureDesc];
}

void Texture::SetPixels(const TArray<uint8_t>& pixels) const
{
    MTLRegion region = MTLRegionMake2D(0, 0, mSize.width, mSize.height);
    [id<MTLTexture>(mHandle) replaceRegion:region mipmapLevel:0 withBytes:pixels.data() bytesPerRow:mSize.width * GetTextureFormatSize(mFormat)];
    
    id<MTLCommandBuffer> commandBuffer = [RendererContext::GetTransferCommandPool(0) commandBuffer];
    id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
    
    [commandEncoder generateMipmapsForTexture:id<MTLTexture>(mHandle)];
    [commandEncoder endEncoding];
    
    [commandBuffer commit];
}

#endif
