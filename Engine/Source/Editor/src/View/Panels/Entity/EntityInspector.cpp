//
//  EntityInspector.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 25.05.2023.
//

#include "EntityInspector.h"
#include "View/Widgets/PropertyDrawer.h"
#include "Selection/SelectionSystem.h"
#include "Undo/UndoSystem.h"
#include "Utils/ReflectionUtils.h"

#include "World/World.h"
#include "Renderer/Renderers/ImGuiRenderer.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <EASTL/sort.h>

#include <cstring>

using namespace GEditor;

static bool IsAddableComponent(const Gleam::Reflection::ClassDescription& classDesc)
{
	if (classDesc.HasAttribute<Gleam::Reflection::Attribute::EntityComponent>() == false)
	{
		return false;
	}

	// The entity owns its transform, a standalone Transform component would shadow it
	return classDesc.TypeHash() != Gleam::Reflection::GetClass<Gleam::Transform>().TypeHash();
}

void EntityInspector::OnCreate(Gleam::World* world)
{
	mEditWorld = world;
	mSelectionSystem = world->GetSubsystem<SelectionSystem>();
	mUndoSystem = world->GetSubsystem<UndoSystem>();
}

void EntityInspector::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		if (ImGui::Begin("Entity Inspector"))
		{
			PropertyDrawer::BeginEditTracking();

			const auto& selectedEntities = mSelectionSystem->GetSelectedEntities();
			const auto selectedSingleton = mSelectionSystem->GetSelectedSingleton();
			if (selectedEntities.empty() == false)
			{
				DrawEntities(selectedEntities);

				if (PropertyDrawer::HasPendingEdit())
				{
					mUndoSystem->BeginEntityTransaction(selectedEntities);
					PropertyDrawer::ApplyPendingEdit();
					mUndoSystem->EndTransaction();
				}
				
				if (PropertyDrawer::EditStarted())
				{
					mUndoSystem->BeginEntityTransaction(selectedEntities);
				}
			}
			else if (selectedSingleton != 0)
			{
				DrawSingleton(selectedSingleton);

				if (PropertyDrawer::HasPendingEdit())
				{
					mUndoSystem->BeginSingletonTransaction(selectedSingleton);
					PropertyDrawer::ApplyPendingEdit();
					mUndoSystem->EndTransaction();
				}

				if (PropertyDrawer::EditStarted())
				{
					mUndoSystem->BeginSingletonTransaction(selectedSingleton);
				}
			}

			if (PropertyDrawer::EditCommitted())
			{
				mUndoSystem->EndTransaction();
			}
		}
		ImGui::End();
	});
}

void EntityInspector::DrawEntities(const Gleam::TArray<Gleam::EntityHandle>& entities)
{
	auto activeEntity = mSelectionSystem->GetActiveEntity();

	Gleam::TArray<Gleam::EntityHandle> selectionOrder;
	selectionOrder.push_back(activeEntity);
	for (auto handle : entities)
	{
		if (handle != activeEntity)
		{
			selectionOrder.push_back(handle);
		}
	}

	if (selectionOrder.size() > 1)
	{
		ImGui::TextDisabled("%u Entities Selected", static_cast<uint32_t>(selectionOrder.size()));
	}

	DrawTransform(selectionOrder);
	DrawComponents(selectionOrder);
	DrawAddComponent(selectionOrder);
}

void EntityInspector::DrawTransform(const Gleam::TArray<Gleam::EntityHandle>& entities)
{
	auto& entityManager = mEditWorld->GetEntityManager();
	auto& activeEntity = entityManager.GetComponent<Gleam::Entity>(entities[0]);
	auto localTransform = activeEntity.GetLocalTransform();

	bool positionMixed = false;
	bool rotationMixed = false;
	bool scaleMixed = false;
	for (size_t i = 1; i < entities.size(); ++i)
	{
		const auto& other = entityManager.GetComponent<Gleam::Entity>(entities[i]).GetLocalTransform();
		positionMixed |= other.position != localTransform.position;
		rotationMixed |= other.rotation != localTransform.rotation;
		scaleMixed |= other.scale != localTransform.scale;
	}

	const auto previousTransform = localTransform;

	PropertyDrawer::DrawCustom("Local Transform", Gleam::Reflection::GetClass<Gleam::Transform>().TypeHash(), [&]()
	{
		ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, positionMixed);
		PropertyDrawer::DrawVec3Control("Translation", localTransform.position, 0.0f);
		ImGui::PopItemFlag();

		ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, rotationMixed);
		PropertyDrawer::DrawRotationControl("Rotation", localTransform.rotation);
		ImGui::PopItemFlag();

		ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, scaleMixed);
		PropertyDrawer::DrawScalarControl("Scale", localTransform.scale, 1.0f);
		ImGui::PopItemFlag();
	});

	activeEntity.SetLocalTransform(localTransform);

	const bool positionEdited = localTransform.position != previousTransform.position;
	const bool rotationEdited = localTransform.rotation != previousTransform.rotation;
	const bool scaleEdited = localTransform.scale != previousTransform.scale;
	if (positionEdited || rotationEdited || scaleEdited)
	{
		for (size_t i = 1; i < entities.size(); ++i)
		{
			auto& entity = entityManager.GetComponent<Gleam::Entity>(entities[i]);
			auto transform = entity.GetLocalTransform();
			if (positionEdited)
			{
				transform.position = localTransform.position;
			}
			if (rotationEdited)
			{
				transform.rotation = localTransform.rotation;
			}
			if (scaleEdited)
			{
				transform.scale = localTransform.scale;
			}
			entity.SetLocalTransform(transform);
		}
	}
}

void EntityInspector::DrawComponents(const Gleam::TArray<Gleam::EntityHandle>& entities)
{
	struct SharedComponent
	{
		const Gleam::Reflection::ClassDescription* classDesc = nullptr;
		Gleam::TArray<void*> instances;
	};
	Gleam::TArray<SharedComponent> sharedComponents;

	auto& entityManager = mEditWorld->GetEntityManager();
	entityManager.Visit(entities[0], [&](void* component, const Gleam::Reflection::ClassDescription& classDesc)
	{
		if (classDesc.HasAttribute<Gleam::Reflection::Attribute::EntityComponent>())
		{
			sharedComponents.push_back({ .classDesc = &classDesc, .instances = { component } });
		}
	});

	// Only the component types the whole selection has in common can be edited together
	for (size_t i = 1; i < entities.size(); ++i)
	{
		Gleam::HashMap<uint32_t, void*> componentLookup;
		entityManager.Visit(entities[i], [&](void* component, const Gleam::Reflection::ClassDescription& classDesc)
		{
			componentLookup[classDesc.TypeHash()] = component;
		});

		for (auto it = sharedComponents.begin(); it != sharedComponents.end();)
		{
			auto found = componentLookup.find(it->classDesc->TypeHash());
			if (found == componentLookup.end())
			{
				it = sharedComponents.erase(it);
			}
			else
			{
				it->instances.push_back(found->second);
				++it;
			}
		}
	}

	uint32_t pendingRemove = 0;
	for (auto& shared : sharedComponents)
	{
		const bool dirtyBefore = PropertyDrawer::EditDirty();
		PropertyDrawer::DrawClass(ReflectionUtils::ResolveDisplayName(*shared.classDesc), shared.instances, *shared.classDesc, 0.0f, [&]()
		{
			const float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
			if (PropertyDrawer::DrawSettingsButton(lineHeight))
			{
				ImGui::OpenPopup("ComponentSettings");
			}

			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Remove Component"))
				{
					pendingRemove = shared.classDesc->TypeHash();
				}
				ImGui::EndPopup();
			}
		});

		if (not dirtyBefore and PropertyDrawer::EditDirty())
		{
			for (auto handle : entities)
			{
				entityManager.GetChangeTracker().MarkChanged(shared.classDesc->TypeHash(), handle);
			}
		}
	}

	// The component pointers above stay valid while the list is being drawn
	if (pendingRemove != 0)
	{
		mUndoSystem->RemoveComponent(pendingRemove, entities);
	}
}

void EntityInspector::DrawAddComponent(const Gleam::TArray<Gleam::EntityHandle>& entities)
{
	auto& entityManager = mEditWorld->GetEntityManager();

	ImGui::Separator();
	if (ImGui::Button("Add Component", ImVec2(-1.0f, 0.0f)))
	{
		ImGui::OpenPopup("AddComponent");
	}

	if (ImGui::BeginPopup("AddComponent") == false)
	{
		return;
	}

	Gleam::TArray<const Gleam::Reflection::ClassDescription*> candidates;
	for (const auto& classDesc : Gleam::Reflection::IDatabase::GetInstance()->GetClasses())
	{
		if (IsAddableComponent(classDesc))
		{
			candidates.push_back(&classDesc);
		}
	}

	eastl::sort(candidates.begin(), candidates.end(), [](const auto lhs, const auto rhs)
	{
		return ReflectionUtils::ResolveDisplayName(*lhs) < ReflectionUtils::ResolveDisplayName(*rhs);
	});

	bool anyAvailable = false;
	for (const auto* classDesc : candidates)
	{
		Gleam::TArray<Gleam::EntityHandle> missing;
		for (auto handle : entities)
		{
			if (entityManager.FindComponent(handle, classDesc->TypeHash()) == nullptr)
			{
				missing.push_back(handle);
			}
		}

		if (missing.empty())
		{
			continue;
		}
		anyAvailable = true;

		const auto displayName = ReflectionUtils::ResolveDisplayName(*classDesc);
		char buffer[64];
		std::memcpy(buffer, displayName.data(), displayName.size());
		buffer[displayName.size()] = '\0';

		if (ImGui::MenuItem(buffer))
		{
			mUndoSystem->AddComponent(classDesc->TypeHash(), missing);
			ImGui::CloseCurrentPopup();
		}
	}

	if (anyAvailable == false)
	{
		ImGui::TextDisabled("No components available");
	}
	ImGui::EndPopup();
}

void EntityInspector::DrawSingleton(uint32_t typeHash)
{
	mEditWorld->GetEntityManager().VisitSingletons([typeHash](void* component, const Gleam::Reflection::ClassDescription& classDesc)
	{
		if (classDesc.TypeHash() == typeHash)
		{
			PropertyDrawer::DrawClass(ReflectionUtils::ResolveDisplayName(classDesc), component, classDesc);
		}
	});
}
