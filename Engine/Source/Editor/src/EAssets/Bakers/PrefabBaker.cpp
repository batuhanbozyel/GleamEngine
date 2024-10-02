#include "PrefabBaker.h"

using namespace GEditor;

PrefabBaker::PrefabBaker(Gleam::World&& world)
	: mWorld(std::move(world))
{
	
}

void PrefabBaker::Bake(Gleam::FileStream& stream) const
{
	
}

Gleam::TString PrefabBaker::Filename() const
{
	return mWorld.GetName();
}

Gleam::Guid PrefabBaker::TypeGuid() const
{
    return Gleam::Reflection::GetClass<Gleam::Prefab>().Guid();
}
