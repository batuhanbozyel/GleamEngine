#pragma once

namespace Gleam {

class GraphicsObject
{
public:

	GraphicsObject(NativeGraphicsHandle handle)
		: mHandle(handle)
	{

	}

	GraphicsObject() = default;

	GraphicsObject(const GraphicsObject&) = default;

	GraphicsObject& operator=(const GraphicsObject&) = default;

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

};

} // namespace Gleam
