#include "EAssetManager.h"
#include "AssetBaker.h"
#include "AssetWriter.h"
#include "MaterialSource.h"

#include "Core/Globals.h"
#include "Core/Engine.h"
#include "Core/Application.h"

#include "Serialization/BinarySerializer.h"
#include "Serialization/JSONSerializer.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/MeshDescriptor.h"
#include "Renderer/TextureDescriptor.h"
#include "Renderer/Material/MaterialDescriptor.h"

#include "Assets/Asset.h"
#include "Assets/AssetManager.h"
#include "Assets/AssetHeader.h"

#include "World/Prefab.h"
#include "World/World.h"

using namespace GEditor;

EAssetManager::EAssetManager(const Gleam::Path& directory)
	: mRegistry(directory)
	, mAssetDirectory(directory)
{

}

void EAssetManager::Initialize(Gleam::Application* app)
{
    Gleam::Filesystem::ForEach(mAssetDirectory, [this](const auto& entry)
    {
        if (entry.Extension() == Gleam::Asset::Extension())
        {
			auto file = Gleam::Filesystem::OpenRead(entry, Gleam::FileType::Binary);
			auto serializer = Gleam::BinarySerializer();

			auto header = serializer.Deserialize<Gleam::AssetHeader>(file->GetStream());
            auto guid = Gleam::Guid(entry.Stem());
            auto asset = Gleam::AssetReference{ .guid = guid };
            auto item = AssetItem{
                .reference = asset,
                .type = header.typeGuid,
                .name = header.name
            };
            auto path = entry.Parent() / header.name;
            mRegistry.RegisterAsset(path, item);
        }
		else if (entry.Extension() == Gleam::Prefab::Extension())
		{
			auto file = Gleam::Filesystem::OpenRead(entry, Gleam::FileType::Text);
			auto serializer = Gleam::JSONSerializer();

			auto prefab = serializer.Deserialize<Gleam::Prefab>(file->GetStream());
			auto guid = Gleam::Guid(entry.Stem());
			auto asset = Gleam::AssetReference{ .guid = guid };
			auto item = AssetItem{
				.reference = asset,
				.type = Gleam::Reflection::GetClass<Gleam::Prefab>().Guid(),
				.name = prefab.name
			};
			auto path = entry.Parent() / prefab.name;
			mRegistry.RegisterAsset(path, item);
		}
		else if (entry.Extension() == Gleam::World::Extension())
		{
			auto file = Gleam::Filesystem::OpenRead(entry, Gleam::FileType::Text);
			auto serializer = Gleam::JSONSerializer();

			auto header = serializer.Deserialize<Gleam::AssetHeader>(file->GetStream());
			auto guid = Gleam::Guid(entry.Stem());
			auto asset = Gleam::AssetReference{ .guid = guid };
			auto item = AssetItem{
				.reference = asset,
				.type = header.typeGuid,
				.name = header.name
			};
			auto path = entry.Parent() / header.name;
			mRegistry.RegisterAsset(path, item);
		}
    }, true);
    
	Gleam::Filesystem::ForEach(mAssetDirectory, [&](const auto& entry)
	{
		if (entry.Extension() == L".mat")
		{
			CompileMaterial(entry, mAssetDirectory / "Materials");
		}
	}, true);

	Gleam::Filesystem::ForEach(Gleam::Globals::BuiltinAssetsDirectory, [&](const auto& entry)
	{
		if (entry.Extension() == L".mat")
		{
			CompileMaterial(entry, mAssetDirectory / "Engine" / "Materials");
		}
	}, true);
}

void EAssetManager::CompileMaterials(const Gleam::Path& sourceDirectory, const Gleam::Path& outputDirectory)
{
	Gleam::Filesystem::ForEach(sourceDirectory, [&](const auto& entry)
	{
		if (entry.Extension() == L".mat")
		{
			CompileMaterial(entry, outputDirectory);
		}
	}, false);
}

void EAssetManager::CompileMaterial(const Gleam::Path& source, const Gleam::Path& outputDirectory)
{
	Gleam::Filesystem::CreateDirectories(outputDirectory);

	const auto& item = mRegistry.GetAsset<Gleam::MaterialDescriptor>(outputDirectory / source.Stem());
	auto assetRegistry = AssetRegistry(source.Parent());
	if (item.reference.guid != Gleam::Guid::InvalidGuid())
	{
		assetRegistry.RegisterAsset(source.Stem(), item);
	}

	auto materialSource = MaterialSource(this, &assetRegistry);
	auto settings = MaterialSource::ImportSettings();
	materialSource.Import(source, settings);
	Import(outputDirectory, materialSource);

	if (item.reference.guid != Gleam::Guid::InvalidGuid())
	{
		auto assetManager = Gleam::Globals::GameInstance->GetSubsystem<Gleam::AssetManager>();
		auto material = assetManager->LoadDescriptor<Gleam::MaterialDescriptor>(item.reference);

		auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
		renderSystem->RecompileShader(material.surfaceShader);
	}
}

void EAssetManager::Shutdown(Gleam::Application* app)
{
	
}

void EAssetManager::Import(const Gleam::Path& directory, const AssetPackage& package)
{
	for (const auto& baker : package.mBakers)
	{
		auto path = directory / baker->Name();

		const auto existing = mRegistry.FindAsset(path, baker->TypeGuid());
		const auto& item = (existing != nullptr) ? *existing
										  : package.mRegistry->GetAsset(baker->Name(), baker->TypeGuid());

		const auto& asset = mRegistry.RegisterAsset(path, item);
		baker->Bake(directory, asset);
	}
}

const AssetItem& EAssetManager::GetAsset(const Gleam::Guid& guid) const
{
	return mRegistry.GetAsset(guid);
}

const AssetItem* EAssetManager::FindAsset(const Gleam::Guid& guid) const
{
	return mRegistry.FindAsset(guid);
}

const AssetItem& EAssetManager::CreateAsset(const Gleam::Path& directory, Gleam::TStringView name, const void* descriptor, const Gleam::Reflection::ClassDescription& classDesc)
{
	auto item = AssetItem{
		.reference = Gleam::AssetReference{ .guid = Gleam::Guid::NewGuid() },
		.type = classDesc.Guid(),
		.name = Gleam::TString(name)
	};
	for (uint32_t suffix = 1; mRegistry.FindAsset(directory / item.name, item.type) != nullptr; ++suffix)
	{
		item.name = name;
		item.name.append(" ");
		item.name.append(std::to_string(suffix).c_str());
	}

	BinaryAssetWriter writer;
	writer.Write(directory, item, descriptor, classDesc);

	mRegistry.UnregisterAsset(item.reference.guid);
	return mRegistry.RegisterAsset(directory / item.name, item);
}

bool EAssetManager::SaveAsset(const Gleam::Path& directory, const void* descriptor, const Gleam::Reflection::ClassDescription& classDesc, AssetItem& item)
{
	if (not IsValidAssetName(directory, item, item.name))
	{
		return false;
	}

	BinaryAssetWriter writer;
	writer.Write(directory, item, descriptor, classDesc);

	mRegistry.UnregisterAsset(item.reference.guid);
	item = mRegistry.RegisterAsset(directory / item.name, item);
	return true;
}

bool EAssetManager::LoadAsset(const Gleam::Path& file, void* descriptor, const Gleam::Reflection::ClassDescription& classDesc) const
{
	auto stream = Gleam::Filesystem::OpenRead(file, Gleam::FileType::Binary);
	if (not stream->IsOpen())
	{
		GLEAM_ERROR("Asset could not be opened: {0}", file.String());
		return false;
	}

	auto serializer = Gleam::BinarySerializer();
	const auto header = serializer.Deserialize<Gleam::AssetHeader>(stream->GetStream());
	stream->GetStream().seekg(header.metadata.offset);
	serializer.Deserialize(stream->GetStream(), classDesc, descriptor);
	return true;
}

bool EAssetManager::IsValidAssetName(const Gleam::Path& directory, const AssetItem& item, Gleam::TStringView name) const
{
	if (name.empty() or name.find_first_of("/\\") != Gleam::TStringView::npos)
	{
		return false;
	}

	const auto existing = mRegistry.FindAsset(directory / name, item.type);
	return existing == nullptr or existing->reference == item.reference;
}

bool EAssetManager::RenameAsset(const Gleam::Path& file, AssetItem& item, Gleam::TStringView name)
{
	if (not IsValidAssetName(file.Parent(), item, name))
	{
		return false;
	}

	auto serializer = Gleam::BinarySerializer();
	Gleam::AssetHeader header;
	Gleam::BinaryBuffer payload;
	{
		auto input = Gleam::Filesystem::OpenRead(file, Gleam::FileType::Binary);
		auto& stream = input->GetStream();
		header = serializer.Deserialize<Gleam::AssetHeader>(stream);
		payload = Gleam::BinaryBuffer(header.metadata.size + header.bulkData.size);
		stream.seekg(header.metadata.offset);
		stream.read(static_cast<char*>(payload.data), payload.size);
	}

	header.name = name;
	auto output = Gleam::Filesystem::Create(file, Gleam::FileType::Binary);
	auto& stream = output->GetStream();
	serializer.Serialize(header, stream);
	header.metadata.offset = static_cast<uint64_t>(stream.tellp());
	header.bulkData.offset = header.metadata.offset + header.metadata.size;
	stream.write(static_cast<const char*>(payload.data), payload.size);
	stream.seekp(0);
	serializer.Serialize(header, stream);

	item.name = name;
	mRegistry.UnregisterAsset(item.reference.guid);
	mRegistry.RegisterAsset(file.Parent() / item.name, item);
	return true;
}

bool EAssetManager::RemoveAsset(const Gleam::Path& file, const AssetItem& item)
{
	if (Gleam::Filesystem::Remove(file) == false)
	{
		return false;
	}

	mRegistry.UnregisterAsset(item.reference.guid);
	return true;
}
