#pragma once
#include "Model.h"
#include "Buffer.h"

namespace Gleam {
    
class MeshBuffer
{
public:
    
    MeshBuffer(const MeshData& mesh);
    
    const VertexBuffer<MeshVertex>& GetVertexBuffer() const
    {
        return mVertexBuffer;
    }
    
    const IndexBuffer& GetIndexBuffer() const
    {
        return mIndexBuffer;
    }
    
private:
    
    IndexBuffer mIndexBuffer;
    VertexBuffer<MeshVertex> mVertexBuffer;
    
};
    
} // namespace Gleam
