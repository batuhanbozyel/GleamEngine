#pragma once
#include "Asset.h"
#include "AssetReference.h"
#include "AssetStorage.h"

#include "Core/Globals.h"
#include "Core/Subsystem.h"
#include "Core/Application.h"

#include "IO/Log.h"
#include "IO/File.h"
#include "IO/Filesystem.h"

#include <entt/core/type_info.hpp>
#include <entt/meta/resolve.hpp>
#include <entt/meta/factory.hpp>

#ifndef __GLEAM_REFLECTION__
#include <Runtime.Reflection.generated.h>
#endif

namespace Gleam {

template <typename T>
concept AssetType = std::is_base_of<Asset, T>::value;

template <typename T>
concept LoadableAssetType = AssetType<T> and IsLoadableAsset<T>::value;

class AssetManager final : public TickableGameInstanceSubsystem
{
public:
    
    virtual void Initialize(Application* app) override;

    virtual void Shutdown(Application* app) override;

    virtual void Tick(Application* app) override;

	template<LoadableAssetType T>
	bool Has(const AssetReference& ref) const
	{
		return mAssetCache.find(ref) != mAssetCache.end();
	}

	template<LoadableAssetType T>
	T* Get(const AssetReference& ref) const
	{
		if (auto it = mAssetCache.find(ref); it != mAssetCache.end())
		{
			return static_cast<T*>(it->second.asset.get());
		}
		GLEAM_ASSERT(false, "Asset is not loaded for GUID: {0}", ref.guid.ToString());
		return nullptr;
	}
	
	template<LoadableAssetType T>
	T* Load(const AssetReference& ref)
	{
		if (ref.guid == Guid::InvalidGuid())
		{
			return nullptr;
		}

		auto it = mAssetCache.find(ref);
		if (it == mAssetCache.end())
		{
			const auto type = entt::type_hash<T>().value();
			auto asset = entt::resolve(type).func("ConstructAsset"_hs).invoke({}, static_cast<Asset*>(nullptr), ref).template cast<Asset*>();
			it = mAssetCache.emplace(ref, AssetEntry{ .asset = Scope<Asset>(asset), .type = type }).first;
		}

		++it->second.asset->mRefCount;
		return static_cast<T*>(it->second.asset.get());
	}

	void Release(const AssetReference& ref)
	{
		if (ref.guid == Guid::InvalidGuid())
		{
			return;
		}

		if (auto it = mAssetCache.find(ref); it != mAssetCache.end())
		{
			if (--it->second.asset->mRefCount == 0)
			{
				mPendingEvictions.push_back(ref);
			}
		}
	}

	void Reload(const AssetReference& ref);
	
	template<typename T>
	T LoadDescriptor(const AssetReference& ref) const
	{
		T descriptor{};
		mStorage->ReadAsset(ref, Reflection::GetClass<T>(), &descriptor);
		return descriptor;
	}

	template<LoadableAssetType T, typename Desc>
	static void RegisterMetaAsset()
	{
		entt::meta_factory<T>().template func<&ConstructAsset<T, Desc>>("ConstructAsset"_hs);
	}
	
	const Path& GetAssetPath(const AssetReference& ref) const;

	AssetStorage* GetStorage() const
	{
		return mStorage.get();
	}

private:

	struct AssetEntry
	{
		Scope<Asset> asset;
		entt::id_type type = 0;
	};

	template<LoadableAssetType T, typename Desc>
	static Asset* ConstructAsset(Asset* memory, const AssetReference& ref)
	{
		static auto instance = Globals::GameInstance->GetSubsystem<AssetManager>();

		Desc descriptor{};
		const auto header = instance->mStorage->ReadAsset(ref, Reflection::GetClass<Desc>(), &descriptor);
		return memory ? new (static_cast<void*>(memory)) T(ref, header, descriptor) : new T(ref, header, descriptor);
	}

	HashMap<AssetReference, AssetEntry> mAssetCache;

	Scope<AssetStorage> mStorage;

	TArray<AssetReference> mPendingReloads;

	TArray<AssetReference> mPendingEvictions;

};

} // namespace Gleam
