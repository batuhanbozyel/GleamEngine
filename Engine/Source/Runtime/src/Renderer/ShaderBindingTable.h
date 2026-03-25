#pragma once
#include "GraphicsObject.h"

namespace Gleam {

class GraphicsDevice;

class ShaderBindingTable : public GraphicsObject
{
	friend class GraphicsDevice;
public:

	ShaderBindingTable() = default;
    
    ShaderBindingTable(const ShaderBindingTable& other) = default;
    
    ShaderBindingTable& operator=(const ShaderBindingTable& other) = default;

	ShaderBindingTable(NativeGraphicsHandle handle, const GPUVirtualAddressRange& rayGenRecord, const GPUVirtualAddressRangeAndStride& missRecord, const GPUVirtualAddressRangeAndStride& hitGroupRecord)
		: GraphicsObject(handle)
		, mRayGenRecord(rayGenRecord)
		, mMissRecord(missRecord)
		, mHitGroupRecord(hitGroupRecord)
	{
		
	}

	const GPUVirtualAddressRange& GetRayGenRecord() const
	{
		return mRayGenRecord;
	}

	const GPUVirtualAddressRangeAndStride& GetMissRecord() const
	{
		return mMissRecord;
	}

	const GPUVirtualAddressRangeAndStride& GetHitGroupRecord() const
	{
		return mHitGroupRecord;
	}

private:

	GPUVirtualAddressRange mRayGenRecord;
	GPUVirtualAddressRangeAndStride mMissRecord;
	GPUVirtualAddressRangeAndStride mHitGroupRecord;

};
	
} // namespace Gleam
