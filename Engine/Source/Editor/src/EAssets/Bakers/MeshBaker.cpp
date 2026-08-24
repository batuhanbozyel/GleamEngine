#include "MeshBaker.h"
#include "EAssets/AssetRegistry.h"
#include "EAssets/AssetWriter.h"

#include "Assets/Asset.h"

using namespace GEditor;

MeshBaker::MeshBaker(MeshLodData&& lod)
{
	mLods.emplace_back(std::move(lod));
}

void MeshBaker::Bake(const Gleam::Path& directory, const AssetItem& item) const
{
	Gleam::MeshDescriptor descriptor;
	descriptor.name = Name();
	descriptor.lods.resize(mLods.size());

	BinaryAssetWriter writer;
	for (uint32_t i = 0; i < mLods.size(); ++i)
	{
		const auto& lod = mLods[i];
		auto& lodDesc = descriptor.lods[i];
		
		uint64_t totalSize = lod.indices.size() * sizeof(uint32_t)
							+ lod.positions.size() * sizeof(Gleam::Float3)
							+ lod.interleavedVertices.size() * sizeof(Gleam::InterleavedMeshVertex)
							+ lod.meshlets.size() * sizeof(Gleam::MeshletDescriptor)
							+ lod.meshletVertices.size() * sizeof(uint32_t)
							+ lod.meshletTriangleIndices.size() * sizeof(uint32_t);
		
		Gleam::BinaryWriter binaryWriter(totalSize);
		lodDesc.indices = binaryWriter.Write(lod.indices.data(), lod.indices.size() * sizeof(uint32_t));
		lodDesc.positions = binaryWriter.Write(lod.positions.data(), lod.positions.size() * sizeof(Gleam::Float3));
		lodDesc.interleavedVertices = binaryWriter.Write(lod.interleavedVertices.data(), lod.interleavedVertices.size() * sizeof(Gleam::InterleavedMeshVertex));
		lodDesc.meshlets = binaryWriter.Write(lod.meshlets.data(), lod.meshlets.size() * sizeof(Gleam::MeshletDescriptor));
		lodDesc.meshletVertices = binaryWriter.Write(lod.meshletVertices.data(), lod.meshletVertices.size() * sizeof(uint32_t));
		lodDesc.meshletTriangleIndices = binaryWriter.Write(lod.meshletTriangleIndices.data(), lod.meshletTriangleIndices.size() * sizeof(uint32_t));

		const auto& buffer = binaryWriter.GetBuffer();
		lodDesc.blobSlot = writer.AddBlob(buffer.data,
										  buffer.size,
										  Gleam::MeshLodAssetLayout::Hash(),
										  Gleam::AssetPlatform::Common,
										  Gleam::AssetBackend::Common);
		
		lodDesc.submeshes = lod.submeshes;
	}
	writer.Write(directory, item, descriptor);
}

Gleam::TString MeshBaker::Name() const
{
	return mLods.empty() ? Gleam::TString() : mLods[0].name;
}

Gleam::Guid MeshBaker::TypeGuid() const
{
	return Gleam::Reflection::GetClass<Gleam::MeshDescriptor>().Guid();
}
