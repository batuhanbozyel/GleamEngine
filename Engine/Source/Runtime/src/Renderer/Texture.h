#pragma once
#include "GraphicsObject.h"
#include "TextureDescriptor.h"

namespace Gleam {

class Texture
{
public:
    
    Texture(const TextureDescriptor& descriptor)
        : mDescriptor(descriptor), mMipMapLevels(descriptor.useMipMap ? CalculateMipLevels(descriptor.size) : 1)
    {
        
    }
    
    const Size& GetDescriptor() const
    {
        return mDescriptor.size;
    }
    
    uint32_t GetMipMapLevels() const
    {
        return mMipMapLevels;
    }
    
protected:

    static constexpr uint32_t CalculateMipLevels(const Size& size)
    {
        return static_cast<uint32_t>(Math::Floor(Math::Log2(Math::Max(size.width, size.height)))) + 1;
    }

    uint32_t mMipMapLevels;
    TextureDescriptor mDescriptor;
    
};

class Texture2D final : public Texture, public GraphicsObject
{
public:

	Texture2D(const TextureDescriptor& descriptor);

	~Texture2D();

	void SetPixels(const TArray<uint8_t>& pixels) const;

private:

    NativeGraphicsHandle mMemory = nullptr;
    NativeGraphicsHandle mImageView = nullptr;
	
};

class RenderTexture final : public Texture, public MutableGraphicsObject
{
public:
    
    RenderTexture(const RenderTextureDescriptor& descriptor);
    
    ~RenderTexture();
    
    uint32_t GetSampleCount() const
    {
        return mSampleCount;
    }

	NativeGraphicsHandle GetImageView() const;
    
private:
    
    uint32_t mSampleCount = 1;
    
    TArray<NativeGraphicsHandle> mImageViews;

	// multisample
	NativeGraphicsHandle mMultisampleMemory;
	NativeGraphicsHandle mMultisampleImageView;
	NativeGraphicsHandle mMultisampleTexture;

};

} // namespace Gleam
