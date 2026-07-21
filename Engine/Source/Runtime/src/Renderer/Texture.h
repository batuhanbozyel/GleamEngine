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
		return mSliceUnorderedAccessViews[GetSubresourceIndex(mip, slice)];
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
		return GetSlice(subresourceIndex, mMipMapLevels);
	}

	uint32_t GetMip(uint32_t subresourceIndex) const
	{
		return GetMip(subresourceIndex, mMipMapLevels);
	}

	uint32_t GetSubresourceIndex(uint32_t mip, uint32_t slice) const
	{
		return slice * mMipMapLevels + mip;
	}

	static constexpr uint32_t CalculateMipLevels(const Size& size)
	{
		uint32_t extent = static_cast<uint32_t>(Math::Max(size.width, size.height));
		uint32_t levels = 1;
		while (extent > 1)
		{
			extent >>= 1;
			++levels;
		}
		return levels;
	}

	static constexpr uint32_t GetSlice(uint32_t subresourceIndex, uint32_t mipMapLevels)
	{
		return subresourceIndex / mipMapLevels;
	}

	static constexpr uint32_t GetMip(uint32_t subresourceIndex, uint32_t mipMapLevels)
	{
		return subresourceIndex % mipMapLevels;
	}

private:

	uint32_t mMipMapLevels = 1;
	RenderTargetView mView = {};
	TextureDescriptor mDescriptor;

	TArray<RenderTargetView> mSliceViews;
	TArray<ShaderResourceIndex> mSliceUnorderedAccessViews;
};

} // namespace Gleam
