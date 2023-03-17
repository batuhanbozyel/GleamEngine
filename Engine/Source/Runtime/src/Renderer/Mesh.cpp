#include "gpch.h"
#include "Mesh.h"
#include "Assets/Model.h"

using namespace Gleam;

static BoundingBox CalculateBoundingBox(const TArray<Vector3>& positions)
{
    BoundingBox bounds(Math::Infinity, Math::NegativeInfinity);
    for (const auto& position : positions)
    {
        bounds.min = Math::Min(bounds.min, position);
        bounds.max = Math::Max(bounds.max, position);
    }
    return bounds;
}

static MeshData BatchMeshes(const TArray<MeshData>& meshes)
{
    MeshData batchedMesh;
    for (const auto& mesh : meshes)
    {
        auto endIdx = batchedMesh.indices.size();
        batchedMesh.indices.insert(batchedMesh.indices.end(), mesh.indices.begin(), mesh.indices.end());
        
        for (auto i = endIdx; i < batchedMesh.indices.size(); i++)
        {
            batchedMesh.indices[i] += static_cast<uint32_t>(batchedMesh.positions.size());
        }
        
        batchedMesh.positions.insert(batchedMesh.positions.end(), mesh.positions.begin(), mesh.positions.end());
        batchedMesh.normals.insert(batchedMesh.normals.end(), mesh.normals.begin(), mesh.normals.end());
        batchedMesh.texCoords.insert(batchedMesh.texCoords.end(), mesh.texCoords.begin(), mesh.texCoords.end());
    }
    return batchedMesh;
}

Mesh::Mesh(const MeshData& mesh)
    : mBuffer(mesh)
{
	mSubmeshDescriptors.resize(1);
	mSubmeshDescriptors[0].baseVertex = 0;
	mSubmeshDescriptors[0].firstIndex = 0;
	mSubmeshDescriptors[0].indexCount = static_cast<uint32_t>(mesh.indices.size());
	mSubmeshDescriptors[0].bounds = CalculateBoundingBox(mesh.positions);
}

Mesh::Mesh(const TArray<MeshData>& meshes)
    : mBuffer(meshes)
{
	uint32_t baseVertex = 0;
	uint32_t firstIndex = 0;
	mSubmeshDescriptors.resize(meshes.size());
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

StaticMesh::StaticMesh(const Model& model)
    : Mesh(BatchMeshes(model.GetMeshes()))
{
    
}

SkeletalMesh::SkeletalMesh(const Model& model)
    : Mesh(model.GetMeshes())
{
    
}

const MeshBuffer& Mesh::GetBuffer() const
{
    return mBuffer;
}

const TArray<SubmeshDescriptor>& Mesh::GetSubmeshDescriptors() const
{
    return mSubmeshDescriptors;
}
