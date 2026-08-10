//
//  UndoCommand.h
//  Editor
//

#pragma once
#include "EntitySnapshot.h"
#include "World/Components/Transform.h"

namespace Gleam {
class World;
} // namespace Gleam

namespace GEditor {

class UndoCommand
{
public:

	UndoCommand(const Gleam::TString& name)
		: mName(name)
	{

	}

	virtual ~UndoCommand() = default;

	virtual void Undo(Gleam::World* world) = 0;

	virtual void Redo(Gleam::World* world) = 0;

	const Gleam::TString& GetName() const
	{
		return mName;
	}

private:

	Gleam::TString mName;

};

class PropertyEditCommand final : public UndoCommand
{
public:

	struct TransformEntry
	{
		Gleam::Guid entity = Gleam::Guid::InvalidGuid();
		Gleam::Transform before;
		Gleam::Transform after;
	};

	// An invalid entity marks a singleton component
	struct ComponentEntry
	{
		Gleam::Guid entity = Gleam::Guid::InvalidGuid();
		uint32_t typeHash = 0;
		Gleam::TString before;
		Gleam::TString after;
	};

	PropertyEditCommand(const Gleam::TString& name, Gleam::TArray<TransformEntry>&& transforms, Gleam::TArray<ComponentEntry>&& components)
		: UndoCommand(name)
		, mTransforms(eastl::move(transforms))
		, mComponents(eastl::move(components))
	{

	}

	virtual void Undo(Gleam::World* world) override;

	virtual void Redo(Gleam::World* world) override;

private:

	void Apply(Gleam::World* world, bool restoreBefore);

	Gleam::TArray<TransformEntry> mTransforms;

	Gleam::TArray<ComponentEntry> mComponents;

};

class EntityLifetimeCommand : public UndoCommand
{
public:

	EntityLifetimeCommand(const Gleam::TString& name, EntitySnapshot&& snapshot)
		: UndoCommand(name)
		, mSnapshot(eastl::move(snapshot))
	{

	}

protected:

	void RestoreEntities(Gleam::World* world);

	void DestroyEntities(Gleam::World* world);

private:

	EntitySnapshot mSnapshot;

};

class CreateEntityCommand final : public EntityLifetimeCommand
{
public:

	CreateEntityCommand(EntitySnapshot&& snapshot)
		: EntityLifetimeCommand("Create Entity", eastl::move(snapshot))
	{

	}

	virtual void Undo(Gleam::World* world) override
	{
		DestroyEntities(world);
	}

	virtual void Redo(Gleam::World* world) override
	{
		RestoreEntities(world);
	}

};

class DestroyEntityCommand final : public EntityLifetimeCommand
{
public:

	DestroyEntityCommand(EntitySnapshot&& snapshot)
		: EntityLifetimeCommand("Destroy Entity", eastl::move(snapshot))
	{

	}

	virtual void Undo(Gleam::World* world) override
	{
		RestoreEntities(world);
	}

	virtual void Redo(Gleam::World* world) override
	{
		DestroyEntities(world);
	}

};

} // namespace GEditor
