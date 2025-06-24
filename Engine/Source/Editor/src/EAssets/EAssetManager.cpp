#include "EAssetManager.h"
#include "AssetBaker.h"
#include "MaterialSource.h"

using namespace GEditor;

static Gleam::BinaryHeader ParseBinaryHeader(const Gleam::Path& asset)
{
	auto file = Gleam::Filesystem::Open(asset, Gleam::FileType::Binary);
	auto accessor = Gleam::Filesystem::ReadAccessor(asset);
	auto serializer = Gleam::BinarySerializer();
	return serializer.ParseHeader(file.GetStream());
}

static Gleam::JSONHeader ParseJSONHeader(const Gleam::Path& asset)
{
	auto file = Gleam::Filesystem::Open(asset, Gleam::FileType::Text);
	auto accessor = Gleam::Filesystem::ReadAccessor(asset);
	auto serializer = Gleam::JSONSerializer();
	return serializer.ParseHeader(file.GetStream());
}

static Gleam::TString ParseAssetName(const Gleam::Path& asset, const Gleam::Guid& typeGuid)
{
    if (typeGuid == Gleam::Reflection::GetClass<Gleam::MeshDescriptor>().Guid())
    {
		auto file = Gleam::Filesystem::Open(asset, Gleam::FileType::Binary);
		auto accessor = Gleam::Filesystem::ReadAccessor(asset);
		auto serializer = Gleam::BinarySerializer();
		
        auto descriptor = serializer.Deserialize<Gleam::MeshDescriptor>(file.GetStream());
        return descriptor.name;
    }
    
    if (typeGuid == Gleam::Reflection::GetClass<Gleam::TextureDescriptor>().Guid())
    {
		auto file = Gleam::Filesystem::Open(asset, Gleam::FileType::Binary);
		auto accessor = Gleam::Filesystem::ReadAccessor(asset);
		auto serializer = Gleam::BinarySerializer();
		
        auto descriptor = serializer.Deserialize<Gleam::TextureDescriptor>(file.GetStream());
        return descriptor.name;
    }
    
    if (typeGuid == Gleam::Reflection::GetClass<Gleam::MaterialDescriptor>().Guid())
    {
		auto file = Gleam::Filesystem::Open(asset, Gleam::FileType::Binary);
		auto accessor = Gleam::Filesystem::ReadAccessor(asset);
		auto serializer = Gleam::BinarySerializer();
		
        auto descriptor = serializer.Deserialize<Gleam::MaterialDescriptor>(file.GetStream());
        return descriptor.name;
    }
    
    if (typeGuid == Gleam::Reflection::GetClass<Gleam::MaterialInstanceDescriptor>().Guid())
    {
		auto file = Gleam::Filesystem::Open(asset, Gleam::FileType::Binary);
		auto accessor = Gleam::Filesystem::ReadAccessor(asset);
		auto serializer = Gleam::BinarySerializer();
		
        auto descriptor = serializer.Deserialize<Gleam::MaterialInstanceDescriptor>(file.GetStream());
        return descriptor.name;
    }

	if (typeGuid == Gleam::Reflection::GetClass<Gleam::Prefab>().Guid())
	{
		auto file = Gleam::Filesystem::Open(asset, Gleam::FileType::Text);
		auto accessor = Gleam::Filesystem::ReadAccessor(asset);
		auto serializer = Gleam::JSONSerializer();
		
		auto prefab = serializer.Deserialize<Gleam::Prefab>(file.GetStream());
		return prefab.name;
	}
	
	if (typeGuid == Gleam::Reflection::GetClass<Gleam::World>().Guid())
	{
		auto file = Gleam::Filesystem::Open(asset, Gleam::FileType::Text);
		auto accessor = Gleam::Filesystem::ReadAccessor(asset);
		auto serializer = Gleam::JSONSerializer();
		
		auto world = serializer.Deserialize<Gleam::World>(file.GetStream());
		return world.name;
	}
    
    return "";
}

EAssetManager::EAssetManager(const Gleam::Path& directory)
	: mRegistry(directory)
	, mAssetDirectory(directory)
{

}

void EAssetManager::Initialize(Gleam::World* world)
{
    Gleam::Filesystem::ForEach(mAssetDirectory, [this](const auto& entry)
    {
        if (entry.extension() == Gleam::Asset::Extension())
        {
            auto header = ParseBinaryHeader(entry);
            auto guid = Gleam::Guid(entry.stem().string());
            auto asset = Gleam::AssetReference{ .guid = guid };
            auto name = ParseAssetName(entry, header.guid);
            auto item = AssetItem{
                .reference = asset,
                .type = header.guid,
                .name = name
            };
			auto path = entry.parent_path() / name;
			mRegistry.RegisterAsset(path, item);
        }
		else if (entry.extension() == Gleam::Prefab::Extension())
		{
			auto typeGuid = Gleam::Reflection::GetClass<Gleam::Prefab>().Guid();
			auto guid = Gleam::Guid(entry.stem().string());
			auto asset = Gleam::AssetReference{ .guid = guid };
			auto name = ParseAssetName(entry, typeGuid);
			auto item = AssetItem{
				.reference = asset,
				.type = typeGuid,
				.name = name
			};
			auto path = entry.parent_path() / name;
			mRegistry.RegisterAsset(path, item);
		}
		else if (entry.extension() == Gleam::World::Extension())
		{
			auto typeGuid = Gleam::Reflection::GetClass<Gleam::World>().Guid();
			auto guid = Gleam::Guid(entry.stem().string());
			auto asset = Gleam::AssetReference{ .guid = guid };
			auto name = ParseAssetName(entry, typeGuid);
			auto item = AssetItem{
				.reference = asset,
				.type = typeGuid,
				.name = name
			};
			auto path = entry.parent_path() / name;
			mRegistry.RegisterAsset(path, item);
		}
    }, true);
    
    Gleam::Filesystem::ForEach(mAssetDirectory, [this](const auto& entry)
    {
        if (entry.extension() == ".mat")
        {
            auto path = entry.parent_path()/entry.stem();
			const auto& item = mRegistry.GetAsset<Gleam::MaterialDescriptor>(path);
            if (item.reference.guid == Gleam::Guid::InvalidGuid())
            {
				auto assetRegistry = AssetRegistry(entry.parent_path());
                auto materialSource = MaterialSource(this, &assetRegistry);
                auto settings = MaterialSource::ImportSettings();
                materialSource.Import(entry, settings);
                Import(mAssetDirectory / "Materials", materialSource);
            }
			else
			{
				// TODO: reimport only if material/shader source changed since last compile
				auto assetRegistry = AssetRegistry(entry.parent_path());
				assetRegistry.RegisterAsset(entry.stem(), item);

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

void EAssetManager::Shutdown()
{
	
}

void EAssetManager::Import(const Gleam::Path& directory, const AssetPackage& package)
{
	for (const auto& baker : package.mBakers)
	{
		auto path = directory / baker->Filename();
		const auto& item = package.mRegistry->GetAsset(baker->Filename(), baker->TypeGuid());
		const auto& asset = mRegistry.RegisterAsset(path, item);
		baker->Bake(directory, asset);
	}
}

const AssetItem& EAssetManager::GetAsset(const Gleam::Guid& guid) const
{
	return mRegistry.GetAsset(guid);
}
