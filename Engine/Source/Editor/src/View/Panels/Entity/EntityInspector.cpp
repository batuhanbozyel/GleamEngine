//
//  EntityInspector.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 25.05.2023.
//

#include "EntityInspector.h"
#include "View/Widgets/PropertyDrawer.h"

#include "World/World.h"
#include "Renderer/Renderers/ImGuiRenderer.h"

#include <imgui.h>
#include <imgui_internal.h>

using namespace GEditor;

void EntityInspector::Init(Gleam::World* world)
{
	mEditWorld = world;
    Gleam::EventDispatcher<EntitySelectedEvent>::Subscribe([this](EntitySelectedEvent e)
    {
        mSelectedEntity = e.GetEntity();
    });
}

void EntityInspector::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		if (!ImGui::Begin("Entity Inspector")) return;
        
        if (mSelectedEntity == Gleam::InvalidEntity)
        {
            ImGui::End();
            return;
        }
        
        auto& entityManager = mEditWorld->GetEntityManager();
		auto& entity = entityManager.GetComponent<Gleam::Entity>(mSelectedEntity);
		auto localTransform = entity.GetLocalTransform();
		PropertyDrawer::DrawCustom("Local Transform", Gleam::Reflection::GetClass<Gleam::Transform>().TypeHash(), [&]()
		{
			auto localRotation = Gleam::Math::Rad2Deg(localTransform.rotation.EulerAngles());
			PropertyDrawer::DrawVec3Control("Translation", localTransform.position, 0.0f);
			PropertyDrawer::DrawVec3Control("Rotation", localRotation, 0.0f);
			PropertyDrawer::DrawScalarControl<float>("Scale", localTransform.scale, 1.0f);
			localTransform.rotation = Gleam::Quaternion(Gleam::Math::Deg2Rad(localRotation));
		});
		entity.SetLocalTransform(localTransform);

		entityManager.Visit(mSelectedEntity, [](void* component, const Gleam::Reflection::ClassDescription& classDesc)
		{
			if (classDesc.HasAttribute<Gleam::Reflection::Attribute::EntityComponent>())
			{
				PropertyDrawer::DrawClass(classDesc.ResolveName(), component, classDesc);
			}
		});
        
		ImGui::End();
	});
}
