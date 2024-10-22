#pragma once

namespace Gleam {

class GraphicsDevice;

class UploadManager final
{
	static constexpr size_t UploadHeapSize = 512 * 1024 * 1024;
public:

    UploadManager(GraphicsDevice* device);

    ~UploadManager();

	void Commit() const;
	
	void Reset() const;
	
	void CommitUpload(const Buffer& buffer, const void* data, size_t size, size_t offset = 0) const;

	void CommitUpload(const Texture& texture, const void* data, size_t size) const;

private:

	struct Impl;
	Scope<Impl> mHandle;

    GraphicsDevice* mDevice;

};

} // namespace Gleam