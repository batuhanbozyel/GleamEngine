//
//  SelectionSystem.cpp
//  Editor
//

#include "SelectionSystem.h"
#include "World/World.h"
#include "World/Systems/RenderSceneProxy.h"

using namespace GEditor;

void SelectionSystem::Initialize(Gleam::World* world)
{
	auto picking = world->AddSubsystem<Gleam::PickingSystem>();
	mPickingCallback = picking->AddCallback([this](const Gleam::PickingResult& result)
	{
		SelectEntities(result.entities);
	});
}

void SelectionSystem::Shutdown(Gleam::World* world)
{
	world->GetSubsystem<Gleam::PickingSystem>()->RemoveCallback(mPickingCallback);
	world->RemoveSubsystem<Gleam::PickingSystem>();
}

void SelectionSystem::Tick(Gleam::World* world)
{
	mInstanceMask.clear();
	if (mSelectedEntities.empty())
	{
		return;
	}

	const auto& globalMeshes = world->GetSubsystem<Gleam::RenderSceneProxy>()->GetGlobalMeshes();
	const auto instanceCount = static_cast<uint32_t>(globalMeshes.size());
	mInstanceMask.resize(Gleam::Math::DivideRoundingUp(instanceCount, 32u), 0u);

	bool hasSelectedInstance = false;
	for (uint32_t instanceID = 0; instanceID < instanceCount; ++instanceID)
	{
		if (IsSelected(globalMeshes[instanceID].entity))
		{
			mInstanceMask[instanceID >> 5u] |= 1u << (instanceID & 31u);
			hasSelectedInstance = true;
		}
	}

	if (hasSelectedInstance == false)
	{
		mInstanceMask.clear();
	}
}

void SelectionSystem::SelectEntity(Gleam::EntityHandle entity)
{
	mSelectedEntities.clear();
	if (entity != Gleam::InvalidEntity)
	{
		mSelectedEntities.push_back(entity);
	}
	mSelectedSingleton = 0;
}

void SelectionSystem::SelectEntities(const Gleam::TArray<Gleam::EntityHandle>& entities)
{
	mSelectedEntities = entities;
	mSelectedSingleton = 0;
}

void SelectionSystem::SelectSingleton(uint32_t typeHash)
{
	mSelectedSingleton = typeHash;
	mSelectedEntities.clear();
}

void SelectionSystem::ClearSelection()
{
	mSelectedEntities.clear();
	mSelectedSingleton = 0;
}

bool SelectionSystem::IsSelected(Gleam::EntityHandle entity) const
{
	return eastl::find(mSelectedEntities.begin(), mSelectedEntities.end(), entity) != mSelectedEntities.end();
}
