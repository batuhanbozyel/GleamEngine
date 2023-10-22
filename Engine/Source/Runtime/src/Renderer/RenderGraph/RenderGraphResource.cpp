#include "gpch.h"
#include "RenderGraphResource.h"
#include "RenderGraphNode.h"

using namespace Gleam;

NO_DISCARD BufferHandle::operator Buffer() const
{
	return node->buffer;
}

NO_DISCARD TextureHandle::operator Texture() const
{
	return node->texture;
}
