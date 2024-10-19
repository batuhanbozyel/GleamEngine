#pragma once
#include "Asset.h"
#include "AssetReference.h"

#include "Core/Globals.h"
#include "Core/Subsystem.h"
#include "Core/Application.h"

#include "Serialization/JSONSerializer.h"

#include <mutex>

namespace Gleam {

template <typename T>
concept AssetType = std::is_base_of<Asset, T>::value;

class AssetManager final : public GameInstanceSubsystem
{
public:
    
    virtual void Initialize(Application* app) override;

    virtual void Shutdown() override;
	
	template<AssetType T>
	const T* Load(const AssetReference& ref)
	{
		if (auto it = mAssetCache.find(ref); it != mAssetCache.end())
		{
			return static_cast<const T*>(it->second.get());
		}
		
		constexpr auto typeHash = entt::type_hash<T>::value();
		auto meta = entt::resolve(static_cast<uint32_t>(typeHash));
		auto asset = Reflection::Get<T*>(meta.func("CreateAsset"_hs).invoke({}, ref).data());
		auto it = mAssetCache.emplace_hint(mAssetCache.end(),
										   std::piecewise_construct,
										   std::forward_as_tuple(ref),
										   std::forward_as_tuple(asset));
		return static_cast<const T*>(it->second.get());
	}
	
	template<typename T>
	T LoadDescriptor(const AssetReference& ref) const
	{
		auto it = mAssetPaths.find(ref);
		if (it != mAssetPaths.end())
		{
			auto fullpath = Globals::ProjectContentDirectory / it->second;
			auto file = Filesystem::Open(fullpath, FileType::Text);
			auto serializer = JSONSerializer(file.GetStream());
			auto asset = serializer.Deserialize<T>();
			return asset;
		}

		GLEAM_CORE_ERROR("Asset could not located for GUID: {0}", ref.guid.ToString());
		GLEAM_ASSERT(false);
		return T();
	}
	
	template<AssetType T, typename Desc>
	static void RegisterMetaAsset()
	{
		[[maybe_unused]] const auto& classDesc = Reflection::GetClass<Desc>();
		entt::meta<T>()
			.type(entt::type_hash<T>::value())
			.template func<&CreateAsset<T, Desc>>("CreateAsset"_hs);
	}
	
	const Filesystem::Path& GetAssetPath(const AssetReference& ref) const;

private:
	
	template<AssetType T, typename Desc>
	static T* CreateAsset(const AssetReference& ref)
	{
		auto instance = Globals::GameInstance->GetSubsystem<AssetManager>();
		return new T(instance->LoadDescriptor<Desc>(ref));
	}

	void EmplaceAssetPath(const Filesystem::Path& path);

	std::mutex mMutex;
	
	HashMap<AssetReference, Scope<Asset>> mAssetCache;
    
    HashMap<AssetReference, Filesystem::Path> mAssetPaths;

};

} // namespace Gleam
