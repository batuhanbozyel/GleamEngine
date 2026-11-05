#pragma once
#include "Texture.h"
#include "TextureDescriptor.h"
#include "RenderSurface.h"
#include "Assets/Asset.h"

namespace Gleam {

class RenderTexture final : public Asset, public RenderSurface
{
public:

	RenderTexture(const AssetReference& reference, const AssetHeader& header, const RenderTextureDescriptor& descriptor);

	~RenderTexture();

	virtual void Resize(GraphicsDevice* device, const Size& size) override;

	virtual TextureFormat GetFormat() const override
	{
		return mTexture.GetDescriptor().format;
	}

	virtual const Size& GetSize() const override
	{
		return mTexture.GetDescriptor().size;
	}

	const Texture& GetTexture() const
	{
		return mTexture;
	}

	const Color& GetClearColor() const
	{
		return mClearColor;
	}

	bool ShouldClear() const
	{
		return mClearBuffer;
	}

private:

	Texture mTexture;

	Color mClearColor = Color::clear;
	bool mClearBuffer = false;

};

} // namespace Gleam
