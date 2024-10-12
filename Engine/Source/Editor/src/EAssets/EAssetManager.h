#pragma once
#include "Gleam.h"
#include "AssetPackage.h"
#include "AssetRegistry.h"

namespace GEditor {

class EAssetManager final : public Gleam::WorldSubsystem
{
public:

	EAssetManager(const Gleam::Filesystem::Path& directory);

	virtual void Initialize(Gleam::World* world) override;

	virtual void Shutdown() override;

	void Import(const Gleam::Filesystem::Path& directory, const AssetPackage& package);

	const AssetItem& GetAsset(const Gleam::Guid& guid) const;

	template<typename T>
	const AssetItem& GetAsset(const Gleam::Filesystem::Path& path) const
	{
		return mRegistry.GetAsset<T>(path);
	}

private:

	AssetRegistry mRegistry;

	Gleam::Filesystem::Path mAssetDirectory;

};

} // namespace GEditor
