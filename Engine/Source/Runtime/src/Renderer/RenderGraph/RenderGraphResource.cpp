#include "RenderGraphResource.h"
#include "RenderGraphNode.h"

#include "../Buffer.h"
#include "../Texture.h"

using namespace Gleam;

NO_DISCARD BufferHandle::operator Buffer() const
{
	return static_cast<RenderGraphBufferNode*>(node)->buffer;
}

NO_DISCARD TextureHandle::operator Texture() const
{
	return static_cast<RenderGraphTextureNode*>(node)->texture;
}