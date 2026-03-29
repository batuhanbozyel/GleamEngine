#include "gpch.h"
#include "Mesh.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Renderer/RenderSystem.h"

using namespace Gleam;

Mesh::Mesh(const MeshDescriptor& descriptor)
    : Asset(descriptor.name)
	, mSubmeshes(descriptor.submeshes)
{
    static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();
    
    size_t positionSize = descriptor.positions.size() * sizeof(Float3);
    size_t interleavedSize = descriptor.interleavedVertices.size() * sizeof(InterleavedMeshVertex);
    size_t indexSize = descriptor.indices.size() * sizeof(uint32_t);

    BufferDescriptor bufferDesc;
    bufferDesc.name = "Mesh: " + descriptor.name + " Positions";
    bufferDesc.size = positionSize;
    mPositionBuffer = device->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);
    
    bufferDesc.name = "Mesh: " + descriptor.name + " InterleavedData";
    bufferDesc.size = interleavedSize;
    mInterleavedBuffer = device->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);
    
    bufferDesc.name = "Mesh: " + descriptor.name + " Indices";
    bufferDesc.size = indexSize;
    mIndexBuffer = device->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);

    // Send mesh data to buffers
	{
		auto cmd = renderSystem->GetCopyCommandBuffer();
		cmd->Commit(mPositionBuffer, descriptor.positions.data(), positionSize, 0);
		cmd->Commit(mInterleavedBuffer, descriptor.interleavedVertices.data(), interleavedSize, 0);
		cmd->Commit(mIndexBuffer, descriptor.indices.data(), indexSize, 0);
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
		device->Dispose(mBLAS);
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