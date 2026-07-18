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

    const Buffer& GetBuffer() const;

    const BufferRange& GetPositions() const;

    const BufferRange& GetInterleavedVertices() const;

    const BufferRange& GetIndices() const;

    const BufferRange& GetMeshlets() const;

    const BufferRange& GetMeshletVertices() const;

    const BufferRange& GetMeshletTriangleIndices() const;

    const TArray<SubmeshDescriptor>& GetSubmeshes() const;

	const SubmeshDescriptor& GetSubmesh(uint32_t index) const;

	const BottomLevelAccelerationStructure& GetBLAS(uint32_t submesh) const;

protected:

    Buffer mBuffer;
    BufferRange mPositions;
    BufferRange mInterleavedVertices;
    BufferRange mIndices;
    BufferRange mMeshlets;
    BufferRange mMeshletVertices;
    BufferRange mMeshletTriangleIndices;
    TArray<SubmeshDescriptor> mSubmeshes;
	TArray<BottomLevelAccelerationStructure> mBLASes;
};

} // namespace Gleam
