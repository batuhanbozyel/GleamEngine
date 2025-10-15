#pragma once
#include "AssetPackage.h"
#include "AssetRegistry.h"

#include "IO/Path.h"
#include "World/WorldSubsystem.h"

namespace GEditor {

class EAssetManager final : public Gleam::WorldSubsystem
{
public:

	EAssetManager(const Gleam::Path& directory);

	virtual void Initialize(Gleam::World* world) override;

	virtual void Shutdown() override;

	void Import(const Gleam::Path& directory, const AssetPackage& package);

	const AssetItem& GetAsset(const Gleam::Guid& guid) const;

	template<typename T>
	const AssetItem& GetAsset(const Gleam::Path& path) const
	{
		return mRegistry.GetAsset<T>(path);
	}

private:

	AssetRegistry mRegistry;

	Gleam::Path mAssetDirectory;

};

} // namespace GEditor
