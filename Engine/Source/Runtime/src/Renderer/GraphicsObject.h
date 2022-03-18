#pragma once

namespace Gleam {

class GraphicsObject
{
public:

	NativeGraphicsHandle GetHandle() const
	{
		return mHandle;
	}

protected:

	NativeGraphicsHandle mHandle = nullptr;

};

} // namespace Gleam