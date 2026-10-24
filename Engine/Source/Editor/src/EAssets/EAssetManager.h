#pragma once
#include "AssetPackage.h"
#include "AssetRegistry.h"

#include "IO/Path.h"
#include "Core/Subsystem.h"
#include "Container/Pointer.h"

namespace GEditor {

class EAssetManager final : public Gleam::GameInstanceSubsystem
{
public:

	EAssetManager(const Gleam::Path& directory);

	virtual void Initialize(Gleam::Application* app) override;

	virtual void Shutdown(Gleam::Application* app) override;

	void Import(const Gleam::Path& directory, const AssetPackage& package);

	const AssetItem& GetAsset(const Gleam::Guid& guid) const;

	const AssetItem* FindAsset(const Gleam::Guid& guid) const;

	template<typename Function>
	void ForEachAsset(Function&& function) const
	{
		mRegistry.ForEachAsset(eastl::forward<Function>(function));
	}

	template<typename T>
	const AssetItem& CreateAsset(const Gleam::Path& directory, Gleam::TStringView name, const T& descriptor)
	{
		return CreateAsset(directory, name, &descriptor, Gleam::Reflection::GetClass<T>());
	}

	const AssetItem& CreateAsset(const Gleam::Path& directory, Gleam::TStringView name, const void* descriptor, const Gleam::Reflection::ClassDescription& classDesc);

	template<typename T>
	bool SaveAsset(const Gleam::Path& directory, const T& descriptor, AssetItem& item)
	{
		return SaveAsset(directory, &descriptor, Gleam::Reflection::GetClass<T>(), item);
	}

	bool SaveAsset(const Gleam::Path& directory, const void* descriptor, const Gleam::Reflection::ClassDescription& classDesc, AssetItem& item);

	template<typename T>
	bool LoadAsset(const Gleam::Path& file, T& descriptor) const
	{
		return LoadAsset(file, &descriptor, Gleam::Reflection::GetClass<T>());
	}

	bool LoadAsset(const Gleam::Path& file, void* descriptor, const Gleam::Reflection::ClassDescription& classDesc) const;

	bool IsValidAssetName(const Gleam::Path& directory, const AssetItem& item, Gleam::TStringView name) const;

	bool RenameAsset(const Gleam::Path& file, AssetItem& item, Gleam::TStringView name);

	bool RemoveAsset(const Gleam::Path& file, const AssetItem& item);

	template<typename T>
	const AssetItem& GetAsset(const Gleam::Path& path) const
	{
		return mRegistry.GetAsset<T>(path);
	}

private:

	void CompileMaterials(const Gleam::Path& sourceDirectory, const Gleam::Path& outputDirectory);

	void CompileMaterial(const Gleam::Path& source, const Gleam::Path& outputDirectory);

	AssetRegistry mRegistry;

	Gleam::Path mAssetDirectory;

};

} // namespace GEditor
