#pragma once
#include "GraphicsObject.h"

namespace Gleam {

#if defined(USE_DIRECTX_RENDERER)
using AccelerationStructureView = ShaderResourceIndex;
#else
struct AccelerationStructureView
{
	ShaderResourceIndex index = InvalidResourceIndex;
	NativeGraphicsHandle header = nil;

	NO_DISCARD operator ShaderResourceIndex() const
	{
		return index;
	}
};
#endif

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
	uint32_t instanceCount = 0;
};

class TopLevelAccelerationStructure final : public GraphicsObject
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

	TopLevelAccelerationStructure(const TLASDescriptor& descriptor, NativeGraphicsHandle handle, AccelerationStructureView view)
		: GraphicsObject(handle)
		, mDescriptor(descriptor)
		, mResourceView(view)
	{

	}

	const TLASDescriptor& GetDescriptor() const
	{
		return mDescriptor;
	}

	AccelerationStructureView GetResourceView() const
	{
		return mResourceView;
	}

private:

	TLASDescriptor mDescriptor;

	AccelerationStructureView mResourceView;

};

} // namespace Gleam
