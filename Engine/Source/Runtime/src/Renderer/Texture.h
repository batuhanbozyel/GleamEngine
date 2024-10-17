#pragma once
#include "GraphicsObject.h"
#include "TextureDescriptor.h"
#include "Assets/Asset.h"

namespace Gleam {

class GraphicsDevice;

class Texture final : public ShaderResource, public Asset
{
    friend class GraphicsDevice;
    
public:
    
    Texture() = default;
    
    Texture(const Texture& other) = default;
    
    Texture& operator=(const Texture& other) = default;
    
    Texture(const TextureDescriptor& descriptor)
        : mDescriptor(descriptor), mMipMapLevels(descriptor.useMipMap ? CalculateMipLevels(descriptor.size) : 1)
    {
        
    }
    
	NativeGraphicsResourceView GetView() const
    {
        return mView;
    }

	NativeGraphicsHandle GetMSAAHandle() const
	{
		return mMultisampleHandle;
	}

	NativeGraphicsResourceView GetMSAAView() const
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
    
private:
    
    uint32_t mMipMapLevels = 1;
    TextureDescriptor mDescriptor;
	NativeGraphicsResourceView mView = {};

	// multisample
	NativeGraphicsHandle mMultisampleHandle = nullptr;
	NativeGraphicsResourceView mMultisampleView = {};
    
};

} // namespace Gleam
