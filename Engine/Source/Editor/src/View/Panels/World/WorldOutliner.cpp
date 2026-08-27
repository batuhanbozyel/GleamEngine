//
//  WorldOutliner.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 25.05.2023.
//

#include "WorldOutliner.h"
#include "WorldViewport.h"
#include "Selection/SelectionSystem.h"
#include "Undo/UndoSystem.h"

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
	mSelectionSystem = world->GetSubsystem<SelectionSystem>();
	mUndoSystem = world->GetSubsystem<UndoSystem>();
}

void WorldOutliner::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		if (ImGui::Begin("World Outliner"))
		{
			static float singletonsPanelHeight = 400.0f;
			float availableHeight = ImGui::GetContentRegionAvail().y;

			ImGui::BeginChild("EntityList", ImVec2(0, availableHeight - singletonsPanelHeight - 8.0f), ImGuiChildFlags_None);
			{
				mVisibleEntities.clear();
				if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen))
				{
					auto& entityManager = mEditWorld->GetEntityManager();
					entityManager.ForEach<Gleam::Entity>([&](Gleam::Entity& entity)
					{
						if (entity.HasParent() == false)
						{
							DrawEntityNode(entity);
						}
					});
				}

				if (mPendingRangeSelect != Gleam::InvalidEntity)
				{
					SelectRange(mRangeAnchor, mPendingRangeSelect, mPendingRangeAdditive ? SelectionMode::Add : SelectionMode::Replace);
					mPendingRangeSelect = Gleam::InvalidEntity;
				}

				if (mPendingDestroy != Gleam::InvalidEntity)
				{
					Gleam::TArray<Gleam::EntityHandle> entities;
					if (mSelectionSystem->IsSelected(mPendingDestroy))
					{
						entities = mSelectionSystem->GetSelectedEntities();
					}
					else
					{
						entities.push_back(mPendingDestroy);
					}

					mUndoSystem->DestroyEntities(entities);
					mPendingDestroy = Gleam::InvalidEntity;
					mRangeAnchor = Gleam::InvalidEntity;
				}

				if (ImGui::IsWindowHovered() && ImGui::IsAnyItemHovered() == false && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					mSelectionSystem->ClearSelection();
					mRangeAnchor = Gleam::InvalidEntity;
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
		}
		ImGui::End();
	});
}

void WorldOutliner::DrawEntityNode(const Gleam::Entity& entity)
{
	auto& entityManager = mEditWorld->GetEntityManager();
	auto handle = entity.GetHandle();
	uint32_t id = static_cast<uint32_t>(handle);

	const auto& children = entity.GetChildren();
	bool hasChildren = children.size() > 0;

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
		ImGuiTreeNodeFlags_OpenOnDoubleClick |
		ImGuiTreeNodeFlags_SpanAvailWidth;

	if (not hasChildren)
	{
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	if (mSelectionSystem->IsSelected(handle))
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	mVisibleEntities.push_back(handle);

	bool nodeOpen = ImGui::TreeNodeEx((void*)(uint64_t)id, flags, "%s", entity.GetName().c_str());

	if (ImGui::IsItemClicked() && ImGui::IsItemToggledOpen() == false)
	{
		HandleSelectionInput(handle);
	}

	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Destroy Entity"))
		{
			mPendingDestroy = handle;
		}
		ImGui::EndPopup();
	}

	if (nodeOpen && hasChildren)
	{
		for (const auto& childHandle : children)
		{
			if (childHandle != Gleam::InvalidEntity)
			{
				DrawEntityNode(entity.GetChildEntity(childHandle));
			}
		}
		ImGui::TreePop();
	}
}

void WorldOutliner::HandleSelectionInput(Gleam::EntityHandle handle)
{
	const auto& io = ImGui::GetIO();
	const bool additive = io.KeyCtrl || io.KeySuper;

	if (io.KeyShift)
	{
		mPendingRangeSelect = handle;
		mPendingRangeAdditive = additive;
		return;
	}

	mSelectionSystem->SelectEntity(handle, additive ? SelectionMode::Toggle : SelectionMode::Replace);
	mRangeAnchor = handle;
}

void WorldOutliner::SelectRange(Gleam::EntityHandle anchor, Gleam::EntityHandle target, SelectionMode mode)
{
	auto targetIt = eastl::find(mVisibleEntities.begin(), mVisibleEntities.end(), target);
	auto anchorIt = eastl::find(mVisibleEntities.begin(), mVisibleEntities.end(), anchor);

	// Shift clicking before anything else was selected has nothing to extend from
	if (anchorIt == mVisibleEntities.end())
	{
		anchorIt = targetIt;
	}

	auto first = anchorIt;
	auto last = targetIt;
	if (first > last)
	{
		eastl::swap(first, last);
	}

	mSelectionSystem->SelectEntities(Gleam::TArray<Gleam::EntityHandle>(first, last + 1), mode);
	mSelectionSystem->SetActiveEntity(target);
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

			if (mSelectionSystem->GetSelectedSingleton() == componentID)
			{
				flags |= ImGuiTreeNodeFlags_Selected;
			}

			char label[64];
			std::memcpy(label, componentName.data(), componentName.size());
			label[componentName.size()] = '\0';

			ImGui::TreeNodeEx((void*)(uint64_t)componentID, flags, "%s", label);

			if (ImGui::IsItemClicked())
			{
				mSelectionSystem->SelectSingleton(componentID);
			}
		}
	});
}
