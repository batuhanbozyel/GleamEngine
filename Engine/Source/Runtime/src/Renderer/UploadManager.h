#pragma once

namespace Gleam {

class GraphicsDevice;

class UploadManager final
{
public:
    UploadManager(GraphicsDevice* device);

    ~UploadManager();

	void Commit();

	void CommitUpload(const Buffer& buffer, const void* data, size_t size, size_t offset = 0);

	void CommitUpload(const Texture& texture, const void* data, size_t size, uint32_t dstX = 0, uint32_t dstY = 0, uint32_t dstZ = 0);

private:

	struct Impl;
	Scope<Impl> mHandle;

    GraphicsDevice* mDevice;

};

} // namespace Gleam