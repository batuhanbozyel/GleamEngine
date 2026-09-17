#include "gpch.h"
#include "Mesh.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Assets/AssetManager.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/CopyCommandBuffer.h"

using namespace Gleam;

Mesh::Mesh(const AssetReference& reference, const AssetHeader& header, const MeshDescriptor& descriptor)
	: Asset(reference, header)
	, mDescriptor(descriptor)
	, mLods(descriptor.lods.size())
{
	GLEAM_ASSERT(mDescriptor.lods.size() > 0, "Mesh has no LODs: {0}", descriptor.name);
	mBLASes.resize(mDescriptor.lods[0].submeshes.size());
	RequestLod(0);
}

Mesh::~Mesh()
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();

	for (auto& lod : mLods)
	{
		if (lod.IsValid())
		{
			device->Dispose(renderSystem->GetAllocator(), lod, BarrierStage::None);
		}
	}

	for (auto& blas : mBLASes)
	{
		if (blas.IsValid())
		{
			device->Dispose(blas);
		}
	}
}

void Mesh::RequestLod(uint32_t lod)
{
	GLEAM_ASSERT(lod < mLods.size(), "Mesh LOD {0} is out of range for: {1}", lod, GetName());
	
	auto& lodData = mLods[lod];
	if (not lodData.IsValid())
	{
		static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
		static auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
		
		auto storage = assetManager->GetStorage();
		const auto& lodDesc = mDescriptor.lods[lod];
		const auto blob = FindBlob<MeshLodDescriptor>(lodDesc.blobSlot, AssetPlatform::Common, AssetBackend::Common);
		if (blob == nullptr)
		{
			return;
		}

		BufferDescriptor bufferDesc;
		bufferDesc.name = "Mesh: " + mDescriptor.name;
		bufferDesc.size = blob->range.size;
		lodData = renderSystem->GetDevice()->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);

		auto cmd = renderSystem->GetCopyCommandBuffer();
		cmd->Commit(lodData, storage->GetAssetFile(GetReference()), GetBlobRange(*blob), 0);
	}
}

uint32_t Mesh::GetActiveLod() const
{
	return mActiveLod;
}

uint32_t Mesh::GetLodCount() const
{
	return static_cast<uint32_t>(mLods.size());
}

bool Mesh::IsLodResident(uint32_t lod) const
{
	if (lod >= mLods.size())
	{
		return false;
	}
	return mLods[lod].IsValid();
}

const Buffer& Mesh::GetBuffer() const
{
	return mLods[mActiveLod];
}

const BufferRange& Mesh::GetPositions() const
{
	return mDescriptor.lods[mActiveLod].positions;
}

const BufferRange& Mesh::GetInterleavedVertices() const
{
	return mDescriptor.lods[mActiveLod].interleavedVertices;
}

const BufferRange& Mesh::GetIndices() const
{
	return mDescriptor.lods[mActiveLod].indices;
}

const BufferRange& Mesh::GetMeshlets() const
{
	return mDescriptor.lods[mActiveLod].meshlets;
}

const BufferRange& Mesh::GetMeshletVertices() const
{
	return mDescriptor.lods[mActiveLod].meshletVertices;
}

const BufferRange& Mesh::GetMeshletTriangleIndices() const
{
	return mDescriptor.lods[mActiveLod].meshletTriangleIndices;
}

const TArray<SubmeshDescriptor>& Mesh::GetSubmeshes() const
{
	return mDescriptor.lods[mActiveLod].submeshes;
}

const SubmeshDescriptor& Mesh::GetSubmesh(uint32_t index) const
{
	return mDescriptor.lods[mActiveLod].submeshes[index];
}

const BottomLevelAccelerationStructure& Mesh::GetBLAS(uint32_t submesh) const
{
	return mBLASes[submesh];
}
