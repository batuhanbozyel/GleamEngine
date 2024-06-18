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
    
    const TArray<SubmeshDescriptor>& GetSubmeshDescriptors() const;
    
protected:
    
    Heap mHeap;
    Buffer mIndexBuffer;
    Buffer mPositionBuffer;
    Buffer mInterleavedBuffer;
    TArray<SubmeshDescriptor> mSubmeshDescriptors;
};

class StaticMesh final : public Mesh
{
public:
    
};

class SkeletalMesh final : public Mesh
{
public:
    
};

} // namespace Gleam
