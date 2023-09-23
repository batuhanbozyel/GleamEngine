#pragma once
#include "../Buffer.h"
#include "../Texture.h"

namespace Gleam {

class PoolAllocator final
{
public:
    
    Buffer CreateBuffer(const BufferDescriptor& descriptor);

	Texture CreateTexture(const TextureDescriptor& descriptor);

private:

};

} // namespace Gleam
