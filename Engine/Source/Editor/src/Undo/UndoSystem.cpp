//
//  UndoSystem.cpp
//  Editor
//

#include "UndoSystem.h"
#include "World/World.h"
#include "Serialization/JSONSerializer.h"

using namespace GEditor;

static Gleam::TStringView ResolveDisplayName(const Gleam::Reflection::ClassDescription& classDesc)
{
	if (classDesc.HasAttribute<Gleam::Reflection::Attribute::PrettyName>())
	{
		return classDesc.GetAttribute<Gleam::Reflection::Attribute::PrettyName>()->name;
	}
	return classDesc.ResolveName();
}

static bool IsSameTransform(const Gleam::Transform& lhs, const Gleam::Transform& rhs)
{
	return lhs.position == rhs.position && lhs.rotation == rhs.rotation && lhs.scale == rhs.scale;
}

void UndoSystem::Initialize(Gleam::World* world)
{
	mEditWorld = world;
}

void UndoSystem::Tick(Gleam::World* world)
{
	if (mRequest == Request::Undo && mUndoStack.empty() == false)
	{
		auto command = eastl::move(mUndoStack.back());
		mUndoStack.pop_back();
		command->Undo(world);
		mRedoStack.push_back(eastl::move(command));
	}
	else if (mRequest == Request::Redo && mRedoStack.empty() == false)
	{
		auto command = eastl::move(mRedoStack.back());
		mRedoStack.pop_back();
		command->Redo(world);
		mUndoStack.push_back(eastl::move(command));
	}
	mRequest = Request::None;
}

void UndoSystem::RequestUndo()
{
	mRequest = Request::Undo;
}

void UndoSystem::RequestRedo()
{
	mRequest = Request::Redo;
}

void UndoSystem::BeginTransformTransaction(const Gleam::TArray<Gleam::EntityHandle>& entities)
{
	mTransaction = Transaction();
	mTransaction.entities = entities;
	mTransaction.transforms = CaptureTransforms();
}

void UndoSystem::BeginEntityTransaction(const Gleam::TArray<Gleam::EntityHandle>& entities)
{
	mTransaction = Transaction();
	mTransaction.entities = entities;
	mTransaction.transforms = CaptureTransforms();
	mTransaction.components = CaptureComponents();
}

void UndoSystem::BeginSingletonTransaction(uint32_t typeHash)
{
	mTransaction = Transaction();
	mTransaction.singleton = typeHash;
	mTransaction.components = CaptureComponents();
}

void UndoSystem::EndTransaction()
{
	auto& entityManager = mEditWorld->GetEntityManager();

	Gleam::TArray<PropertyEditCommand::TransformEntry> transforms;
	if (mTransaction.transforms.empty() == false)
	{
		auto current = CaptureTransforms();
		for (size_t i = 0; i < current.size(); ++i)
		{
			if (IsSameTransform(current[i], mTransaction.transforms[i]) == false)
			{
				transforms.push_back({
					.entity = entityManager.GetComponent<Gleam::Entity>(mTransaction.entities[i]).GetGuid(),
					.before = mTransaction.transforms[i],
					.after = current[i]
				});
			}
		}
	}

	Gleam::TArray<PropertyEditCommand::ComponentEntry> components;
	if (mTransaction.components.empty() == false)
	{
		auto current = CaptureComponents();
		for (size_t i = 0; i < current.size(); ++i)
		{
			if (current[i].data != mTransaction.components[i].data)
			{
				components.push_back({
					.entity = current[i].entity == Gleam::InvalidEntity
						? Gleam::Guid(Gleam::Guid::InvalidGuid())
						: entityManager.GetComponent<Gleam::Entity>(current[i].entity).GetGuid(),
					.typeHash = current[i].typeHash,
					.before = mTransaction.components[i].data,
					.after = current[i].data
				});
			}
		}
	}

	mTransaction = Transaction();

	if (transforms.empty() && components.empty())
	{
		return;
	}

	Gleam::TString name = "Transform";
	if (components.empty() == false)
	{
		const auto componentName = ResolveDisplayName(*Gleam::Reflection::GetClass(components[0].typeHash));
		name = "Edit ";
		name.append(componentName.data(), componentName.size());
	}
	Push(Gleam::CreateScope<PropertyEditCommand>(name, eastl::move(transforms), eastl::move(components)));
}

void UndoSystem::RecordEntityCreation(Gleam::EntityHandle entity)
{
	EntitySnapshot snapshot;
	snapshot.Capture(mEditWorld->GetEntityManager(), { entity });
	Push(Gleam::CreateScope<CreateEntityCommand>(eastl::move(snapshot)));
}

void UndoSystem::DestroyEntities(const Gleam::TArray<Gleam::EntityHandle>& entities)
{
	EntitySnapshot snapshot;
	snapshot.Capture(mEditWorld->GetEntityManager(), entities);

	// The destroy itself goes through the command, so it takes the same path as a redo does
	auto command = Gleam::CreateScope<DestroyEntityCommand>(eastl::move(snapshot));
	command->Redo(mEditWorld);
	Push(eastl::move(command));
}

Gleam::TStringView UndoSystem::GetUndoName() const
{
	if (mUndoStack.empty())
	{
		return {};
	}
	return mUndoStack.back()->GetName();
}

Gleam::TStringView UndoSystem::GetRedoName() const
{
	if (mRedoStack.empty())
	{
		return {};
	}
	return mRedoStack.back()->GetName();
}

void UndoSystem::Push(Gleam::Scope<UndoCommand>&& command)
{
	mRedoStack.clear();
	mUndoStack.push_back(eastl::move(command));
}

Gleam::TArray<Gleam::Transform> UndoSystem::CaptureTransforms() const
{
	auto& entityManager = mEditWorld->GetEntityManager();

	Gleam::TArray<Gleam::Transform> transforms;
	transforms.reserve(mTransaction.entities.size());
	for (auto handle : mTransaction.entities)
	{
		transforms.push_back(entityManager.GetComponent<Gleam::Entity>(handle).GetLocalTransform());
	}
	return transforms;
}

Gleam::TArray<UndoSystem::ComponentState> UndoSystem::CaptureComponents() const
{
	auto& entityManager = mEditWorld->GetEntityManager();
	Gleam::TArray<ComponentState> components;
	Gleam::JSONSerializer serializer;

	if (mTransaction.singleton != 0)
	{
		entityManager.VisitSingletons([&](void* component, const Gleam::Reflection::ClassDescription& classDesc)
		{
			if (classDesc.TypeHash() == mTransaction.singleton)
			{
				components.push_back({
					.typeHash = classDesc.TypeHash(),
					.data = serializer.Serialize(component, classDesc)
				});
			}
		});
		return components;
	}

	for (auto handle : mTransaction.entities)
	{
		entityManager.Visit(handle, [&](void* component, const Gleam::Reflection::ClassDescription& classDesc)
		{
			if (classDesc.HasAttribute<Gleam::Reflection::Attribute::EntityComponent>())
			{
				components.push_back({
					.entity = handle,
					.typeHash = classDesc.TypeHash(),
					.data = serializer.Serialize(component, classDesc)
				});
			}
		});
	}
	return components;
}
