#include "gpch.h"
#include "Mesh.h"

using namespace Gleam;

BoundingBox Mesh::CalculateBoundingBox(const TArray<Vector3>& positions)
{
    BoundingBox bounds(Math::Infinity, Math::NegativeInfinity);
    for (const auto& position : positions)
    {
        bounds.min = Math::Min(bounds.min, position);
        bounds.max = Math::Max(bounds.max, position);
    }
    return bounds;
}

MeshData Mesh::BatchMeshes(const TArray<MeshData>& meshes)
{
    MeshData batchedMesh;
    for (const auto& mesh : meshes)
    {
        batchedMesh.positions.insert(batchedMesh.positions.end(), mesh.positions.begin(), mesh.positions.end());
        batchedMesh.normals.insert(batchedMesh.normals.end(), mesh.normals.begin(), mesh.normals.end());
        batchedMesh.texCoords.insert(batchedMesh.texCoords.end(), mesh.texCoords.begin(), mesh.texCoords.end());
        batchedMesh.indices.insert(batchedMesh.indices.end(), mesh.indices.begin(), mesh.indices.end());
    }
    return batchedMesh;
}

StaticMesh::StaticMesh(const Model& model)
    : StaticMesh(BatchMeshes(model.GetMeshes()))
{
    
}

StaticMesh::StaticMesh(const MeshData& batchedMesh)
    : Mesh(batchedMesh)
{
    mSubmeshDescriptor.bounds = CalculateBoundingBox(batchedMesh.positions);
    mSubmeshDescriptor.indexCount = static_cast<uint32_t>(batchedMesh.indices.size());
}

const SubmeshDescriptor& StaticMesh::GetSubmeshDescriptor() const
{
    return mSubmeshDescriptor;
}

SkeletalMesh::SkeletalMesh(const Model& model)
    : mSubmeshDescriptors(model.GetMeshes().size()), Mesh(BatchMeshes(model.GetMeshes()))
{
    uint32_t baseVertex = 0;
    uint32_t firstIndex = 0;
    const auto& meshes = model.GetMeshes();
    for (uint32_t i = 0; i < meshes.size(); i++)
    {
        mSubmeshDescriptors[i].baseVertex = baseVertex;
        mSubmeshDescriptors[i].firstIndex = firstIndex;
        mSubmeshDescriptors[i].indexCount = static_cast<uint32_t>(meshes[i].indices.size());
        mSubmeshDescriptors[i].bounds = CalculateBoundingBox(meshes[i].positions);
        
        baseVertex += static_cast<uint32_t>(meshes[i].positions.size());
        firstIndex += static_cast<uint32_t>(meshes[i].indices.size());
    }
}

const TArray<SubmeshDescriptor>& SkeletalMesh::GetSubmeshDescriptors() const
{
    return mSubmeshDescriptors;
}
