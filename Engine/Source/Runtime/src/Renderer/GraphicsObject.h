#pragma once
#include "Core/EngineDefines.h"
#include "Shaders/ShaderInterop.h"

namespace Gleam {

class GraphicsObject
{
public:

    GraphicsObject() = default;
    
	GraphicsObject(NativeGraphicsHandle handle)
		: mHandle(handle)
	{

	}
    
    virtual ~GraphicsObject() = default;

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

class ShaderResource : public GraphicsObject
{
public:
    
    ShaderResource() = default;
    
    ShaderResource(NativeGraphicsHandle handle)
        : GraphicsObject(handle)
    {

    }

	ShaderResource(NativeGraphicsHandle handle, ShaderResourceIndex view)
		: GraphicsObject(handle)
	{
		mResourceView = view;
	}
    
    virtual ~ShaderResource() = default;
    
    ShaderResource(const ShaderResource&) = default;

    ShaderResource& operator=(const ShaderResource&) = default;
    
    ShaderResourceIndex GetResourceView() const
    {
        return mResourceView;
    }
    
protected:
    
    ShaderResourceIndex mResourceView = InvalidResourceIndex;
    
};

} // namespace Gleam
