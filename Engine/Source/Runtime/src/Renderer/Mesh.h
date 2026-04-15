#pragma once
#include "Buffer.h"
#include "MeshDescriptor.h"
#include "AccelerationStructure.h"
#include "Assets/Asset.h"

namespace Gleam {

class RayTracingScene;

class Mesh : public Asset
{
	friend class RayTracingScene;
public:
    
    Mesh(const MeshDescriptor& descriptor);
	
	~Mesh();
    
    const Buffer& GetPositionBuffer() const;
    
    const Buffer& GetInterleavedBuffer() const;
    
    const Buffer& GetIndexBuffer() const;

    const TArray<SubmeshDescriptor>& GetSubmeshes() const;

	const SubmeshDescriptor& GetSubmesh(uint32_t index) const;
    
	const BottomLevelAccelerationStructure& GetBLAS(uint32_t submesh) const;
    
protected:

    Buffer mIndexBuffer;
    Buffer mPositionBuffer;
    Buffer mInterleavedBuffer;
    TArray<SubmeshDescriptor> mSubmeshes;
	TArray<BottomLevelAccelerationStructure> mBLASes;
};

} // namespace Gleam
