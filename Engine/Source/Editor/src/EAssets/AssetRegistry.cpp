#include "Gleam.h"
#include "AssetRegistry.h"
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
    
    return "";
}

AssetRegistry::AssetRegistry(const Gleam::Filesystem::Path& directory)
	: mAssetDirectory(directory)
{

}

void AssetRegistry::Initialize(Gleam::World* world)
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
            auto relPath = Gleam::Filesystem::Relative(entry.parent_path(), mAssetDirectory);
            relPath /= name;
			auto& items = mAssetCache[relPath];
			items.push_back(item);
        }
    }, true);
    
    // TODO: reimport if material/shader source changed since last compile
    Gleam::Filesystem::ForEach(mAssetDirectory, [this](const auto& entry)
    {
        if (entry.extension() == ".mat")
        {
            auto path = entry.parent_path()/entry.stem();
            auto relPath = Gleam::Filesystem::Relative(path, mAssetDirectory);
            if (mAssetCache.find(relPath) == mAssetCache.end())
            {
                auto materialSource = MaterialSource(this);
                auto settings = MaterialSource::ImportSettings();
                materialSource.Import(entry, settings);
                Import(mAssetDirectory/"Materials", materialSource);
            }
        }
    }, true);
}

void AssetRegistry::Shutdown()
{
	mAssetCache.clear();
}

void AssetRegistry::Import(const Gleam::Filesystem::Path& directory, const AssetPackage& package)
{
	for (const auto& baker : package.bakers)
	{
		auto path = directory / baker->Filename();
		const auto& asset = RegisterAsset(path, baker->TypeGuid());

		auto filename = asset.reference.guid.ToString() + Gleam::Asset::extension().data();
		auto file = Gleam::Filesystem::Create(directory / filename, Gleam::FileType::Text);
		auto accessor = Gleam::Filesystem::WriteAccessor(directory / filename);
		baker->Bake(file.GetStream());
	}
}

const AssetItem& AssetRegistry::RegisterAsset(const Gleam::Filesystem::Path& path, const Gleam::Guid& type)
{
	auto relPath = path.is_relative() ? path : Gleam::Filesystem::Relative(path, mAssetDirectory);
	auto& items = mAssetCache[relPath];
	for (const auto& item : items)
	{
		if (item.type == type)
		{
			return item;
		}
	}

	auto asset = Gleam::AssetReference{ .guid = Gleam::Guid::NewGuid() };
	auto item = AssetItem{
		.reference = asset,
		.type = type,
		.name = path.stem().string()
	};
	GLEAM_INFO("Asset imported: {0} GUID: {1}", item.name, asset.guid.ToString());
	return items.emplace_back(item);
}

const AssetItem& AssetRegistry::GetAsset(const Gleam::Guid& guid) const
{
	for (const auto& [path, items] : mAssetCache)
	{
		for (const auto& item : items)
		{
			if (item.reference.guid == guid)
			{
				return item;
			}
		}
	}

	GLEAM_CORE_ERROR("Asset could not located for GUID: {0}", guid.ToString());
	GLEAM_ASSERT(false);
	static AssetItem invalidAsset;
	return invalidAsset;
}
