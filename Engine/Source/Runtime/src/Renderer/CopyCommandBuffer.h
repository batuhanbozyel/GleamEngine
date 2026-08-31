#pragma once
#include "StorageFile.h"

#include "Core/Macro.h"
#include "IO/Path.h"
#include "Container/Pointer.h"
#include "Container/BinaryBuffer.h"

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

	NO_DISCARD StorageFile OpenFile(const Path& path) const;

	void CloseFile(StorageFile& file) const;

	void Commit(const Buffer& buffer, const void* data, size_t size, size_t offset) const;

	void Commit(const Texture& texture, const void* data, size_t size, uint32_t mip, uint32_t slice) const;

	void Commit(const Buffer& buffer, const StorageFile& file, const BufferRange& range, size_t offset) const;

private:

	struct Impl;
	Scope<Impl> mHandle;

    GraphicsDevice* mDevice;

};

} // namespace Gleam
