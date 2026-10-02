#include "EAssetManager.h"
#include "AssetBaker.h"
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

			auto world = serializer.Deserialize<Gleam::World>(file->GetStream());
			auto guid = Gleam::Guid(entry.Stem());
			auto asset = Gleam::AssetReference{ .guid = guid };
			auto item = AssetItem{
				.reference = asset,
				.type = Gleam::Reflection::GetClass<Gleam::World>().Guid(),
				.name = world.name
			};
			auto path = entry.Parent() / world.name;
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
