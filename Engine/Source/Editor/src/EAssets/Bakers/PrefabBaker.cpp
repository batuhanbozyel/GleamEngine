#include "PrefabBaker.h"
#include "EAssets/AssetRegistry.h"

#include "Gleam.h"

using namespace GEditor;

PrefabBaker::PrefabBaker(const Gleam::RefCounted<Gleam::World>& world)
	: mWorld(world)
{
	
}

void PrefabBaker::Bake(const Gleam::Path& directory, const AssetItem& item) const
{
	auto filename = Gleam::TWString(item.reference.guid.ToString()) + Gleam::Prefab::Extension();
	auto file = Gleam::Filesystem::Create(directory / filename, Gleam::FileType::Text);
	auto accessor = Gleam::Filesystem::WriteAccessor(directory / filename);

	Gleam::Prefab prefab;
	prefab.name = mWorld->name;
	prefab.Serialize(mWorld->GetEntityManager(), file.GetStream());
}

Gleam::TString PrefabBaker::Filename() const
{
	return mWorld->name;
}

Gleam::Guid PrefabBaker::TypeGuid() const
{
    return Gleam::Reflection::GetClass<Gleam::Prefab>().Guid();
}
