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

void CommandBuffer::SetBufferData(const Buffer& buffer, const void* data, size_t size, size_t offset) const
{
	auto contents = buffer.GetContents();
	if (contents == nullptr)
	{
        BufferDescriptor desc;
        desc.name = "CommandBuffer::StagingBuffer";
        desc.size = size;
        
		Buffer stagingBuffer = mStagingHeap.CreateBuffer(desc);
		memcpy(stagingBuffer.GetContents(), data, size);
		CopyBuffer(stagingBuffer.GetHandle(), buffer.GetHandle(), size, 0, offset);
		mDevice->ReleaseBuffer(stagingBuffer);
	}
	else
	{
		memcpy(static_cast<uint8_t*>(contents) + offset, data, size);
	}
}
