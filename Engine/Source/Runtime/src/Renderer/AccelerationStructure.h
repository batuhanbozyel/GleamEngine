#pragma once
#include "GraphicsObject.h"

namespace Gleam {

struct BLASDescriptor
{
	TString name;
	size_t size = 0;
};

class BottomLevelAccelerationStructure final : public GraphicsObject
{
    friend class GraphicsDevice;
public:

	BottomLevelAccelerationStructure() = default;
    
    BottomLevelAccelerationStructure(const BottomLevelAccelerationStructure& other) = default;
    
    BottomLevelAccelerationStructure& operator=(const BottomLevelAccelerationStructure& other) = default;

    BottomLevelAccelerationStructure(const BLASDescriptor& descriptor)
        : mDescriptor(descriptor)
    {
        
    }

	BottomLevelAccelerationStructure(const BLASDescriptor& descriptor, NativeGraphicsHandle handle)
		: GraphicsObject(handle)
		, mDescriptor(descriptor)
	{
		
	}
    
    const BLASDescriptor& GetDescriptor() const
    {
        return mDescriptor;
    }
    
private:

    BLASDescriptor mDescriptor;
    
};

struct TLASDescriptor
{
	TString name;
	size_t size = 0;
};

class TopLevelAccelerationStructure final : public ShaderResource
{
	friend class GraphicsDevice;
public:

	TopLevelAccelerationStructure() = default;

	TopLevelAccelerationStructure(const TopLevelAccelerationStructure& other) = default;

	TopLevelAccelerationStructure& operator=(const TopLevelAccelerationStructure& other) = default;

	TopLevelAccelerationStructure(const TLASDescriptor& descriptor)
		: mDescriptor(descriptor)
	{

	}

	TopLevelAccelerationStructure(const TLASDescriptor& descriptor, NativeGraphicsHandle handle, ShaderResourceIndex view)
		: ShaderResource(handle, view)
		, mDescriptor(descriptor)
	{

	}

	const TLASDescriptor& GetDescriptor() const
	{
		return mDescriptor;
	}

private:

	TLASDescriptor mDescriptor;

};

} // namespace Gleam