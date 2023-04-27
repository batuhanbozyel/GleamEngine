#pragma once
#include "GraphicsObject.h"
#include "TextureDescriptor.h"

namespace Gleam {

class Texture : public GraphicsObject
{
public:
    
    Texture(const TextureDescriptor& descriptor)
        : mDescriptor(descriptor), mMipMapLevels(descriptor.useMipMap ? CalculateMipLevels(descriptor.size) : 1)
    {
        
    }
    
    NativeGraphicsHandle GetView() const
    {
        return mView;
    }
    
    const TextureDescriptor& GetDescriptor() const
    {
        return mDescriptor;
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
    
    NativeGraphicsHandle mView = nullptr;
    
};

class Texture2D final : public Texture
{
public:

	Texture2D(const TextureDescriptor& descriptor);

	~Texture2D();

	void SetPixels(const TArray<uint8_t>& pixels) const;
	
};

class TextureCube final : public Texture
{
public:
    
    TextureCube(const TextureDescriptor& descriptor);
    
    ~TextureCube();
    
};

class RenderTexture final : public Texture
{
public:
    
    RenderTexture(const TextureDescriptor& descriptor, bool swapchainTarget = false);
    
    ~RenderTexture();

	NativeGraphicsHandle GetMSAAHandle() const
    {
        return mMultisampleHandle;
    }
    
    NativeGraphicsHandle GetMSAAView() const
    {
        return mMultisampleView;
    }
    
private:

	// multisample
	NativeGraphicsHandle mMultisampleMemory;
	NativeGraphicsHandle mMultisampleView;
	NativeGraphicsHandle mMultisampleHandle;

};

} // namespace Gleam
