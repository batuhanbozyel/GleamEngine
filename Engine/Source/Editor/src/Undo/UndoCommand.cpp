//
//  UndoCommand.cpp
//  Editor
//

#include "UndoCommand.h"
#include "Selection/SelectionSystem.h"

#include "World/World.h"
#include "Serialization/JSONSerializer.h"

using namespace GEditor;

void PropertyEditCommand::Undo(Gleam::World* world)
{
	Apply(world, true);
}

void PropertyEditCommand::Redo(Gleam::World* world)
{
	Apply(world, false);
}

void PropertyEditCommand::Apply(Gleam::World* world, bool undo)
{
	auto& entityManager = world->GetEntityManager();
	for (const auto& entry : mTransforms)
	{
		auto handle = entityManager.GetEntity(Gleam::EntityReference{ .guid = entry.entity });
		entityManager.GetComponent<Gleam::Entity>(handle).SetLocalTransform(undo ? entry.before : entry.after);
	}

	Gleam::JSONSerializer serializer;
	for (const auto& entry : mComponents)
	{
		const auto classDesc = Gleam::Reflection::GetClass(entry.typeHash);
		auto component = entry.entity == Gleam::Guid::InvalidGuid()
			? entityManager.FindSingleton(entry.typeHash)
			: entityManager.FindComponent(entityManager.GetEntity(Gleam::EntityReference{ .guid = entry.entity }), entry.typeHash);

		serializer.Deserialize(*classDesc, component, undo ? entry.before : entry.after);
	}
}

void EntityLifetimeCommand::RestoreEntities(Gleam::World* world)
{
	auto restored = mSnapshot.Restore(world->GetEntityManager());
	world->GetSubsystem<SelectionSystem>()->SelectEntities(restored);
}

void EntityLifetimeCommand::DestroyEntities(Gleam::World* world)
{
	auto destroyed = mSnapshot.Destroy(world->GetEntityManager());
	auto selectionSystem = world->GetSubsystem<SelectionSystem>();
	
	Gleam::TArray<Gleam::EntityHandle> selection;
	for (auto handle : selectionSystem->GetSelectedEntities())
	{
		if (eastl::find(destroyed.begin(), destroyed.end(), handle) == destroyed.end())
		{
			selection.push_back(handle);
		}
	}
	selectionSystem->SelectEntities(selection);
}
