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

struct AssetContainerInfo
{
	Gleam::AssetHeader header;
	Gleam::TString name;
};

static AssetContainerInfo ParseAssetContainer(const Gleam::Path& asset)
{
	AssetContainerInfo info;

	auto file = Gleam::Filesystem::OpenRead(asset, Gleam::FileType::Binary);
	auto& stream = file->GetStream();

	auto serializer = Gleam::BinarySerializer();
	info.header = serializer.Deserialize<Gleam::AssetHeader>(stream);

	info.name.resize(static_cast<size_t>(info.header.name.size));
	stream.seekg(static_cast<std::streamoff>(info.header.name.offset));
	stream.read(info.name.data(), static_cast<std::streamsize>(info.header.name.size));
	return info;
}

static Gleam::TString ParseAssetName(const Gleam::Path& asset, const Gleam::Guid& typeGuid)
{
	if (typeGuid == Gleam::Reflection::GetClass<Gleam::Prefab>().Guid())
	{
		auto file = Gleam::Filesystem::OpenRead(asset, Gleam::FileType::Text);
		auto serializer = Gleam::JSONSerializer();
		
		auto prefab = serializer.Deserialize<Gleam::Prefab>(file->GetStream());
		return prefab.name;
	}
	
	if (typeGuid == Gleam::Reflection::GetClass<Gleam::World>().Guid())
	{
		auto file = Gleam::Filesystem::OpenRead(asset, Gleam::FileType::Text);
		auto serializer = Gleam::JSONSerializer();
		
		auto world = serializer.Deserialize<Gleam::World>(file->GetStream());
		return world.name;
	}
    
    return "";
}

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
            auto info = ParseAssetContainer(entry);
            auto guid = Gleam::Guid(entry.Stem());
            auto asset = Gleam::AssetReference{ .guid = guid };
            auto item = AssetItem{
                .reference = asset,
                .type = info.header.typeGuid,
                .name = info.name
            };
            auto path = entry.Parent() / info.name;
            mRegistry.RegisterAsset(path, item);
        }
		else if (entry.Extension() == Gleam::Prefab::Extension())
		{
			auto typeGuid = Gleam::Reflection::GetClass<Gleam::Prefab>().Guid();
			auto guid = Gleam::Guid(entry.Stem());
			auto asset = Gleam::AssetReference{ .guid = guid };
			auto name = ParseAssetName(entry, typeGuid);
			auto item = AssetItem{
				.reference = asset,
				.type = typeGuid,
				.name = name
			};
			auto path = entry.Parent() / name;
			mRegistry.RegisterAsset(path, item);
		}
		else if (entry.Extension() == Gleam::World::Extension())
		{
			auto typeGuid = Gleam::Reflection::GetClass<Gleam::World>().Guid();
			auto guid = Gleam::Guid(entry.Stem());
			auto asset = Gleam::AssetReference{ .guid = guid };
			auto name = ParseAssetName(entry, typeGuid);
			auto item = AssetItem{
				.reference = asset,
				.type = typeGuid,
				.name = name
			};
			auto path = entry.Parent() / name;
			mRegistry.RegisterAsset(path, item);
		}
    }, true);
    
    Gleam::Filesystem::ForEach(mAssetDirectory, [this](const auto& entry)
    {
        if (entry.Extension() == L".mat")
        {
            auto path = entry.Parent()/entry.Stem();
			const auto& item = mRegistry.GetAsset<Gleam::MaterialDescriptor>(path);
            if (item.reference.guid == Gleam::Guid::InvalidGuid())
            {
				auto assetRegistry = AssetRegistry(entry.Parent());
                auto materialSource = MaterialSource(this, &assetRegistry);
                auto settings = MaterialSource::ImportSettings();
                materialSource.Import(entry, settings);
                Import(mAssetDirectory / "Materials", materialSource);
            }
			else
			{
				// TODO: reimport only if material/shader source changed since last compile
				auto assetRegistry = AssetRegistry(entry.Parent());
				assetRegistry.RegisterAsset(entry.Stem(), item);

				auto materialSource = MaterialSource(this, &assetRegistry);
				auto settings = MaterialSource::ImportSettings();
				materialSource.Import(entry, settings);
				Import(mAssetDirectory / "Materials", materialSource);

				auto assetManager = Gleam::Globals::GameInstance->GetSubsystem<Gleam::AssetManager>();
				auto material = assetManager->LoadDescriptor<Gleam::MaterialDescriptor>(item.reference);

				auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
				renderSystem->RecompileShader(material.surfaceShader);
			}
        }
    }, true);
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
