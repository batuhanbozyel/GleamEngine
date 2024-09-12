#include "Gleam.h"
#include "AssetRegistry.h"
#include "AssetBaker.h"
#include "MaterialSource.h"

using namespace GEditor;

static Gleam::TString ParseNameFromAssetFile(Gleam::JSONSerializer& serializer, const Gleam::Guid& typeGuid)
{
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
    Gleam::Filesystem::ForEach(directory, [this](const auto& entry)
    {
        if (entry.extension() == ".asset")
        {
            auto file = Gleam::Filesystem::Open(entry, Gleam::FileType::Text);
			auto accessor = Gleam::Filesystem::ReadAccessor(entry);
            auto serializer = Gleam::JSONSerializer(file.GetStream());
            auto header = serializer.ParseHeader();
            
            auto guid = Gleam::Guid(entry.stem().string());
            auto asset = Gleam::AssetReference{ .guid = guid };
            auto name = ParseNameFromAssetFile(serializer, header.guid);
            auto item = AssetItem{
                .reference = asset,
                .type = header.guid,
                .name = name
            };
            auto relPath = Gleam::Filesystem::Relative(entry.parent_path(), mAssetDirectory);
            relPath /= name;
            mAssetCache.insert({ relPath, item });
        }
    }, true);
    
    // TODO: materials should only compile and imported when first seen by the registry
    // existing materials should not change guid
    // it should only reimport if material/shader source changed since last compile
    Gleam::Filesystem::ForEach(directory, [this](const auto& entry)
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

void AssetRegistry::Import(const Gleam::Filesystem::Path& directory, const AssetPackage& package)
{
	for (const auto& baker : package.bakers)
	{
		auto path = directory / baker->Filename();
		const auto& asset = RegisterAsset(path, baker->TypeGuid());

		auto filename = asset.guid.ToString() + Gleam::Asset::extension().data();
		auto file = Gleam::Filesystem::Create(directory / filename, Gleam::FileType::Text);
		auto accessor = Gleam::Filesystem::WriteAccessor(directory / filename);
		baker->Bake(file.GetStream());
	}
}

const Gleam::AssetReference& AssetRegistry::RegisterAsset(const Gleam::Filesystem::Path& path, const Gleam::Guid& type)
{
	auto relPath = path.is_relative() ? path : Gleam::Filesystem::Relative(path, mAssetDirectory);
	auto it = mAssetCache.find(relPath);
	if (it != mAssetCache.end())
	{
		return it->second.reference;
	}

	auto asset = Gleam::AssetReference{ .guid = Gleam::Guid::NewGuid() };
	auto item = AssetItem{
		.reference = asset,
		.type = type,
		.name = path.stem().string()
	};

	it = mAssetCache.emplace_hint(mAssetCache.end(), relPath, item);
	GLEAM_INFO("Asset imported: {0} GUID: {1}", item.name, asset.guid.ToString());
	return it->second.reference;
}

const Gleam::AssetReference& AssetRegistry::GetAsset(const Gleam::Filesystem::Path& path) const
{
	auto relPath = path.is_relative() ? path : Gleam::Filesystem::Relative(path, mAssetDirectory);
	auto it = mAssetCache.find(relPath);
	if (it != mAssetCache.end())
	{
		return it->second.reference;
	}
	static Gleam::AssetReference invalidAsset;
	return invalidAsset;
}
