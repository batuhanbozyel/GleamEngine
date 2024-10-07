#include "EAssetManager.h"
#include "AssetBaker.h"
#include "MaterialSource.h"

using namespace GEditor;

static Gleam::JSONHeader ParseAssetHeader(const Gleam::Filesystem::Path& asset)
{
	auto file = Gleam::Filesystem::Open(asset, Gleam::FileType::Text);
	auto accessor = Gleam::Filesystem::ReadAccessor(asset);
	auto serializer = Gleam::JSONSerializer(file.GetStream());
	return serializer.ParseHeader();
}

static Gleam::TString ParseAssetName(const Gleam::Filesystem::Path& asset, const Gleam::Guid& typeGuid)
{
	auto file = Gleam::Filesystem::Open(asset, Gleam::FileType::Text);
	auto accessor = Gleam::Filesystem::ReadAccessor(asset);
	auto serializer = Gleam::JSONSerializer(file.GetStream());

    if (typeGuid == Gleam::Reflection::GetClass<Gleam::MeshDescriptor>().Guid())
    {
        auto descriptor = serializer.Deserialize<Gleam::MeshDescriptor>();
        return descriptor.name;
    }
    
    if (typeGuid == Gleam::Reflection::GetClass<Gleam::TextureDescriptor>().Guid())
    {
        auto descriptor = serializer.Deserialize<Gleam::TextureDescriptor>();
        return descriptor.name;
    }
    
    if (typeGuid == Gleam::Reflection::GetClass<Gleam::MaterialDescriptor>().Guid())
    {
        auto descriptor = serializer.Deserialize<Gleam::MaterialDescriptor>();
        return descriptor.name;
    }
    
    if (typeGuid == Gleam::Reflection::GetClass<Gleam::MaterialInstanceDescriptor>().Guid())
    {
        auto descriptor = serializer.Deserialize<Gleam::MaterialInstanceDescriptor>();
        return descriptor.name;
    }

	if (typeGuid == Gleam::Reflection::GetClass<Gleam::Prefab>().Guid())
	{
		auto prefab = serializer.Deserialize<Gleam::Prefab>();
		return prefab.name;
	}
    
    return "";
}

EAssetManager::EAssetManager(const Gleam::Filesystem::Path& directory)
	: mRegistry(directory)
	, mAssetDirectory(directory)
{

}

void EAssetManager::Initialize(Gleam::World* world)
{
    Gleam::Filesystem::ForEach(mAssetDirectory, [this](const auto& entry)
    {
        if (entry.extension() == ".asset")
        {
            auto header = ParseAssetHeader(entry);
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
    }, true);
    
    // TODO: reimport if material/shader source changed since last compile
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
                Import(mAssetDirectory/"Materials", materialSource);
            }
        }
    }, true);
}

void EAssetManager::Shutdown()
{
	
}

void EAssetManager::Import(const Gleam::Filesystem::Path& directory, const AssetPackage& package)
{
	for (const auto& baker : package.mBakers)
	{
		auto path = directory / baker->Filename();
		const auto& item = package.mRegistry->GetAsset(baker->Filename(), baker->TypeGuid());
		const auto& asset = mRegistry.RegisterAsset(path, item);

		auto filename = asset.reference.guid.ToString() + Gleam::Asset::Extension().data();
		auto file = Gleam::Filesystem::Create(directory / filename, Gleam::FileType::Text);
		auto accessor = Gleam::Filesystem::WriteAccessor(directory / filename);
		baker->Bake(file.GetStream());
	}
}

const AssetItem& EAssetManager::GetAsset(const Gleam::Guid& guid) const
{
	return mRegistry.GetAsset(guid);
}