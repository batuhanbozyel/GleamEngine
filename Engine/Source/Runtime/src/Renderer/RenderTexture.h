#pragma once
#include "Texture.h"
#include "TextureDescriptor.h"
#include "Assets/Asset.h"

namespace Gleam {

class RenderTexture final : public Asset
{
public:

	RenderTexture(const AssetReference& reference, const AssetHeader& header, const RenderTextureDescriptor& descriptor);

	~RenderTexture();

	void Resize(const Size& size);

	const RenderTextureDescriptor& GetDescriptor() const
	{
		return mDescriptor;
	}

	const Texture& GetTexture() const
	{
		return mTexture;
	}

private:

	void CreateTexture();

	RenderTextureDescriptor mDescriptor;

	Texture mTexture;

};

} // namespace Gleam
