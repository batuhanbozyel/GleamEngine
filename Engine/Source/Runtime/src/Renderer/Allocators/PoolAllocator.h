#pragma once
#include "../TextureDescriptor.h"

namespace Gleam {

class PoolAllocator final
{
public:

	Texture CreateTexture(const TextureDescriptor& descriptor);

private:

};

} // namespace Gleam
