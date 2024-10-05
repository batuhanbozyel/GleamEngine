#include "PrefabBaker.h"

using namespace GEditor;

PrefabBaker::PrefabBaker(const Gleam::RefCounted<Gleam::World>& world)
	: mWorld(world)
{
	
}

void PrefabBaker::Bake(Gleam::FileStream& stream) const
{
	Gleam::Prefab prefab;
	prefab.name = mWorld->GetName();
	prefab.entityCount = mWorld->GetEntityManager().GetEntityCount();
	prefab.Serialize(mWorld->GetEntityManager(), stream);
}

Gleam::TString PrefabBaker::Filename() const
{
	return mWorld->GetName();
}

Gleam::Guid PrefabBaker::TypeGuid() const
{
    return Gleam::Reflection::GetClass<Gleam::Prefab>().Guid();
}
