#include "gpch.h"
#include "CommandBuffer.h"
#include "GraphicsDevice.h"

using namespace Gleam;

void CommandBuffer::BindBuffer(const BufferHandle& handle,
	size_t offset,
	uint32_t index,
	ShaderStageFlagBits stage) const
{
	const Buffer& buffer = handle;
	BindBuffer(buffer, offset, index, stage, handle.GetAccess());
}

void CommandBuffer::BindTexture(const TextureHandle& handle,
	uint32_t index,
	ShaderStageFlagBits stage) const
{
	const Texture& texture = handle;
	BindTexture(texture, index, stage, handle.GetAccess());
}

void CommandBuffer::DrawIndexed(const Buffer& indexBuffer,
	IndexType type,
	uint32_t instanceCount,
	uint32_t firstIndex,
	uint32_t baseVertex,
	uint32_t baseInstance) const
{
	DrawIndexed(indexBuffer, type, static_cast<uint32_t>(indexBuffer.GetDescriptor().size / SizeOfIndexType(type)), instanceCount, firstIndex, baseVertex, baseInstance);
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
	auto minSize = Math::Min(src.GetDescriptor().size, dst.GetDescriptor().size);
	CopyBuffer(src.GetHandle(), dst.GetHandle(), minSize);
}

void CommandBuffer::SetBufferData(const Buffer& buffer, const void* data, size_t size, size_t offset) const
{
	auto contents = buffer.GetContents();
	if (contents == nullptr)
	{
		BufferDescriptor bufferDesc;
		bufferDesc.size = size;
		bufferDesc.usage = BufferUsage::StagingBuffer;
		Buffer stagingBuffer = mStagingHeap.CreateBuffer(bufferDesc);
		
		memcpy(stagingBuffer.GetContents(), data, size);
		CopyBuffer(stagingBuffer.GetHandle(), buffer.GetHandle(), size, 0, offset);
		
		mDevice->Dispose(stagingBuffer);
	}
	else
	{
		memcpy(static_cast<uint8_t*>(contents) + offset, data, size);
	}
}