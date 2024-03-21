#pragma once
#include "Heap.h"
#include "Buffer.h"

namespace Gleam {

struct SubmeshDescriptor
{
    BoundingBox bounds;
    uint32_t baseVertex = 0;
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
    uint32_t materialIndex = 0;
};

struct MeshDescriptor
{
    TString name;
    TArray<uint32_t> indices;
    TArray<Vector3> positions;
    TArray<InterleavedMeshVertex> interleavedVertices;
    TArray<SubmeshDescriptor> submeshes;
};

class Mesh
{
public:
    
    GLEAM_NONCOPYABLE(Mesh);
    
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
