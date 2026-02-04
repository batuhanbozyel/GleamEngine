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
		uint32_t numSlices = mMipMapLevels * (descriptor.dimension == TextureDimension::TextureCube ? 6 * descriptor.depth : descriptor.depth);
		if (numSlices > 1)
		{
			mSliceViews.resize(numSlices);

			if (mDescriptor.usage & TextureUsage_Storage && Utils::IsColorFormat(mDescriptor.format))
			{
				mSliceUnorderedAccessViews.resize(numSlices);
			}
		}
    }

	Texture(const TextureDescriptor& descriptor, NativeGraphicsHandle handle, RenderTargetView rtv)
        : ShaderResource(handle)
		, mView(rtv)
		, mDescriptor(descriptor)
		, mMipMapLevels(descriptor.useMipMap ? CalculateMipLevels(descriptor.size) : 1)
    {
		uint32_t numSlices = mMipMapLevels * (descriptor.dimension == TextureDimension::TextureCube ? 6 * descriptor.depth : descriptor.depth);
		if (numSlices > 1)
		{
			mSliceViews.resize(numSlices);

			if (mDescriptor.usage & TextureUsage_Storage && Utils::IsColorFormat(mDescriptor.format))
			{
				mSliceUnorderedAccessViews.resize(numSlices);
			}
		}
    }
    
	RenderTargetView GetRenderTargetView() const
    {
        return mView;
    }

	RenderTargetView GetRenderTargetView(uint32_t mip) const
	{
		return mSliceViews[mip];
	}

	RenderTargetView GetRenderTargetView(uint32_t mip, uint32_t slice) const
	{
		return mSliceViews[slice * mMipMapLevels + mip];
	}

	UnorderedAccessIndex GetUnorderedAccessView() const
	{
		return mResourceView;
	}

	UnorderedAccessIndex GetUnorderedAccessView(uint32_t mip) const
	{
		return mSliceUnorderedAccessViews[mip];
	}

	UnorderedAccessIndex GetUnorderedAccessView(uint32_t mip, uint32_t slice) const
	{
		return mSliceUnorderedAccessViews[slice * mMipMapLevels + mip];
	}
    
    const TextureDescriptor& GetDescriptor() const
    {
        return mDescriptor;
    }
    
    uint32_t GetMipMapLevels() const
    {
        return mMipMapLevels;
    }

	uint32_t GetSlice(uint32_t subresourceIndex) const
	{
		return subresourceIndex / mMipMapLevels;
	}

	uint32_t GetMip(uint32_t subresourceIndex) const
	{
		return subresourceIndex % mMipMapLevels;
	}

	uint32_t GetSubresourceIndex(uint32_t mip, uint32_t slice) const
	{
		return slice * mMipMapLevels + mip;
	}

	static constexpr uint32_t CalculateMipLevels(const Size& size)
	{
		return static_cast<uint32_t>(Math::Floor(Math::Log2(Math::Max(size.width, size.height)))) + 1;
	}
    
private:
    
    uint32_t mMipMapLevels = 1;
	RenderTargetView mView = {};
    TextureDescriptor mDescriptor;

	TArray<RenderTargetView> mSliceViews;
	TArray<ShaderResourceIndex> mSliceUnorderedAccessViews;
};

} // namespace Gleam
