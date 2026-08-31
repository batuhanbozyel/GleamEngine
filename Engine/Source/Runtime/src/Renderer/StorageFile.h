#pragma once
#include "GraphicsObject.h"

namespace Gleam {

class StorageFile final : public GraphicsObject
{
	friend class CopyCommandBuffer;
public:

	StorageFile() = default;

	StorageFile(const StorageFile& other) = default;

	StorageFile& operator=(const StorageFile& other) = default;

private:

	StorageFile(NativeGraphicsHandle handle)
		: GraphicsObject(handle)
	{

	}

};

} // namespace Gleam
