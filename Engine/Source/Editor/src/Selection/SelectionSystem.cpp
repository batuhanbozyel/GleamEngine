//
//  SelectionSystem.cpp
//  Editor
//

#include "SelectionSystem.h"
#include "World/World.h"

using namespace GEditor;

void SelectionSystem::Initialize(Gleam::World* world)
{
	auto picking = world->AddSubsystem<Gleam::PickingSystem>();
	mPickingCallback = picking->AddCallback([this](const Gleam::PickingResult& result)
	{
		SelectEntity(result.entities.empty() ? Gleam::InvalidEntity : result.entities.front());
	});
}

void SelectionSystem::Shutdown(Gleam::World* world)
{
	world->GetSubsystem<Gleam::PickingSystem>()->RemoveCallback(mPickingCallback);
	world->RemoveSubsystem<Gleam::PickingSystem>();
}

void SelectionSystem::SelectEntity(Gleam::EntityHandle entity)
{
	mSelectedEntity = entity;
	mSelectedSingleton = 0;
}

void SelectionSystem::SelectSingleton(uint32_t typeHash)
{
	mSelectedSingleton = typeHash;
	mSelectedEntity = Gleam::InvalidEntity;
}
