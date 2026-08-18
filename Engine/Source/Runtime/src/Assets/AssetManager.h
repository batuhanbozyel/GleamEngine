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

class AssetManager final : public GameInstanceSubsystem
{
public:
    
    virtual void Initialize(Application* app) override;

    virtual void Shutdown(Application* app) override;

	template<AssetType T>
	bool Has(const AssetReference& ref) const
	{
		return mAssetCache.find(ref) != mAssetCache.end();
	}

	template<AssetType T>
	T* Get(const AssetReference& ref) const
	{
		if (auto it = mAssetCache.find(ref); it != mAssetCache.end())
		{
			return static_cast<T*>(it->second.get());
		}
		GLEAM_ASSERT(false, "Asset is not loaded for GUID: {0}", ref.guid.ToString());
		return nullptr;
	}
	
	template<AssetType T>
	T* Load(const AssetReference& ref)
	{
		if (ref.guid == Guid::InvalidGuid())
		{
			return nullptr;
		}

		auto it = mAssetCache.find(ref);
		if (it == mAssetCache.end())
		{
			auto meta = entt::resolve(entt::type_hash<T>().value());
			auto any = meta.func("CreateAsset"_hs).invoke({}, ref);
			auto asset = any.template cast<T*>();
			it = mAssetCache.emplace_hint(mAssetCache.end(),
										  eastl::piecewise_construct,
										  eastl::forward_as_tuple(ref),
										  eastl::forward_as_tuple(asset));
		}

		++it->second->mRefCount;
		return static_cast<T*>(it->second.get());
	}

	void Release(const AssetReference& ref)
	{
		if (ref.guid == Guid::InvalidGuid())
		{
			return;
		}

		if (auto it = mAssetCache.find(ref); it != mAssetCache.end())
		{
			auto& asset = it->second;
			if (--asset->mRefCount == 0)
			{
				mAssetCache.erase(it);
			}
		}
	}
	
	template<typename T>
	T LoadDescriptor(const AssetReference& ref) const
	{
		T descriptor{};
		mStorage->ReadMetadata(ref, Reflection::GetClass<T>(), &descriptor);
		return descriptor;
	}

	template<AssetType T, typename Desc>
	static void RegisterMetaAsset()
	{
		entt::meta_factory<T>().template func<&CreateAsset<T, Desc>>("CreateAsset"_hs);
	}
	
	const Path& GetAssetPath(const AssetReference& ref) const;

	AssetStorage* GetStorage() const
	{
		return mStorage.get();
	}

private:

	template<AssetType T, typename Desc>
	static T* CreateAsset(const AssetReference& ref)
	{
		static auto instance = Globals::GameInstance->GetSubsystem<AssetManager>();
		return new T(ref, instance->LoadDescriptor<Desc>(ref));
	}

	HashMap<AssetReference, Scope<Asset>> mAssetCache;

	Scope<AssetStorage> mStorage;

};

} // namespace Gleam
