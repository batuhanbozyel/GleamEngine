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
    
	const BottomLevelAccelerationStructure& GetBLAS() const;
    
protected:

    Buffer mIndexBuffer;
    Buffer mPositionBuffer;
    Buffer mInterleavedBuffer;
    TArray<SubmeshDescriptor> mSubmeshes;
	BottomLevelAccelerationStructure mBLAS;
};

} // namespace Gleam
