#pragma once
#include "SamplerState.h"
#include "GraphicsObject.h"
#include "TextureDescriptor.h"

namespace Gleam {

class Texture : public GraphicsObject
{
public:

	Texture() = default;

    Texture(const TextureDescriptor& descriptor)
        : mDescriptor(descriptor), mMipMapLevels(descriptor.useMipMap ? CalculateMipLevels(descriptor.size) : 1)
    {
        
    }
    
    NativeGraphicsHandle GetView() const
    {
        return mView;
    }

	NativeGraphicsHandle GetMSAAHandle() const
	{
		return mMultisampleHandle;
	}

	NativeGraphicsHandle GetMSAAView() const
	{
		return mMultisampleView;
	}
    
    const TextureDescriptor& GetDescriptor() const
    {
        return mDescriptor;
    }
    
    uint32_t GetMipMapLevels() const
    {
        return mMipMapLevels;
    }

	static constexpr uint32_t CalculateMipLevels(const Size& size)
	{
		return static_cast<uint32_t>(Math::Floor(Math::Log2(Math::Max(size.width, size.height)))) + 1;
	}
    
protected:
    
    uint32_t mMipMapLevels;
    TextureDescriptor mDescriptor;
	NativeGraphicsHandle mHeap = nullptr;
    NativeGraphicsHandle mView = nullptr;

	// multisample
	NativeGraphicsHandle mMultisampleHeap = nullptr;
	NativeGraphicsHandle mMultisampleView = nullptr;
	NativeGraphicsHandle mMultisampleHandle = nullptr;
    
};

class Texture2D final : public Texture
{
public:

	GLEAM_NONCOPYABLE(Texture2D);

	Texture2D(const TextureDescriptor& descriptor);

	~Texture2D();
    
    void SetPixels(const void* pixels) const;
	
};

class TextureCube final : public Texture
{
public:

	GLEAM_NONCOPYABLE(TextureCube);
    
    TextureCube(const TextureDescriptor& descriptor);
    
    ~TextureCube();
    
};

class RenderTexture final : public Texture
{
public:

	GLEAM_NONCOPYABLE(RenderTexture);
    
    // Swapchain target
    RenderTexture();
    
    RenderTexture(const TextureDescriptor& descriptor);
    
    ~RenderTexture();

};

} // namespace Gleam
