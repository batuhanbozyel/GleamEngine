#pragma once
#include "GraphicsObject.h"
#include "TextureDescriptor.h"

namespace Gleam {

class Texture : public GraphicsObject
{
public:

	Texture();

    Texture(const TextureDescriptor& descriptor, bool allocate = true);
    
    Texture(const Texture& other) = default;
    
    Texture& operator=(const Texture& other) = default;
    
    void Dispose();
    
    void SetPixels(const void* pixels) const;
    
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

} // namespace Gleam
