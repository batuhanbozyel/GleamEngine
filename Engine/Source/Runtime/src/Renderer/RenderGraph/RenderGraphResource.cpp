#include "gpch.h"
#include "RenderGraphResource.h"
#include "RenderGraphNode.h"

using namespace Gleam;

NO_DISCARD const Buffer& BufferHandle::GetBuffer() const
{
	return node->buffer;
}

NO_DISCARD const Texture& TextureHandle::GetTexture() const
{
	return node->texture;
}
