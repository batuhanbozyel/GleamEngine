//
//  EntityInspector.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 25.05.2023.
//

#include "EntityInspector.h"
#include "View/Widgets/PropertyDrawer.h"
#include "Selection/SelectionSystem.h"

#include "World/World.h"
#include "Renderer/Renderers/ImGuiRenderer.h"

#include <imgui.h>
#include <imgui_internal.h>

using namespace GEditor;

void EntityInspector::OnCreate(Gleam::World* world)
{
	mEditWorld = world;
	mSelectionSystem = world->GetSubsystem<SelectionSystem>();
}

void EntityInspector::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		if (ImGui::Begin("Entity Inspector"))
		{
			const auto& selectedEntities = mSelectionSystem->GetSelectedEntities();
			if (selectedEntities.empty() == false)
			{
				DrawEntities(selectedEntities);
			}
			else if (mSelectionSystem->GetSelectedSingleton() != 0)
			{
				DrawSingleton(mSelectionSystem->GetSelectedSingleton());
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
}

void EntityInspector::DrawTransform(const Gleam::TArray<Gleam::EntityHandle>& entities)
{
	auto& entityManager = mEditWorld->GetEntityManager();
	auto& activeEntity = entityManager.GetComponent<Gleam::Entity>(entities[0]);
	auto localTransform = activeEntity.GetLocalTransform();

	// Rotation is edited as euler angles, so the cache has to follow the transform gizmo
	if (entities[0] != mCachedEntity || localTransform.rotation != mEntityRotation)
	{
		mEntityEulerRotation = Gleam::Math::Rad2Deg(Gleam::Math::EulerAngles(localTransform.rotation));
		mCachedEntity = entities[0];
	}

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
	const auto previousEulerRotation = mEntityEulerRotation;

	PropertyDrawer::DrawCustom("Local Transform", Gleam::Reflection::GetClass<Gleam::Transform>().TypeHash(), [&]()
	{
		ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, positionMixed);
		PropertyDrawer::DrawVec3Control("Translation", localTransform.position, 0.0f);
		ImGui::PopItemFlag();

		ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, rotationMixed);
		PropertyDrawer::DrawVec3Control("Rotation", mEntityEulerRotation, 0.0f);
		ImGui::PopItemFlag();

		ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, scaleMixed);
		PropertyDrawer::DrawScalarControl("Scale", localTransform.scale, 1.0f);
		ImGui::PopItemFlag();

		localTransform.rotation = Gleam::Quaternion(Gleam::Math::Deg2Rad(mEntityEulerRotation));
	});

	activeEntity.SetLocalTransform(localTransform);
	mEntityRotation = localTransform.rotation;

	const bool positionEdited = localTransform.position != previousTransform.position;
	const bool rotationEdited = mEntityEulerRotation != previousEulerRotation;
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

	for (auto& shared : sharedComponents)
	{
		PropertyDrawer::DrawClass(shared.classDesc->ResolveName(), shared.instances, *shared.classDesc);
	}
}

void EntityInspector::DrawSingleton(uint32_t typeHash)
{
	mEditWorld->GetEntityManager().VisitSingletons([typeHash](void* component, const Gleam::Reflection::ClassDescription& classDesc)
	{
		if (classDesc.TypeHash() == typeHash)
		{
			PropertyDrawer::DrawClass(classDesc.ResolveName(), component, classDesc);
		}
	});
}
