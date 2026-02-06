#pragma once
#include "Container/Pointer.h"

namespace Gleam {

class Buffer;
class Texture;
class GraphicsDevice;
class CommandBuffer;

class CopyCommandBuffer final
{
	static constexpr size_t UploadHeapSize = 128 * 1024 * 1024;
public:

    CopyCommandBuffer(GraphicsDevice* device);

    ~CopyCommandBuffer();

	void Execute() const;

	void Barrier(const CommandBuffer* cmd) const;
	
	void WaitUntilCompleted() const;
	
	void Commit(const Buffer& buffer, const void* data, size_t size, size_t offset) const;

	void Commit(const Texture& texture, const void* data, size_t size, uint32_t mip, uint32_t slice) const;

private:

	struct Impl;
	Scope<Impl> mHandle;

    GraphicsDevice* mDevice;

};

} // namespace Gleam
