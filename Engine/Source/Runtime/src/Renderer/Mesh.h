#pragma once
#include "MeshBuffer.h"

namespace Gleam {
    
class Model;

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
    
	Mesh(const MeshData& mesh);

    Mesh(const TArray<MeshData>& meshes);
    
    const MeshBuffer& GetBuffer() const;
    
    const TArray<SubmeshDescriptor>& GetSubmeshDescriptors() const;
    
protected:
    
    Mesh(const MeshData& mesh, bool isBatched);
    
    static BoundingBox CalculateBoundingBox(const TArray<Vector3>& positions);
    
    static MeshData BatchMeshes(const TArray<MeshData>& meshes, bool isBatchRendered);
    
    MeshBuffer mBuffer;
    
    TArray<SubmeshDescriptor> mSubmeshDescriptors;
    
};
    
class StaticMesh : public Mesh
{
public:

    StaticMesh(const Model& model);
    
};

class SkeletalMesh : public Mesh
{
public:
    
    SkeletalMesh(const Model& model);
    
};

} // namespace Gleam
