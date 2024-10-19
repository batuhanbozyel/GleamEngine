#pragma once
#include "Texture.h"
#include "Assets/Asset.h"

namespace Gleam {

class Texture2D final : public Asset
{
public:
    
    Texture2D(const Texture2DDescriptor& descriptor);

	virtual void Release() override;

	ShaderResourceIndex GetResourceView() const;
    
private:
    
    Texture mTexture;

};

} // namespace Gleam
