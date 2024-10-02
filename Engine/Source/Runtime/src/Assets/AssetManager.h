#pragma once
#include "Asset.h"
#include "AssetReference.h"
#include "Core/Subsystem.h"
#include "Serialization/JSONSerializer.h"

#include <mutex>

namespace Gleam {

struct Asset;

class AssetManager final : public GameInstanceSubsystem
{
public:
    
    virtual void Initialize(Application* app) override;

    virtual void Shutdown() override;

	template<typename T>
	T Get(const AssetReference& ref) const
	{
		auto it = mAssets.find(ref);
		if (it != mAssets.end())
		{
			auto fullpath = Globals::ProjectContentDirectory / it->second.path;
			auto file = Filesystem::Open(fullpath, FileType::Text);
			auto serializer = JSONSerializer(file.GetStream());
			auto asset = serializer.Deserialize<T>();
			return asset;
		}

		GLEAM_CORE_ERROR("Asset could not located for GUID: {0}", ref.guid.ToString());
		GLEAM_ASSERT(false);
		return T();
	}

	const Asset& GetAsset(const AssetReference& ref) const;

private:

	bool TryEmplaceAsset(const Asset& asset);

	std::mutex mMutex;
    
    HashMap<AssetReference, Asset> mAssets;

};

} // namespace Gleam
