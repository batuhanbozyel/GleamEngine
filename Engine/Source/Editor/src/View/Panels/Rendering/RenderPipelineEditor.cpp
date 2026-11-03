#include "RenderPipelineEditor.h"
#include "EAssets/EAssetManager.h"
#include "EInput/EditorInputSystem.h"
#include "View/ViewStack.h"
#include "View/EditorFonts.h"
#include "View/GleamTheme.h"
#include "View/IconsLucide.h"
#include "View/Widgets/EditorWidgets.h"
#include "Utils/ReflectionUtils.h"

#include "Core/Globals.h"
#include "Core/Application.h"
#include "Assets/AssetManager.h"
#include "Renderer/Renderers/ImGuiRenderer.h"
#include "Serialization/ReflectionUtils.h"

#include <Runtime.Reflection.generated.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <EASTL/algorithm.h>
#include <cctype>
#include <cstdio>
#include <cstring>

using namespace GEditor;

static constexpr float kRowHeight = 30.0f;
static constexpr float kHeaderHeight = 30.0f;
static constexpr float kEmptyHeight = 40.0f;
static constexpr float kFooterHeight = 30.0f;
static constexpr float kIndent = 22.0f;
static constexpr const char* kRowPayload = "GLEAM_RENDERER_ROW";

template<size_t N>
static const char* DisplayName(const Gleam::Reflection::ClassDescription& classDesc, char (&buffer)[N])
{
	const auto name = ReflectionUtils::ResolveDisplayName(classDesc);
	const auto length = Gleam::Math::Min(name.size(), N - 1);
	std::memcpy(buffer, name.data(), length);
	buffer[length] = '\0';
	return buffer;
}

template<size_t N>
static const char* UpperCase(Gleam::TStringView text, char (&buffer)[N])
{
	const auto length = Gleam::Math::Min(text.size(), N - 1);
	for (size_t i = 0; i < length; ++i)
	{
		buffer[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(text[i])));
	}
	buffer[length] = '\0';
	return buffer;
}

static const Gleam::Reflection::Attribute::Renderer* RendererAttribute(const Gleam::Reflection::ClassDescription& classDesc)
{
	return classDesc.GetAttribute<Gleam::Reflection::Attribute::Renderer>();
}

static float DrawPill(ImDrawList* drawList, ImFont* font, const ImVec2& pos, float height, const char* text, const ImVec4& color)
{
	ImGui::PushFont(font);
	const ImVec2 textSize = ImGui::CalcTextSize(text);
	const float width = textSize.x + 12.0f;
	drawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), Widgets::ColorU32(color, 0.14f), height * 0.5f);
	drawList->AddText(ImVec2(pos.x + 6.0f, IM_ROUND(pos.y + (height - textSize.y) * 0.5f)), Widgets::ColorU32(color), text);
	ImGui::PopFont();
	return width;
}

RenderPipelineEditor::RenderPipelineEditor(EAssetManager* assetManager)
	: mAssetManager(assetManager)
{

}

void RenderPipelineEditor::OnCreate(Gleam::Application* app)
{
	mFonts = &app->GetSubsystem<ViewStack>()->GetFonts();
	mEditorInput = app->GetSubsystem<EditorInputSystem>();
}

void RenderPipelineEditor::Open(const AssetItem& asset, const Gleam::Path& file)
{
	if (mAssetManager->LoadAsset<Gleam::RenderPipelineDescriptor>(file, mDescriptor) == false)
	{
		return;
	}

	mToolbar.Reset(asset.name);
	mAsset = asset;
	mFile = file;
	mReadOnly = mAssetManager->IsBuiltinAsset(file);
	mFilter[0] = '\0';
	mSelectedRow = -1;
	mDirty = false;
	mIsOpen = true;
}

void RenderPipelineEditor::OnAssetRenamed(const AssetItem& asset)
{
	if (mIsOpen and mAsset.reference == asset.reference)
	{
		mAsset = asset;
		mToolbar.Reset(asset.name);
	}
}

void RenderPipelineEditor::OnAssetRemoved(const Gleam::Guid& guid)
{
	if (mIsOpen and mAsset.reference.guid == guid)
	{
		mIsOpen = false;
		mDirty = false;
	}
}

void RenderPipelineEditor::Render(Gleam::ImGuiRenderer* imgui)
{
	if (mIsOpen == false)
	{
		return;
	}

	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		ImGui::SetNextWindowSize(ImVec2(520.0f, 560.0f), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Render Pipeline###RenderPipelineEditor", &mIsOpen))
		{
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
			{
				mEditorInput->RequestPanelActions();
			}

			if (mToolbar.Draw(*mFonts, *mAssetManager, mFile.Parent(), mAsset, mDirty, mReadOnly == false))
			{
				Save();
			}
			if (mReadOnly)
			{
				ImGui::TextColored(Gleam::Theme::TextDim, "Built-in pipelines are recreated on every launch, create a new pipeline to customize one");
			}

			ImGui::BeginDisabled(mReadOnly);
			DrawRenderers();
			ImGui::EndDisabled();
		}
		ImGui::End();
	});
}

void RenderPipelineEditor::Save()
{
	auto item = mAsset;
	item.name = mToolbar.GetName();
	if (mAssetManager->SaveAsset(mFile.Parent(), mDescriptor, item))
	{
		mAsset = item;
		mDirty = false;

		Gleam::Globals::GameInstance->GetSubsystem<Gleam::AssetManager>()->Reload(mAsset.reference);
	}
}

void RenderPipelineEditor::DrawRenderers()
{
	DrawFilterRow();

	auto database = Gleam::Reflection::IDatabase::GetInstance();
	const bool filtering = mFilter[0] != '\0';
	auto matchesFilter = [this, filtering](const Gleam::Reflection::ClassDescription& classDesc)
	{
		char nameBuffer[64];
		return filtering == false or ImStristr(DisplayName(classDesc, nameBuffer), nullptr, mFilter, nullptr) != nullptr;
	};

	RowEdits edits;
	uint32_t rendererCount = 0u;
	uint32_t sharedCount = 0u;
	ImGui::BeginChild("##Renderers", ImVec2(0.0f, ImGui::GetContentRegionAvail().y - kFooterHeight));
	char stageBuffer[64];
	for (const auto& stageItem : Gleam::Reflection::GetEnum<Gleam::RenderStage>().Cases())
	{
		const auto stage = static_cast<Gleam::RenderStage>(stageItem.Value());
		uint32_t count = 0u;
		uint32_t matches = 0u;
		for (const auto& guid : mDescriptor.renderers)
		{
			const auto classDesc = database->GetClass(guid);
			if (classDesc and RendererAttribute(*classDesc)->stage == stage)
			{
				++count;
				matches += matchesFilter(*classDesc) ? 1u : 0u;
				sharedCount += RendererAttribute(*classDesc)->shared ? 1u : 0u;
			}
		}
		rendererCount += count;
		if (filtering and matches == 0u)
		{
			continue;
		}

		// Filtering expands every stage so matches are never hidden
		const uint32_t stageBit = 1u << static_cast<uint32_t>(stage);
		const bool collapsed = filtering == false and (mCollapsedStages & stageBit) != 0u;
		ImGui::PushID(static_cast<int>(stage));
		if (DrawStageHeader(UpperCase(Gleam::ReflectionUtils::ResolveDisplayName(stageItem), stageBuffer), count, collapsed))
		{
			mCollapsedStages ^= stageBit;
		}
		if (collapsed == false)
		{
			if (count == 0u)
			{
				DrawEmptyStage(stage);
			}
			for (int32_t i = 0; i < static_cast<int32_t>(mDescriptor.renderers.size()); ++i)
			{
				const auto classDesc = database->GetClass(mDescriptor.renderers[i]);
				if (classDesc and RendererAttribute(*classDesc)->stage == stage and matchesFilter(*classDesc))
				{
					DrawRow(i, *classDesc, edits);
				}
			}
		}
		ImGui::PopID();
	}

	// Classes deleted from code stay listed so they can be removed from the asset
	for (int32_t i = 0; i < static_cast<int32_t>(mDescriptor.renderers.size()); ++i)
	{
		if (database->GetClass(mDescriptor.renderers[i]) == nullptr)
		{
			const float buttonSize = ImGui::GetFrameHeight();
			ImGui::PushID(i);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kIndent);
			ImGui::AlignTextToFramePadding();
			ImGui::TextColored(Gleam::Theme::Error, "Missing renderer %s", mDescriptor.renderers[i].ToString().c_str());
			ImGui::SameLine(ImGui::GetContentRegionMax().x - buttonSize);
			if (Widgets::IconButton("##Remove", ICON_LC_TRASH_2, ImVec2(buttonSize, buttonSize)))
			{
				edits.removed = i;
			}
			ImGui::PopID();
		}
	}

	if (mPickerRequested)
	{
		mSearch[0] = '\0';
		ImGui::OpenPopup("##RendererPicker");
		mPickerRequested = false;
	}
	DrawRendererPicker();
	ImGui::EndChild();

	if (mReadOnly == false and mSelectedRow >= 0 and ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) and mEditorInput->GetAction(mEditorInput->GetPanelActions().remove).performedThisFrame)
	{
		edits.removed = mSelectedRow;
	}

	DrawFooter(rendererCount, sharedCount);

	if (edits.removed >= 0)
	{
		mDescriptor.renderers.erase(mDescriptor.renderers.begin() + edits.removed);
		mSelectedRow = -1;
		mDirty = true;
	}
	else if (edits.moveFrom >= 0 and edits.moveFrom != edits.moveTo)
	{
		const auto guid = mDescriptor.renderers[edits.moveFrom];
		mDescriptor.renderers.erase(mDescriptor.renderers.begin() + edits.moveFrom);
		mDescriptor.renderers.insert(mDescriptor.renderers.begin() + edits.moveTo, guid);
		mSelectedRow = edits.moveTo;
		mDirty = true;
	}
}

void RenderPipelineEditor::DrawFilterRow()
{
	constexpr float kAddWidth = 132.0f;
	constexpr float kHeight = 30.0f;
	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	Widgets::SearchField("##RendererFilter", "Filter renderers", mFilter, sizeof(mFilter), ImGui::GetContentRegionAvail().x - kAddWidth - spacing, kHeight);
	ImGui::SameLine();
	if (Widgets::OutlineButton(ICON_LC_PLUS "  Add renderer", ImVec2(kAddWidth, kHeight)))
	{
		OpenRendererPicker(-1);
	}
	ImGui::Spacing();
}

bool RenderPipelineEditor::DrawStageHeader(const char* label, uint32_t count, bool collapsed)
{
	const ImVec2 min = ImGui::GetCursorScreenPos();
	const bool clicked = ImGui::InvisibleButton("##StageHeader", ImVec2(ImGui::GetContentRegionAvail().x, kHeaderHeight));
	const bool hovered = ImGui::IsItemHovered();
	const float centerY = min.y + kHeaderHeight * 0.5f;
	auto drawList = ImGui::GetWindowDrawList();

	const auto& color = hovered ? Gleam::Theme::TextSecondary : Gleam::Theme::TextDim;
	Widgets::DrawIcon(drawList, collapsed ? ICON_LC_CHEVRON_RIGHT : ICON_LC_CHEVRON_DOWN, ImVec2(min.x + kIndent * 0.5f, centerY), color);

	ImGui::PushFont(mFonts->microBold);
	const ImVec2 labelSize = ImGui::CalcTextSize(label);
	drawList->AddText(ImVec2(min.x + kIndent, IM_ROUND(centerY - labelSize.y * 0.5f)), Widgets::ColorU32(color), label);
	ImGui::PopFont();

	char countText[16];
	std::snprintf(countText, sizeof(countText), "%u", count);
	constexpr float kPillHeight = 18.0f;
	DrawPill(drawList, mFonts->mono, ImVec2(min.x + kIndent + labelSize.x + 8.0f, centerY - kPillHeight * 0.5f), kPillHeight, countText, Gleam::Theme::TextDim);
	return clicked;
}

void RenderPipelineEditor::DrawRow(int32_t index, const Gleam::Reflection::ClassDescription& classDesc, RowEdits& edits)
{
	char nameBuffer[64];
	const char* name = DisplayName(classDesc, nameBuffer);
	const bool shared = RendererAttribute(classDesc)->shared;
	const bool selected = mSelectedRow == index;

	ImGui::PushID(index);
	const ImVec2 min(ImGui::GetCursorScreenPos().x + kIndent, ImGui::GetCursorScreenPos().y);
	const ImVec2 max(min.x + ImGui::GetContentRegionAvail().x - kIndent, min.y + kRowHeight);
	ImGui::SetCursorScreenPos(min);
	ImGui::SetNextItemAllowOverlap();
	if (ImGui::InvisibleButton("##Row", ImVec2(max.x - min.x, kRowHeight)))
	{
		mSelectedRow = index;
	}
	const bool hovered = ImGui::IsItemHovered();
	ImGui::OpenPopupOnItemClick("##RowActions", ImGuiPopupFlags_MouseButtonRight);

	if (ImGui::BeginDragDropSource())
	{
		ImGui::SetDragDropPayload(kRowPayload, &index, sizeof(index));
		ImGui::TextUnformatted(name);
		ImGui::EndDragDropSource();
	}

	auto drawList = ImGui::GetWindowDrawList();
	if (selected or hovered)
	{
		drawList->AddRectFilled(min, max, Widgets::ColorU32(selected ? Gleam::Theme::Selected : Gleam::Theme::ControlHover, selected ? 1.0f : 0.6f), 6.0f);
	}

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kRowPayload, ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
		{
			const int32_t from = *static_cast<const int32_t*>(payload->Data);
			const auto fromClass = Gleam::Reflection::IDatabase::GetInstance()->GetClass(mDescriptor.renderers[from]);
			if (from != index and RendererAttribute(*fromClass)->stage == RendererAttribute(classDesc)->stage)
			{
				// Rows dragged down land after the target, rows dragged up land before it
				const float lineY = from < index ? max.y : min.y;
				drawList->AddLine(ImVec2(min.x, lineY), ImVec2(max.x, lineY), Widgets::ColorU32(Gleam::Theme::Accent), 2.0f);
				if (payload->IsDelivery())
				{
					edits.moveFrom = from;
					edits.moveTo = index;
				}
			}
		}
		ImGui::EndDragDropTarget();
	}

	const float centerY = min.y + kRowHeight * 0.5f;
	Widgets::DrawIcon(drawList, ICON_LC_GRIP_VERTICAL, ImVec2(min.x + 12.0f, centerY), hovered ? Gleam::Theme::TextMuted : Gleam::Theme::Border);
	const ImVec2 nameSize = ImGui::CalcTextSize(name);
	drawList->AddText(ImVec2(min.x + 28.0f, IM_ROUND(centerY - nameSize.y * 0.5f)), Widgets::ColorU32(Gleam::Theme::Text), name);

	constexpr float kButtonSize = 24.0f;
	const float right = max.x - 4.0f - kButtonSize;
	if (shared)
	{
		constexpr float kPillHeight = 20.0f;
		constexpr const char* kSharedText = ICON_LC_LINK " SHARED";
		ImGui::PushFont(mFonts->microBold);
		const float pillWidth = ImGui::CalcTextSize(kSharedText).x + 12.0f;
		ImGui::PopFont();
		DrawPill(drawList, mFonts->microBold, ImVec2(right - 8.0f - pillWidth, centerY - kPillHeight * 0.5f), kPillHeight, kSharedText, Gleam::Theme::Info);
	}

	ImGui::SetCursorScreenPos(ImVec2(right, centerY - kButtonSize * 0.5f));
	if (Widgets::IconButton("##Actions", ICON_LC_ELLIPSIS, ImVec2(kButtonSize, kButtonSize)))
	{
		ImGui::OpenPopup("##RowActions");
	}
	if (ImGui::BeginPopup("##RowActions"))
	{
		mSelectedRow = index;
		ImGui::PushFont(mFonts->semiBold);
		ImGui::TextUnformatted(name);
		ImGui::PopFont();
		ImGui::TextColored(Gleam::Theme::TextDim, shared ? "Shared renderer" : "Renderer");
		ImGui::Separator();
		ImGui::PushStyleColor(ImGuiCol_Text, Gleam::Theme::Error);
		const auto shortcut = mEditorInput->GetShortcutLabel(mEditorInput->GetPanelActions().remove);
		if (ImGui::MenuItem(ICON_LC_TRASH_2 "  Remove from pipeline", shortcut.c_str()))
		{
			edits.removed = index;
		}
		ImGui::PopStyleColor();
		ImGui::EndPopup();
	}

	ImGui::SetCursorScreenPos(ImVec2(min.x - kIndent, max.y));
	ImGui::Dummy(ImVec2(0.0f, 0.0f));
	ImGui::PopID();
}

void RenderPipelineEditor::DrawEmptyStage(Gleam::RenderStage stage)
{
	const ImVec2 cursor = ImGui::GetCursorScreenPos();
	const ImVec2 min(cursor.x + kIndent, cursor.y + 2.0f);
	const ImVec2 max(cursor.x + ImGui::GetContentRegionAvail().x, min.y + kEmptyHeight);
	auto drawList = ImGui::GetWindowDrawList();
	Widgets::DrawDashedRect(drawList, min, max, Widgets::ColorU32(Gleam::Theme::Border), 6.0f, 4.0f);

	constexpr const char* kMessage = "No renderers in this stage.  ";
	constexpr const char* kLink = "Add renderer";
	const ImVec2 messageSize = ImGui::CalcTextSize(kMessage);
	const ImVec2 linkSize = ImGui::CalcTextSize(kLink);
	const ImVec2 textPos(IM_ROUND((min.x + max.x - messageSize.x - linkSize.x) * 0.5f), IM_ROUND((min.y + max.y - messageSize.y) * 0.5f));
	drawList->AddText(textPos, Widgets::ColorU32(Gleam::Theme::TextDim), kMessage);

	const ImVec2 linkPos(textPos.x + messageSize.x, textPos.y);
	ImGui::SetCursorScreenPos(linkPos);
	if (ImGui::InvisibleButton("##AddToStage", linkSize))
	{
		OpenRendererPicker(static_cast<int32_t>(stage));
	}
	const bool hovered = ImGui::IsItemHovered();
	drawList->AddText(linkPos, Widgets::ColorU32(hovered ? Gleam::Theme::Text : Gleam::Theme::Info), kLink);
	if (hovered)
	{
		drawList->AddLine(ImVec2(linkPos.x, linkPos.y + linkSize.y), ImVec2(linkPos.x + linkSize.x, linkPos.y + linkSize.y), Widgets::ColorU32(Gleam::Theme::Text));
	}

	ImGui::SetCursorScreenPos(ImVec2(cursor.x, max.y + 6.0f));
	ImGui::Dummy(ImVec2(0.0f, 0.0f));
}

void RenderPipelineEditor::DrawFooter(uint32_t rendererCount, uint32_t sharedCount)
{
	ImGui::Separator();
	ImGui::PushFont(mFonts->footnote);
	ImGui::AlignTextToFramePadding();
	ImGui::TextColored(Gleam::Theme::TextDim, "%u renderers \xC2\xB7 %u shared", rendererCount, sharedCount);

	char hint[64];
	const auto shortcut = mEditorInput->GetShortcutLabel(mEditorInput->GetPanelActions().remove);
	std::snprintf(hint, sizeof(hint), "Drag to reorder \xC2\xB7 %s removes", shortcut.c_str());
	ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(hint).x);
	ImGui::TextColored(Gleam::Theme::TextDim, "%s", hint);
	ImGui::PopFont();
}

void RenderPipelineEditor::OpenRendererPicker(int32_t stage)
{
	mPickerStage = stage;
	mPickerRequested = true;
}

void RenderPipelineEditor::DrawRendererPicker()
{
	ImGui::SetNextWindowSize(ImVec2(320.0f, 360.0f));
	if (ImGui::BeginPopup("##RendererPicker") == false)
	{
		return;
	}

	if (ImGui::IsWindowAppearing())
	{
		ImGui::SetKeyboardFocusHere();
	}
	Widgets::SearchField("##RendererSearch", "Search renderers", mSearch, sizeof(mSearch), ImGui::GetContentRegionAvail().x);

	char nameBuffer[64];
	ImGui::BeginChild("##RendererPickerList");
	for (const auto& classDesc : Gleam::Reflection::IDatabase::GetInstance()->GetClasses())
	{
		const auto attribute = RendererAttribute(classDesc);
		if (attribute == nullptr or (mPickerStage >= 0 and static_cast<int32_t>(attribute->stage) != mPickerStage))
		{
			continue;
		}
		if (eastl::find(mDescriptor.renderers.begin(), mDescriptor.renderers.end(), classDesc.Guid()) != mDescriptor.renderers.end())
		{
			continue;
		}

		const char* name = DisplayName(classDesc, nameBuffer);
		if (mSearch[0] != '\0' and ImStristr(name, nullptr, mSearch, nullptr) == nullptr)
		{
			continue;
		}

		ImGui::PushID(static_cast<int>(classDesc.TypeHash()));
		ImGui::SetNextItemAllowOverlap();
		if (ImGui::Selectable(name, false, ImGuiSelectableFlags_None, ImVec2(0.0f, kRowHeight)))
		{
			mDescriptor.renderers.push_back(classDesc.Guid());
			mDirty = true;
			ImGui::CloseCurrentPopup();
		}

		const auto stageCase = Gleam::ReflectionUtils::FindCase(Gleam::Reflection::GetEnum<Gleam::RenderStage>(), static_cast<int64_t>(attribute->stage));
		const auto stageName = Gleam::ReflectionUtils::ResolveDisplayName(*stageCase);
		const float stageWidth = ImGui::CalcTextSize(stageName.data(), stageName.data() + stageName.size()).x;
		ImGui::SameLine(ImGui::GetContentRegionMax().x - stageWidth - 4.0f);
		ImGui::TextColored(Gleam::Theme::TextDim, "%.*s", static_cast<int>(stageName.size()), stageName.data());
		ImGui::PopID();
	}
	ImGui::EndChild();
	ImGui::EndPopup();
}
