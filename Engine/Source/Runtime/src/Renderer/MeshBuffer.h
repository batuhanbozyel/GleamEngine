#pragma once
#include "Model.h"
#include "Buffer.h"

namespace Gleam {
    
class MeshBuffer
{
public:
    
    MeshBuffer(const MeshData& mesh);
    
    const VertexBuffer<Vector3>& GetPositionBuffer() const;
    
    const VertexBuffer<InterleavedMeshVertex>& GetInterleavedBuffer() const;
    
    const IndexBuffer& GetIndexBuffer() const;
    
private:
    
    IndexBuffer mIndexBuffer;
    VertexBuffer<Vector3> mPositionBuffer;
    VertexBuffer<InterleavedMeshVertex> mInterleavedBuffer;
    
};
    
} // namespace Gleam
