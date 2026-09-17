#pragma once
#include "Texture.h"
#include "Assets/Asset.h"

namespace Gleam {

class Texture2D final : public Asset
{
public:

    Texture2D(const AssetReference& reference, const AssetHeader& header, const Texture2DDescriptor& descriptor);

    Texture2D(const TextureDescriptor& descriptor, const void* pixels, size_t size);

	~Texture2D();

	ShaderResourceIndex GetResourceView() const;

private:

    Texture mTexture;

};

} // namespace Gleam
