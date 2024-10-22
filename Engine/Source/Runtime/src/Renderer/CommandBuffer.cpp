#include "gpch.h"
#include "CommandBuffer.h"
#include "GraphicsDevice.h"

using namespace Gleam;

void CommandBuffer::DrawIndexed(const Buffer& indexBuffer,
	IndexType type,
	uint32_t instanceCount,
	uint32_t firstIndex) const
{
	DrawIndexed(indexBuffer, type, static_cast<uint32_t>(indexBuffer.GetSize() / SizeOfIndexType(type)), instanceCount, firstIndex);
}

void CommandBuffer::CopyBuffer(const Buffer& src, const Buffer& dst,
	size_t size,
	size_t srcOffset,
	size_t dstOffset) const
{
	CopyBuffer(src.GetHandle(), dst.GetHandle(), size, srcOffset, dstOffset);
}

void CommandBuffer::CopyBuffer(const Buffer& src, const Buffer& dst) const
{
	auto minSize = Math::Min(src.GetSize(), dst.GetSize());
	CopyBuffer(src.GetHandle(), dst.GetHandle(), minSize);
}
