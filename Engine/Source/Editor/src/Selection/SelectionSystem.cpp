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
	mPickingSystem = world->AddSubsystem<Gleam::PickingSystem>();
	mPickingCallback = mPickingSystem->AddCallback([this](const Gleam::PickingResult& result)
	{
		auto mode = SelectionMode::Replace;
		auto it = mPendingPickModes.find(result.requestID);
		if (it != mPendingPickModes.end())
		{
			mode = it->second;
			mPendingPickModes.erase(it);
		}
		SelectEntities(result.entities, mode);
	});
}

void SelectionSystem::Shutdown(Gleam::World* world)
{
	world->GetSubsystem<Gleam::PickingSystem>()->RemoveCallback(mPickingCallback);
	world->RemoveSubsystem<Gleam::PickingSystem>();
	mPickingSystem = nullptr;
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

Gleam::PickingRequestID SelectionSystem::RequestPick(const Gleam::PickingRequest& request, SelectionMode mode)
{
	auto requestID = mPickingSystem->RequestPick(request);
	mPendingPickModes[requestID] = mode;
	return requestID;
}

void SelectionSystem::SelectEntity(Gleam::EntityHandle entity, SelectionMode mode)
{
	if (mode == SelectionMode::Replace)
	{
		mSelectedEntities.clear();
	}

	ApplyEntity(entity, mode);
	ResolveActiveEntity();
	mSelectedSingleton = 0;
}

void SelectionSystem::SelectEntities(const Gleam::TArray<Gleam::EntityHandle>& entities, SelectionMode mode)
{
	if (mode == SelectionMode::Replace)
	{
		mSelectedEntities.clear();
	}

	for (auto entity : entities)
	{
		ApplyEntity(entity, mode);
	}
	ResolveActiveEntity();
	mSelectedSingleton = 0;
}

void SelectionSystem::SetActiveEntity(Gleam::EntityHandle entity)
{
	if (IsSelected(entity))
	{
		mActiveEntity = entity;
	}
}

void SelectionSystem::SelectSingleton(uint32_t typeHash)
{
	mSelectedSingleton = typeHash;
	mSelectedEntities.clear();
	mActiveEntity = Gleam::InvalidEntity;
}

void SelectionSystem::ClearSelection()
{
	mSelectedEntities.clear();
	mActiveEntity = Gleam::InvalidEntity;
	mSelectedSingleton = 0;
}

bool SelectionSystem::IsSelected(Gleam::EntityHandle entity) const
{
	return eastl::find(mSelectedEntities.begin(), mSelectedEntities.end(), entity) != mSelectedEntities.end();
}

void SelectionSystem::ApplyEntity(Gleam::EntityHandle entity, SelectionMode mode)
{
	if (entity == Gleam::InvalidEntity)
	{
		return;
	}

	auto it = eastl::find(mSelectedEntities.begin(), mSelectedEntities.end(), entity);
	if (it != mSelectedEntities.end())
	{
		if (mode == SelectionMode::Toggle)
		{
			mSelectedEntities.erase(it);
			return;
		}
	}
	else
	{
		mSelectedEntities.push_back(entity);
	}
	mActiveEntity = entity;
}

void SelectionSystem::ResolveActiveEntity()
{
	if (IsSelected(mActiveEntity) == false)
	{
		mActiveEntity = mSelectedEntities.empty() ? Gleam::InvalidEntity : mSelectedEntities.back();
	}
}
