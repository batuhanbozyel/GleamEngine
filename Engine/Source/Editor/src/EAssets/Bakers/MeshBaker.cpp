#include "MeshBaker.h"
#include "EAssets/AssetRegistry.h"
#include "EAssets/AssetWriter.h"

#include "Assets/Asset.h"

using namespace GEditor;

MeshBaker::MeshBaker(MeshData&& mesh)
	: mMesh(eastl::move(mesh))
{

}

void MeshBaker::Bake(const Gleam::Path& directory, const AssetItem& item) const
{
	Gleam::MeshDescriptor descriptor;
	descriptor.name = Name();
	descriptor.lods.resize(mMesh.lods.size());

	BinaryAssetWriter writer;
	for (uint32_t i = 0; i < mMesh.lods.size(); ++i)
	{
		const auto& lod = mMesh.lods[i];
		auto& lodDesc = descriptor.lods[i];

		lodDesc.indices = lod.indices;
		lodDesc.positions = lod.positions;
		lodDesc.interleavedVertices = lod.interleavedVertices;
		lodDesc.meshlets = lod.meshlets;
		lodDesc.meshletVertices = lod.meshletVertices;
		lodDesc.meshletTriangleIndices = lod.meshletTriangleIndices;
		lodDesc.submeshes = lod.submeshes;

		lodDesc.blobSlot = writer.AddBlob<Gleam::MeshLodDescriptor>(lod.buffer.data,
																	lod.buffer.size,
																	Gleam::AssetPlatform::Common,
																	Gleam::AssetBackend::Common);
	}
	writer.Write(directory, item, descriptor);
}

Gleam::TString MeshBaker::Name() const
{
	return mMesh.name;
}

Gleam::Guid MeshBaker::TypeGuid() const
{
	return Gleam::Reflection::GetClass<Gleam::MeshDescriptor>().Guid();
}
