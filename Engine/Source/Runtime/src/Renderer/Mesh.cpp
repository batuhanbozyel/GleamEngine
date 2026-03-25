#include "gpch.h"
#include "Mesh.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Renderer/RenderSystem.h"

using namespace Gleam;

Mesh::Mesh(const MeshDescriptor& mesh)
    : mSubmeshes(mesh.submeshes)
{
    static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();
    
    size_t positionSize = mesh.positions.size() * sizeof(Float3);
    size_t interleavedSize = mesh.interleavedVertices.size() * sizeof(InterleavedMeshVertex);
    size_t indexSize = mesh.indices.size() * sizeof(uint32_t);

    BufferDescriptor bufferDesc;
    bufferDesc.name = "Mesh: " + mesh.name + " Positions";
    bufferDesc.size = positionSize;
    mPositionBuffer = device->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);
    
    bufferDesc.name = "Mesh: " + mesh.name + " InterleavedData";
    bufferDesc.size = interleavedSize;
    mInterleavedBuffer = device->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);
    
    bufferDesc.name = "Mesh: " + mesh.name + " Indices";
    bufferDesc.size = indexSize;
    mIndexBuffer = device->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);

    // Send mesh data to buffers
	{
		auto cmd = renderSystem->GetCopyCommandBuffer();
		cmd->Commit(mPositionBuffer, mesh.positions.data(), positionSize, 0);
		cmd->Commit(mInterleavedBuffer, mesh.interleavedVertices.data(), interleavedSize, 0);
		cmd->Commit(mIndexBuffer, mesh.indices.data(), indexSize, 0);
	}
}

Mesh::~Mesh()
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();
	device->Dispose(renderSystem->GetAllocator(), mPositionBuffer);
	device->Dispose(renderSystem->GetAllocator(), mInterleavedBuffer);
	device->Dispose(renderSystem->GetAllocator(), mIndexBuffer);

	if (mBLAS.IsValid())
	{
		device->Dispose(renderSystem->GetAllocator(), mBLAS);
	}
}

const Buffer& Mesh::GetPositionBuffer() const
{
    return mPositionBuffer;
}

const Buffer& Mesh::GetInterleavedBuffer() const
{
    return mInterleavedBuffer;
}

const Buffer& Mesh::GetIndexBuffer() const
{
    return mIndexBuffer;
}

const TArray<SubmeshDescriptor>& Mesh::GetSubmeshes() const
{
    return mSubmeshes;
}

const BottomLevelAccelerationStructure& Mesh::GetBLAS() const
{
	return mBLAS;
}