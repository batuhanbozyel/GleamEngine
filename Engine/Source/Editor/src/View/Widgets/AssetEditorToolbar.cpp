#include "AssetEditorToolbar.h"
#include "EditorWidgets.h"
#include "EAssets/EAssetManager.h"
#include "View/EditorFonts.h"
#include "View/GleamTheme.h"
#include "View/IconsLucide.h"

#include <imgui.h>

#include <cstring>

using namespace GEditor;

void AssetEditorToolbar::Reset(const Gleam::TString& name)
{
	mName = name;
	mEditing = false;
}

bool AssetEditorToolbar::Draw(const EditorFonts& fonts, const EAssetManager& assetManager, const Gleam::Path& directory, const AssetItem& asset, bool& dirty, bool canSave)
{
	if (mEditing == false)
	{
		const auto length = Gleam::Math::Min(mName.size(), sizeof(mNameBuffer) - 1);
		std::memcpy(mNameBuffer, mName.c_str(), length);
		mNameBuffer[length] = '\0';
	}

	constexpr float kNameWidth = 280.0f;
	ImGui::PushFont(fonts.semiBold);
	ImGui::SetNextItemWidth(kNameWidth);
	ImGui::InputText("##AssetName", mNameBuffer, sizeof(mNameBuffer), ImGuiInputTextFlags_AutoSelectAll);
	ImGui::PopFont();
	mEditing = ImGui::IsItemActive();
	if (ImGui::IsItemDeactivatedAfterEdit() and mName != Gleam::TStringView(mNameBuffer))
	{
		mName = Gleam::TString(static_cast<const char*>(mNameBuffer));
		dirty = true;
	}

	const bool validName = assetManager.IsValidAssetName(directory, asset, mEditing ? Gleam::TStringView(mNameBuffer) : Gleam::TStringView(mName));
	if (validName == false)
	{
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextColored(Gleam::Theme::Error, "Name is empty, contains a path separator or is already taken");
	}

	if (dirty)
	{
		ImGui::SameLine();
		ImGui::TextColored(Gleam::Theme::TextDim, "(modified)");
	}

	constexpr float kSaveWidth = 90.0f;
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - kSaveWidth);
	ImGui::BeginDisabled(dirty == false or validName == false or canSave == false);
	const bool save = Widgets::AccentButton(ICON_LC_SAVE "  Save", ImVec2(kSaveWidth, ImGui::GetFrameHeight()));
	ImGui::EndDisabled();
	ImGui::Separator();
	return save;
}

const Gleam::TString& AssetEditorToolbar::GetName() const
{
	return mName;
}
