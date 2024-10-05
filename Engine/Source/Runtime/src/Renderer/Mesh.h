#pragma once
#include "Heap.h"
#include "Buffer.h"
#include "MeshDescriptor.h"

namespace Gleam {

class Mesh
{
public:
    
    virtual ~Mesh() = default;
    
    Mesh(const MeshDescriptor& mesh);
    
    void Dispose();
    
    const Buffer& GetPositionBuffer() const;
    
    const Buffer& GetInterleavedBuffer() const;
    
    const Buffer& GetIndexBuffer() const;

    uint32_t GetSubmeshCount() const;
    
    const TArray<SubmeshDescriptor>& GetSubmeshes() const;
    
protected:
    
    Heap mHeap;
    Buffer mIndexBuffer;
    Buffer mPositionBuffer;
    Buffer mInterleavedBuffer;
    TArray<SubmeshDescriptor> mSubmeshes;
};

} // namespace Gleam
