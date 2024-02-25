#include "gpch.h"
#include "CommandBuffer.h"
#include "GraphicsDevice.h"

using namespace Gleam;

void CommandBuffer::BindBuffer(const BufferHandle& handle, size_t offset, uint32_t index, ShaderStageFlagBits stage) const
{
	const Buffer& buffer = handle;
	BindBuffer(buffer.GetHandle(), buffer.GetDescriptor().usage, offset, index, stage, handle.GetAccess());
}

void CommandBuffer::BindBuffer(const Buffer& buffer, size_t offset, uint32_t index, ShaderStageFlagBits stage, ResourceAccess access = ResourceAccess::Read) const
{
	BindBuffer(buffer.GetHandle(), buffer.GetDescriptor().usage, offset, index, stage, access);
}

void CommandBuffer::BindTexture(const TextureHandle& handle, uint32_t index, ShaderStageFlagBits stage) const
{
	const Texture& texture = handle;
	BindTexture(texture.GetView(), index, stage, handle.GetAccess());
}

void CommandBuffer::BindTexture(const Texture& texture, uint32_t index, ShaderStageFlagBits stage, ResourceAccess access = ResourceAccess::Read) const
{
	BindTexture(texture.GetView(), index, stage, access);
}

void CommandBuffer::DrawIndexed(const Buffer& indexBuffer, IndexType type, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t baseVertex = 0, uint32_t baseInstance = 0) const
{
	DrawIndexed(indexBuffer, type, static_cast<uint32_t>(indexBuffer.GetDescriptor().size / SizeOfIndexType(type)), instanceCount, firstIndex, baseVertex, baseInstance);
}

void CommandBuffer::CopyBuffer(const Buffer& src, const Buffer& dst, size_t size, size_t srcOffset = 0, size_t dstOffset = 0) const
{
	CopyBuffer(src.GetHandle(), dst.GetHandle(), size, srcOffset, dstOffset);
}

void CommandBuffer::CopyBuffer(const Buffer& src, const Buffer& dst) const
{
	auto minSize = Math::Min(src.GetDescriptor().size, dst.GetDescriptor().size);
	CopyBuffer(src.GetHandle(), dst.GetHandle(), minSize);
}

void CommandBuffer::SetBufferData(const Buffer& buffer, const void* data, size_t size, size_t offset = 0) const
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