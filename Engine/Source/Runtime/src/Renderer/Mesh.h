#pragma once
#include "MeshBuffer.h"

namespace Gleam {

class Mesh
{
public:
    
    Mesh(const MeshData& mesh)
        : mBuffer(mesh)
    {
        
    }
    
    const MeshBuffer& GetBuffer() const
    {
        return mBuffer;
    }
    
protected:
    
    static BoundingBox CalculateBoundingBox(const TArray<Vector3>& positions);
    
    static MeshData BatchMeshes(const TArray<MeshData>& meshes);
    
    MeshBuffer mBuffer;
    
};

struct SubmeshDescriptor
{
    BoundingBox bounds;
    uint32_t baseVertex = 0;
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
};
    
class StaticMesh : public Mesh
{
public:

    StaticMesh(const Model& model);
    
    const SubmeshDescriptor& GetSubmeshDescriptor() const;
    
private:
    
    StaticMesh(const MeshData& batchedMesh);
    
    SubmeshDescriptor mSubmeshDescriptor;
    
};

class SkeletalMesh : public Mesh
{
public:
    
    SkeletalMesh(const Model& model);
    
    const TArray<SubmeshDescriptor>& GetSubmeshDescriptors() const;
    
private:
    
    TArray<SubmeshDescriptor> mSubmeshDescriptors;
    
};

} // namespace Gleam
