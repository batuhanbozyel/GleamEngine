#include "RenderGraphResource.h"
#include "RenderGraphNode.h"

#include "../Buffer.h"
#include "../Texture.h"

using namespace Gleam;

NO_DISCARD constexpr BufferHandle::operator Buffer() const
{

}

NO_DISCARD constexpr TextureHandle::operator Texture() const
{

}