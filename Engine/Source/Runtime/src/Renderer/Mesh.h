#pragma once
#include "Heap.h"
#include "Buffer.h"
#include "Core/GUID.h"

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
    TArray<Float3> positions;
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

GLEAM_TYPE(Gleam::SubmeshDescriptor, Guid("DD7E3A74-ADF4-45A9-8DFD-CA252EDC49A6"))
    GLEAM_FIELD(baseVertex, Serializable())
    GLEAM_FIELD(firstIndex, Serializable())
    GLEAM_FIELD(indexCount, Serializable())
    GLEAM_FIELD(materialIndex, Serializable())
GLEAM_END

GLEAM_TYPE(Gleam::InterleavedMeshVertex, Guid("4AFE936A-550F-419C-A7F0-5ED38D9D1642"))
    GLEAM_FIELD(normal, Serializable())
    GLEAM_FIELD(texCoord, Serializable())
GLEAM_END

GLEAM_TYPE(Gleam::MeshDescriptor, Guid("59E4007E-F7D4-4107-A05F-E1121067DCD3"))
    GLEAM_FIELD(name, Serializable())
    GLEAM_FIELD(indices, Serializable())
    GLEAM_FIELD(positions, Serializable())
    GLEAM_FIELD(interleavedVertices, Serializable())
    GLEAM_FIELD(submeshes, Serializable())
GLEAM_END
