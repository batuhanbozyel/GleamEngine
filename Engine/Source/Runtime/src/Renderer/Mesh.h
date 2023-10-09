#pragma once
#include "MeshBuffer.h"

namespace Gleam {

struct MeshData
{
    TArray<Vector3> positions;
    TArray<Vector3> normals;
    TArray<Vector2> texCoords;
    TArray<uint32_t> indices;
};

struct SubmeshDescriptor
{
    BoundingBox bounds;
    uint32_t baseVertex = 0;
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
};

class Mesh
{
public:
    
    GLEAM_NONCOPYABLE(Mesh);
    
    Mesh(const MeshData& mesh);

    Mesh(const TArray<MeshData>& meshes);
    
    virtual ~Mesh();

    uint32_t GetSubmeshCount() const;
    
    const MeshBuffer& GetBuffer() const;
    
    const TArray<SubmeshDescriptor>& GetSubmeshDescriptors() const;
    
protected:
    
    MeshBuffer mBuffer;
    
    TArray<SubmeshDescriptor> mSubmeshDescriptors;
    
};

class StaticMesh final : public Mesh
{
public:

    StaticMesh(const MeshData& mesh);

    StaticMesh(const TArray<MeshData>& meshes);
    
};

class SkeletalMesh final : public Mesh
{
public:
    
    SkeletalMesh(const MeshData& mesh);

    SkeletalMesh(const TArray<MeshData>& meshes);
    
};

} // namespace Gleam
