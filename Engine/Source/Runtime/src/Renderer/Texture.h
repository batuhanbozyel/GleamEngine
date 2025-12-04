#pragma once
#include "GraphicsObject.h"
#include "TextureDescriptor.h"

namespace Gleam {

class GraphicsDevice;

class Texture final : public ShaderResource
{
    friend class GraphicsDevice;
    
public:
    
    Texture() = default;
    
    Texture(const Texture& other) = default;
    
    Texture& operator=(const Texture& other) = default;
    
    Texture(const TextureDescriptor& descriptor)
        : mDescriptor(descriptor)
		, mMipMapLevels(descriptor.useMipMap ? CalculateMipLevels(descriptor.size) : 1)
    {
        
    }

	Texture(const TextureDescriptor& descriptor, NativeGraphicsHandle handle, RenderTargetView rtv)
        : ShaderResource(handle)
		, mView(rtv)
		, mDescriptor(descriptor)
		, mMipMapLevels(descriptor.useMipMap ? CalculateMipLevels(descriptor.size) : 1)
    {
        
    }
    
	RenderTargetView GetRenderTargetView() const
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

	static constexpr uint32_t CalculateMipLevels(const Size& size)
	{
		return static_cast<uint32_t>(Math::Floor(Math::Log2(Math::Max(size.width, size.height)))) + 1;
	}
    
private:
    
    uint32_t mMipMapLevels = 1;
	RenderTargetView mView = {};
    TextureDescriptor mDescriptor;
    
};

} // namespace Gleam
