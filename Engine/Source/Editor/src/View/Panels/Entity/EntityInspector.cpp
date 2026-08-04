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
	mSelection = world->GetSubsystem<SelectionSystem>();
}

void EntityInspector::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		if (!ImGui::Begin("Entity Inspector")) return;

		auto& entityManager = mEditWorld->GetEntityManager();
		auto selectedEntity = mSelection->GetSelectedEntity();
        if (selectedEntity != Gleam::InvalidEntity)
        {
			auto& entity = entityManager.GetComponent<Gleam::Entity>(selectedEntity);
			auto localTransform = entity.GetLocalTransform();

			// Rotation is edited as euler angles, so the cache has to follow the transform gizmo
			if (selectedEntity != mCachedEntity || localTransform.rotation != mEntityRotation)
			{
				mEntityEulerRotation = Gleam::Math::Rad2Deg(Gleam::Math::EulerAngles(localTransform.rotation));
				mCachedEntity = selectedEntity;
			}

			PropertyDrawer::DrawCustom("Local Transform", Gleam::Reflection::GetClass<Gleam::Transform>().TypeHash(), [&]()
			{
				PropertyDrawer::DrawVec3Control("Translation", localTransform.position, 0.0f);
				PropertyDrawer::DrawVec3Control("Rotation", mEntityEulerRotation, 0.0f);
				PropertyDrawer::DrawScalarControl("Scale", localTransform.scale, 1.0f);
				localTransform.rotation = Gleam::Quaternion(Gleam::Math::Deg2Rad(mEntityEulerRotation));
			});
			entity.SetLocalTransform(localTransform);
			mEntityRotation = localTransform.rotation;

			entityManager.Visit(selectedEntity, [](void* component, const Gleam::Reflection::ClassDescription& classDesc)
			{
				if (classDesc.HasAttribute<Gleam::Reflection::Attribute::EntityComponent>())
				{
					PropertyDrawer::DrawClass(classDesc.ResolveName(), component, classDesc);
				}
			});
        }
		else if (mSelection->GetSelectedSingleton() != 0)
		{
			auto selectedSingleton = mSelection->GetSelectedSingleton();
			entityManager.VisitSingletons([selectedSingleton](void* component, const Gleam::Reflection::ClassDescription& classDesc)
			{
				if (classDesc.TypeHash() == selectedSingleton)
				{
					PropertyDrawer::DrawClass(classDesc.ResolveName(), component, classDesc);
				}
			});
		}
		ImGui::End();
	});
}
