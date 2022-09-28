#pragma once
#include "TextureFormat.h"
#include "GraphicsObject.h"

namespace Gleam {

class Texture : public GraphicsObject
{
public:
    
    Texture(const Size& size, TextureFormat format, bool useMipMap)
        : mSize(size), mFormat(format), mMipMapCount(CalculateMipLevels(size))
    {
        
    }
    
    virtual ~Texture();

    void SetPixels(const TArray<uint8_t>& pixels) const;
    
    TextureFormat GetFormat() const
    {
        return mFormat;
    }
    
    const Size& GetSize() const
    {
        return mSize;
    }
    
    bool MipMapped() const
    {
        return mMipMapCount > 0;
    }
    
protected:

    static constexpr uint32_t CalculateMipLevels(const Size& size)
    {
        return Math::Floor(Math::Log2(Math::Max(size.width, size.height))) + 1;
    }

    TextureFormat mFormat = TextureFormat::R8G8B8A8_SRGB;
    FilterMode mFilterMode = FilterMode::Point;
    WrapMode mWrapMode = WrapMode::Clamp;
    Size mSize = Size::zero;
    uint32_t mMipMapCount = 0;

    uint32_t mBufferSize = 0;
    NativeGraphicsHandle mMemory;
    NativeGraphicsHandle mImageView;
    
};
    
class Texture2D final : public Texture
{
public:

	Texture2D(const Size& size, TextureFormat format, bool useMipMap = false);
	
};

class RenderTexture final : public Texture
{
public:
    
    RenderTexture(const Size& size, TextureFormat format, uint32_t sampleCount = 1, bool useMipMap = false);
    
    // RenderTexture can only be used in a single RenderTarget
    // to avoid accessing the same RenderTexture, we are locking the resource
    void Lock()
    {
        mLocked = true;
    }
    
    void Release()
    {
        mLocked = false;
    }
    
    bool Available() const
    {
        return !mLocked;
    }
    
    uint32_t GetSampleCount() const
    {
        return mSampleCount;
    }
    
private:
    
    uint32_t mSampleCount = 1;
    
    std::atomic<bool> mLocked = false;

};

} // namespace Gleam
