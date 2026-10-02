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
#include "Utils/ReflectionUtils.h"
#include "View/ViewStack.h"
#include "View/GleamTheme.h"
#include "View/IconsLucide.h"
#include "View/Widgets/EditorWidgets.h"

#include "Core/Globals.h"
#include "Core/Engine.h"
#include "Core/Application.h"

#include "Renderer/Renderers/ImGuiRenderer.h"
#include "World/World.h"

#include <imgui_internal.h>

using namespace GEditor;

static constexpr float kRowHeight = 30.0f;
static constexpr float kRowMargin = 6.0f;
static constexpr float kIndentWidth = 16.0f;
static constexpr float kChevronWidth = 14.0f;

WorldOutliner::WorldOutliner(Gleam::World* world)
	: mEditWorld(world)
{
	mSelectionSystem = world->GetSubsystem<SelectionSystem>();
	mUndoSystem = world->GetSubsystem<UndoSystem>();
}

void WorldOutliner::OnCreate(Gleam::Application* app)
{
	mFonts = &app->GetSubsystem<ViewStack>()->GetFonts();
}

void WorldOutliner::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, Gleam::Theme::Panel);
		const bool visible = ImGui::Begin("World Outliner");
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		if (visible)
		{
			auto& entityManager = mEditWorld->GetEntityManager();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

			const float width = ImGui::GetContentRegionAvail().x;
			ImGui::SetCursorPos(ImVec2(10.0f, ImGui::GetCursorPosY() + 10.0f));
			Widgets::SearchField("##OutlinerSearch", "Search", mSearch, sizeof(mSearch), width - 20.0f);
			ImGui::Dummy(ImVec2(0.0f, 6.0f));

			uint32_t entityCount = 0;
			entityManager.ForEach<Gleam::Entity>([&](Gleam::Entity& entity)
			{
				if (entityManager.IsEditorOnly(entity) == false)
				{
					++entityCount;
				}
			});
			Widgets::CaptionRow(*mFonts, "ENTITIES", entityCount, 28.0f, 14.0f);

			uint32_t singletonCount = 0;
			entityManager.VisitSingletons([&](const void* component, const Gleam::Reflection::ClassDescription& classDesc)
			{
				if (classDesc.Guid() != Gleam::Reflection::GetClass<Gleam::Entity>().Guid())
				{
					++singletonCount;
				}
			});

			constexpr float kSingletonHeaderHeight = 32.0f;
			constexpr float kSingletonPadding = 10.0f;
			const float singletonsHeight = ImMin(1.0f + kSingletonHeaderHeight + singletonCount * kRowHeight + kSingletonPadding, ImGui::GetContentRegionAvail().y * 0.5f);

			ImGui::BeginChild("EntityList", ImVec2(0.0f, ImGui::GetContentRegionAvail().y - singletonsHeight), ImGuiChildFlags_None);
			{
				mVisibleEntities.clear();
				entityManager.ForEach<Gleam::Entity>([&](Gleam::Entity& entity)
				{
					if (entity.HasParent() == false and entityManager.IsEditorOnly(entity) == false and MatchesSearch(entity))
					{
						DrawEntityNode(entity, 0);
					}
				});

				if (mPendingRangeSelect != Gleam::InvalidEntity)
				{
					SelectRange(mRangeAnchor, mPendingRangeSelect, mPendingRangeAdditive ? SelectionMode::Add : SelectionMode::Replace);
					mPendingRangeSelect = Gleam::InvalidEntity;
				}

				if (mPendingToggleActive != Gleam::InvalidEntity)
				{
					ToggleActive(mPendingToggleActive);
					mPendingToggleActive = Gleam::InvalidEntity;
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

				if (ImGui::IsWindowHovered() and ImGui::IsAnyItemHovered() == false and ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					mSelectionSystem->ClearSelection();
					mRangeAnchor = Gleam::InvalidEntity;
				}
			}
			ImGui::EndChild();

			const ImVec2 dividerMin = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddLine(dividerMin, ImVec2(dividerMin.x + width, dividerMin.y), Widgets::ColorU32(Gleam::Theme::Divider));
			ImGui::Dummy(ImVec2(0.0f, 1.0f));
			Widgets::CaptionRow(*mFonts, "SINGLETONS", singletonCount, kSingletonHeaderHeight, 14.0f);

			ImGui::BeginChild("SingletonList", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
			DrawSingletonComponents();
			ImGui::EndChild();

			ImGui::PopStyleVar();
		}
		ImGui::End();
	});
}

WorldOutliner::RowState WorldOutliner::DrawRow(const char* label, const char* icon, const ImVec4& iconColor, bool selected, bool dimmed, float indent)
{
	constexpr float kIconSize = 16.0f;

	const ImVec2 lineStart = ImGui::GetCursorScreenPos();
	const float width = ImGui::GetContentRegionAvail().x;
	const ImVec2 rowMin(lineStart.x + kRowMargin, lineStart.y);
	const ImVec2 rowMax(lineStart.x + width - kRowMargin, lineStart.y + kRowHeight);

	ImGui::SetCursorScreenPos(rowMin);
	ImGui::SetNextItemAllowOverlap();
	ImGui::InvisibleButton("##Row", ImVec2(rowMax.x - rowMin.x, kRowHeight), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

	RowState state;
	state.clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
	state.rightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
	const bool hovered = ImGui::IsItemHovered();

	auto drawList = ImGui::GetWindowDrawList();
	if (selected)
	{
		drawList->AddRectFilled(rowMin, rowMax, Widgets::ColorU32(Gleam::Theme::Selected), 4.0f);
		drawList->AddRectFilled(rowMin, ImVec2(rowMin.x + 2.0f, rowMax.y), Widgets::ColorU32(Gleam::Theme::Accent), 4.0f, ImDrawFlags_RoundCornersLeft);
	}
	else if (hovered)
	{
		drawList->AddRectFilled(rowMin, rowMax, Widgets::ColorU32(Gleam::Theme::SectionHeader), 4.0f);
	}

	const float alpha = dimmed ? 0.45f : 1.0f;
	const ImVec2 iconMin(rowMin.x + 8.0f + indent, rowMin.y + (kRowHeight - kIconSize) * 0.5f);
	if (icon)
	{
		Widgets::DrawIcon(drawList, icon, ImVec2(iconMin.x + kIconSize * 0.5f, iconMin.y + kIconSize * 0.5f), ImVec4(iconColor.x, iconColor.y, iconColor.z, iconColor.w * alpha));
	}
	else
	{
		drawList->AddRectFilled(iconMin, ImVec2(iconMin.x + kIconSize, iconMin.y + kIconSize), Widgets::ColorU32(iconColor, alpha), 3.0f);
	}

	const float textX = iconMin.x + kIconSize + 10.0f;
	drawList->PushClipRect(ImVec2(textX, rowMin.y), ImVec2(rowMax.x - 30.0f, rowMax.y), true);
	drawList->AddText(ImVec2(textX, rowMin.y + (kRowHeight - ImGui::GetFontSize()) * 0.5f), Widgets::ColorU32(selected ? Gleam::Theme::Text : Gleam::Theme::TextSecondary, alpha), label);
	drawList->PopClipRect();

	return state;
}

bool WorldOutliner::MatchesSearch(const Gleam::Entity& entity) const
{
	if (mSearch[0] == '\0' or ImStristr(entity.GetName().c_str(), nullptr, mSearch, nullptr) != nullptr)
	{
		return true;
	}

	for (const auto& childHandle : entity.GetChildren())
	{
		if (childHandle != Gleam::InvalidEntity and MatchesSearch(entity.GetChildEntity(childHandle)))
		{
			return true;
		}
	}
	return false;
}

void WorldOutliner::DrawEntityNode(const Gleam::Entity& entity, uint32_t depth)
{
	const auto& entityManager = mEditWorld->GetEntityManager();
	auto handle = entity.GetHandle();
	const auto& children = entity.GetChildren();
	const bool hasChildren = children.empty() == false;
	const bool searching = mSearch[0] != '\0';

	ImGui::PushID(static_cast<int>(static_cast<uint32_t>(handle)));
	mVisibleEntities.push_back(handle);

	auto storage = ImGui::GetStateStorage();
	const ImGuiID openID = ImGui::GetID("##Open");
	bool open = searching or storage->GetBool(openID, true);

	const float depthIndent = depth * kIndentWidth;
	const float indent = depthIndent + (hasChildren ? kChevronWidth : 0.0f);
	const auto kind = Widgets::GetEntityKind(entityManager, handle);

	const ImVec2 lineStart = ImGui::GetCursorScreenPos();
	const auto row = DrawRow(entity.GetName().c_str(), ICON_LC_BOX, kind.color, mSelectionSystem->IsSelected(handle), entity.IsActive() == false, indent);

	if (row.clicked)
	{
		HandleSelectionInput(handle);
	}

	if (ImGui::BeginPopupContextItem("##EntityContext"))
	{
		if (ImGui::MenuItem("Destroy Entity"))
		{
			mPendingDestroy = handle;
		}
		ImGui::EndPopup();
	}

	const float rowRight = lineStart.x + ImGui::GetContentRegionAvail().x - kRowMargin;
	auto drawList = ImGui::GetWindowDrawList();

	if (hasChildren)
	{
		const ImVec2 chevronMin(lineStart.x + kRowMargin + 4.0f + depthIndent, lineStart.y);
		ImGui::SetCursorScreenPos(chevronMin);
		if (ImGui::InvisibleButton("##Toggle", ImVec2(kChevronWidth, kRowHeight)) and searching == false)
		{
			open = not open;
			storage->SetBool(openID, open);
		}
		Widgets::DrawIcon(drawList, open ? ICON_LC_CHEVRON_DOWN : ICON_LC_CHEVRON_RIGHT, ImVec2(chevronMin.x + kChevronWidth * 0.5f, chevronMin.y + kRowHeight * 0.5f), Gleam::Theme::TextDim);
	}

	constexpr float kEyeSize = 22.0f;
	const ImVec2 eyeMin(rowRight - 4.0f - kEyeSize, lineStart.y + (kRowHeight - kEyeSize) * 0.5f);
	ImGui::SetCursorScreenPos(eyeMin);
	if (ImGui::InvisibleButton("##Active", ImVec2(kEyeSize, kEyeSize)))
	{
		mPendingToggleActive = handle;
	}
	const bool eyeHovered = ImGui::IsItemHovered();
	Widgets::DrawIcon(drawList, entity.IsActive() ? ICON_LC_EYE : ICON_LC_EYE_OFF, ImVec2(eyeMin.x + kEyeSize * 0.5f, eyeMin.y + kEyeSize * 0.5f), eyeHovered ? Gleam::Theme::TextSecondary : Gleam::Theme::TextDim);

	ImGui::SetCursorScreenPos(lineStart);
	ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, kRowHeight));

	if (open and hasChildren)
	{
		for (const auto& childHandle : children)
		{
			if (childHandle == Gleam::InvalidEntity)
			{
				continue;
			}

			const auto& child = entity.GetChildEntity(childHandle);
			if (MatchesSearch(child))
			{
				DrawEntityNode(child, depth + 1);
			}
		}
	}

	ImGui::PopID();
}

void WorldOutliner::ToggleActive(Gleam::EntityHandle handle)
{
	auto& entity = mEditWorld->GetEntityManager().GetComponent<Gleam::Entity>(handle);
	mUndoSystem->BeginEntityTransaction({ handle });
	entity.SetActive(not entity.IsActive());
	mUndoSystem->EndTransaction();
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
		if (classDesc.Guid() == Gleam::Reflection::GetClass<Gleam::Entity>().Guid())
		{
			return;
		}

		auto componentName = ReflectionUtils::ResolveDisplayName(classDesc);
		uint32_t componentID = classDesc.TypeHash();

		char label[64];
		std::memcpy(label, componentName.data(), componentName.size());
		label[componentName.size()] = '\0';

		if (mSearch[0] != '\0' and ImStristr(label, nullptr, mSearch, nullptr) == nullptr)
		{
			return;
		}

		ImGui::PushID(static_cast<int>(componentID));
		if (DrawRow(label, nullptr, Gleam::Theme::Purple, mSelectionSystem->GetSelectedSingleton() == componentID, false, 0.0f).clicked)
		{
			mSelectionSystem->SelectSingleton(componentID);
		}
		ImGui::PopID();
	});
}
