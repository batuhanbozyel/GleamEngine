//
//  WorldOutliner.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 25.05.2023.
//

#include "WorldOutliner.h"
#include "WorldViewport.h"

#include "Gleam.h"

#include <imgui.h>

using namespace GEditor;

void WorldOutliner::Init(Gleam::World* world)
{
	mEditWorld = world;
}

void WorldOutliner::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		if (!ImGui::Begin("World Outliner")) return;
    
		auto& entityManager = mEditWorld->GetEntityManager();
		entityManager.ForEach([&](Gleam::EntityHandle handle)
		{
			const auto& entity = entityManager.GetComponent<Gleam::Entity>(handle);
			if (entity.HasParent() == false)
			{
				DrawEntityNode(handle);
			}
		});
		
		ImGui::End();
	});
}

void WorldOutliner::DrawEntityNode(Gleam::EntityHandle handle)
{
	auto& entityManager = mEditWorld->GetEntityManager();
	uint32_t id = static_cast<uint32_t>(handle);

	const auto& entity = entityManager.GetComponent<Gleam::Entity>(handle);
	const auto& children = entity.GetChildren();
	bool hasChildren = children.size() > 0;

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
		ImGuiTreeNodeFlags_OpenOnDoubleClick |
		ImGuiTreeNodeFlags_SpanAvailWidth;

	if (not hasChildren)
	{
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	if (handle == mSelectedEntity)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	bool nodeOpen = ImGui::TreeNodeEx((void*)(uint64_t)id, flags, "%s", entity.GetName().c_str());

	if (ImGui::IsItemClicked())
	{
		Gleam::EventDispatcher<EntitySelectedEvent>::Publish(EntitySelectedEvent(handle));
		mSelectedEntity = handle;
	}

	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Destroy Entity"))
		{
			// TODO:
		}
		ImGui::EndPopup();
	}

	if (nodeOpen && hasChildren)
	{
		for (const auto& childHandle : children)
		{
			if (childHandle != Gleam::InvalidEntity)
			{
				DrawEntityNode(childHandle);
			}
		}
		ImGui::TreePop();
	}
}
