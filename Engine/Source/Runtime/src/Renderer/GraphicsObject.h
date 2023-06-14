#pragma once

namespace Gleam {

class GraphicsObject
{
public:

    GLEAM_NONCOPYABLE(GraphicsObject);
    GraphicsObject() = default;
    
	NativeGraphicsHandle GetHandle() const
	{
		return mHandle;
	}

	bool IsValid() const
	{
		return mHandle;
	}

protected:

	NativeGraphicsHandle mHandle = nullptr;
    NativeGraphicsHandle mMemory = nullptr;

};

} // namespace Gleam
