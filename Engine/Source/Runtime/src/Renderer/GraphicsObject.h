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

class MutableGraphicsObject
{
public:

    GLEAM_NONCOPYABLE(MutableGraphicsObject);
    MutableGraphicsObject() = default;
    
    NativeGraphicsHandle GetHandle() const;

    bool IsValid() const;

protected:

	TArray<NativeGraphicsHandle> mHandles;
    TArray<NativeGraphicsHandle> mMemories;

};

} // namespace Gleam
