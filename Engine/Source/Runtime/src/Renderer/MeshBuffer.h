#pragma once
#include "Buffer.h"
#include "ShaderTypes.h"

namespace Gleam {

struct MeshData;

class MeshBuffer
{
public:
    
    MeshBuffer(const MeshData& mesh);

	MeshBuffer(const TArray<MeshData>& meshes);
    
    const VertexBuffer<Vector3>& GetPositionBuffer() const;
    
    const VertexBuffer<InterleavedMeshVertex>& GetInterleavedBuffer() const;
    
    const IndexBuffer<IndexType::UINT32>& GetIndexBuffer() const;
    
private:

	static TArray<Vector3> PreparePositionData(const TArray<MeshData>& meshes);

	static TArray<InterleavedMeshVertex> PrepareInterleavedData(const TArray<MeshData>& meshes);

	static TArray<uint32_t> PrepareIndexData(const TArray<MeshData>& meshes);
    
    IndexBuffer<IndexType::UINT32> mIndexBuffer;
    VertexBuffer<Vector3> mPositionBuffer;
    VertexBuffer<InterleavedMeshVertex> mInterleavedBuffer;
    
};

} // namespace Gleam
