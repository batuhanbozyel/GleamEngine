#include "gpch.h"
#include "Mesh.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/CopyCommandBuffer.h"

using namespace Gleam;

Mesh::Mesh(const MeshDescriptor& descriptor)
    : Asset(descriptor.name)
	, mSubmeshes(descriptor.submeshes)
	, mBLASes(descriptor.submeshes.size())
	, mPositions(descriptor.positions)
	, mInterleavedVertices(descriptor.interleavedVertices)
	, mIndices(descriptor.indices)
	, mMeshlets(descriptor.meshlets)
	, mMeshletVertices(descriptor.meshletVertices)
	, mMeshletTriangleIndices(descriptor.meshletTriangleIndices)
{
    static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();

    BufferDescriptor bufferDesc;
    bufferDesc.name = "Mesh: " + descriptor.name;
    bufferDesc.size = descriptor.buffer.size;
    mBuffer = device->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);

	auto cmd = renderSystem->GetCopyCommandBuffer();
	cmd->Commit(mBuffer, descriptor.buffer.data, descriptor.buffer.size, 0);
}

Mesh::~Mesh()
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();
	device->Dispose(renderSystem->GetAllocator(), mBuffer, BarrierStage::None);

	for (auto& blas : mBLASes)
	{
		if (blas.IsValid())
		{
			device->Dispose(blas);
		}
	}
}

const Buffer& Mesh::GetBuffer() const
{
    return mBuffer;
}

const BufferRange& Mesh::GetPositions() const
{
    return mPositions;
}

const BufferRange& Mesh::GetInterleavedVertices() const
{
    return mInterleavedVertices;
}

const BufferRange& Mesh::GetIndices() const
{
    return mIndices;
}

const BufferRange& Mesh::GetMeshlets() const
{
	return mMeshlets;
}

const BufferRange& Mesh::GetMeshletVertices() const
{
	return mMeshletVertices;
}

const BufferRange& Mesh::GetMeshletTriangleIndices() const
{
	return mMeshletTriangleIndices;
}

const TArray<SubmeshDescriptor>& Mesh::GetSubmeshes() const
{
    return mSubmeshes;
}

const SubmeshDescriptor& Mesh::GetSubmesh(uint32_t index) const
{
	return mSubmeshes[index];
}

const BottomLevelAccelerationStructure& Mesh::GetBLAS(uint32_t submesh) const
{
	return mBLASes[submesh];
}
