#include "gpch.h"
#include "MeshBuffer.h"

using namespace Gleam;

MeshBuffer::MeshBuffer(const MeshData& mesh)
    : mVertexBuffer(mesh.positions.size()), mIndexBuffer(mesh.indices)
{
    mVertexBuffer.SetData(mesh.positions);
    
    /*
     TODO: Refactor Vertex Buffer layout to fit better into Vertex Pulling
     -> May be separated into two buffers
     -> Or store as all floats in a single buffer
     */
    struct InterleavedVertexData
    {
        Vector3 normal;
        Vector2 texCoord;
    };
    
    TArray<InterleavedVertexData> interleavedData(mesh.normals.size());
    for (uint32_t i = 0; i < mesh.normals.size(); i++)
    {
        interleavedData[i].normal = mesh.normals[i];
        interleavedData[i].texCoord = mesh.texCoords[i];
    }
    mVertexBuffer.SetData(interleavedData, mesh.positions.size() * sizeof(Vector3));
}
