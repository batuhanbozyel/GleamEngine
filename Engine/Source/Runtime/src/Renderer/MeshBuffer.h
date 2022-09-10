#pragma once
#include "Model.h"
#include "Buffer.h"
#include "ShaderTypes.h"

namespace Gleam {
    
class MeshBuffer
{
public:
    
    MeshBuffer(const MeshData& mesh);
    
    const VertexBuffer<Vector3>& GetPositionBuffer() const;
    
    const VertexBuffer<InterleavedMeshVertex>& GetInterleavedBuffer() const;
    
    const IndexBuffer<IndexType::UINT32>& GetIndexBuffer() const;
    
private:
    
    IndexBuffer<IndexType::UINT32> mIndexBuffer;
    VertexBuffer<Vector3> mPositionBuffer;
    VertexBuffer<InterleavedMeshVertex> mInterleavedBuffer;
    
};
    
} // namespace Gleam
