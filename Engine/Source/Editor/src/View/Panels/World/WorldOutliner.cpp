//
//  WorldOutliner.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 25.05.2023.
//

#include "WorldOutliner.h"
#include "WorldViewport.h"
#include "Selection/SelectionSystem.h"

#include "Core/Globals.h"
#include "Core/Engine.h"

#include "Renderer/Renderers/ImGuiRenderer.h"
#include "World/World.h"

#include <imgui.h>

using namespace GEditor;

static Gleam::TStringView ResolveDisplayName(const Gleam::Reflection::ClassDescription& desc)
{
	if (desc.HasAttribute<Gleam::Reflection::Attribute::PrettyName>())
	{
		return desc.GetAttribute<Gleam::Reflection::Attribute::PrettyName>()->name;
	}
	return desc.ResolveName();
}

void WorldOutliner::OnCreate(Gleam::World* world)
{
	mEditWorld = world;
	mSelection = world->GetSubsystem<SelectionSystem>();
}

void WorldOutliner::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		if (!ImGui::Begin("World Outliner")) return;

		static float singletonsPanelHeight = 400.0f;
		float availableHeight = ImGui::GetContentRegionAvail().y;

		ImGui::BeginChild("EntityList", ImVec2(0, availableHeight - singletonsPanelHeight - 8.0f), ImGuiChildFlags_None);
		{
			if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto& entityManager = mEditWorld->GetEntityManager();
				entityManager.ForEach([&](Gleam::EntityHandle handle)
				{
					const auto& entity = entityManager.GetComponent<Gleam::Entity>(handle);
					if (entity.HasParent() == false)
					{
						DrawEntityNode(handle);
					}
				});
			}
		}
		ImGui::EndChild();

		ImGui::Button("##splitter", ImVec2(-1, 4.0f));
		if (ImGui::IsItemActive())
		{
			float delta = ImGui::GetIO().MouseDelta.y;
			singletonsPanelHeight -= delta;
			singletonsPanelHeight = Gleam::Math::Clamp(singletonsPanelHeight, 50.0f, availableHeight - 50.0f);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
		}

		ImGui::BeginChild("SingletonsList", ImVec2(0, 0), ImGuiChildFlags_None);
		{
			if (ImGui::CollapsingHeader("Singletons", ImGuiTreeNodeFlags_DefaultOpen))
			{
				DrawSingletonComponents();
			}
		}
		ImGui::EndChild();
		
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

	if (mSelection->IsSelected(handle))
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	bool nodeOpen = ImGui::TreeNodeEx((void*)(uint64_t)id, flags, "%s", entity.GetName().c_str());

	if (ImGui::IsItemClicked())
	{
		mSelection->SelectEntity(handle);
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

void WorldOutliner::DrawSingletonComponents()
{
	auto& entityManager = mEditWorld->GetEntityManager();
	entityManager.VisitSingletons([this](const void* component, const Gleam::Reflection::ClassDescription& classDesc)
	{
		if (classDesc.Guid() != Gleam::Reflection::GetClass<Gleam::Entity>().Guid())
		{
			auto componentName = ResolveDisplayName(classDesc);
			uint32_t componentID = classDesc.TypeHash();

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf |
				ImGuiTreeNodeFlags_NoTreePushOnOpen |
				ImGuiTreeNodeFlags_SpanAvailWidth;

			if (mSelection->GetSelectedSingleton() == componentID)
			{
				flags |= ImGuiTreeNodeFlags_Selected;
			}

			char label[64];
			std::memcpy(label, componentName.data(), componentName.size());
			label[componentName.size()] = '\0';

			ImGui::TreeNodeEx((void*)(uint64_t)componentID, flags, "%s", label);

			if (ImGui::IsItemClicked())
			{
				mSelection->SelectSingleton(componentID);
			}
		}
	});
}
