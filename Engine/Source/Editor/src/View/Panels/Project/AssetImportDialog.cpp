#include "AssetImportDialog.h"
#include "EAssets/EAssetManager.h"
#include "EAssets/AssetImportRequest.h"
#include "EAssets/MeshSource.h"
#include "EAssets/TextureSource.h"
#include "View/Widgets/PropertyDrawer.h"
#include "View/Widgets/EditorWidgets.h"
#include "View/ViewStack.h"
#include "View/EditorFonts.h"
#include "View/GleamTheme.h"
#include "Utils/ReflectionUtils.h"

#include "Core/Application.h"
#include "Renderer/Renderers/ImGuiRenderer.h"

#include <Editor.Reflection.generated.h>

#include <imgui.h>

#include <cstdio>

using namespace GEditor;

static constexpr const char* kPopupName = "Import Asset";
static constexpr float kDialogWidth = 480.0f;
static constexpr ImVec2 kButtonSize = ImVec2(96.0f, 32.0f);

AssetImportDialog::AssetImportDialog(EAssetManager* assetManager)
	: mAssetManager(assetManager)
{

}

AssetImportDialog::~AssetImportDialog()
{
	ClearQueue();
}

void AssetImportDialog::OnCreate(Gleam::Application* app)
{
	mFonts = &app->GetSubsystem<ViewStack>()->GetFonts();
}

void AssetImportDialog::Enqueue(const Gleam::TArray<Gleam::Path>& files, const Gleam::Path& outputDirectory)
{
	if (mQueue.empty())
	{
		mTotalCount = 0u;
	}

	for (const auto& file : files)
	{
		AssetImportRequest* request = nullptr;
		const auto extension = file.Extension();
		if (extension == L".gltf")
		{
			request = new TAssetImportRequest<MeshSource>(file);
		}
		else if (extension == L".png" or extension == L".jpg" or extension == L".jpeg" or extension == L".tga" or extension == L".hdr")
		{
			request = new TAssetImportRequest<TextureSource>(file);
		}
		else
		{
			GLEAM_WARN("Unsupported asset type: {}", file.String());
			continue;
		}
		mQueue.push_back(PendingImport{ .request = request, .outputDirectory = outputDirectory });
		mTotalCount++;
	}
}

void AssetImportDialog::Render(Gleam::ImGuiRenderer* imgui)
{
	if (mQueue.empty())
	{
		return;
	}

	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		if (ImGui::IsPopupOpen(kPopupName) == false)
		{
			ImGui::OpenPopup(kPopupName);
		}

		// Zero height auto-fits every frame so expanding nested settings grows the dialog
		ImGui::SetNextWindowSize(ImVec2(kDialogWidth, 0.0f), ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 16.0f));
		const bool open = ImGui::BeginPopupModal(kPopupName, nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize);
		ImGui::PopStyleVar();
		if (open)
		{
			DrawRequest();
			if (mQueue.empty())
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	});
}

void AssetImportDialog::DrawRequest()
{
	const auto& pending = mQueue.front();
	const auto& settingsClass = pending.request->SettingsClass();

	const Gleam::TString filename = pending.request->Source().Filename();
	ImGui::PushFont(mFonts->semiBold);
	ImGui::TextUnformatted(filename.c_str());
	ImGui::PopFont();

	const auto typeName = ReflectionUtils::ResolveDisplayName(settingsClass);
	const uint32_t position = mTotalCount - static_cast<uint32_t>(mQueue.size()) + 1u;
	ImGui::PushFont(mFonts->caption);
	ImGui::TextColored(Gleam::Theme::TextDim, "%.*s", static_cast<int>(typeName.size()), typeName.data());
	if (mTotalCount > 1u)
	{
		char counter[32];
		std::snprintf(counter, sizeof(counter), "%u / %u", position, mTotalCount);
		ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(counter).x);
		ImGui::TextColored(Gleam::Theme::TextDim, "%s", counter);
	}
	ImGui::PopFont();

	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.0f, 4.0f));

	ImGui::PushID(position);
	PropertyDrawer::DrawClassFields(pending.request->Settings(), settingsClass, ImGui::GetContentRegionAvail().x * 0.4f);
	ImGui::PopID();

	ImGui::Dummy(ImVec2(0.0f, 4.0f));
	ImGui::Separator();

	if (const auto matching = CountMatching(pending); matching > 0u)
	{
		char label[64];
		std::snprintf(label, sizeof(label), "Apply to %u remaining files of this type", matching);
		ImGui::Checkbox(label, &mApplyToMatching);
	}

	const float buttonsWidth = kButtonSize.x * 3.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f;
	ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - buttonsWidth);
	ImGui::PushFont(mFonts->semiBold);
	if (Widgets::OutlineButton("Cancel", kButtonSize))
	{
		ClearQueue();
		mApplyToMatching = false;
	}
	ImGui::SameLine();
	if (Widgets::OutlineButton("Skip", kButtonSize))
	{
		delete mQueue.front().request;
		mQueue.erase(mQueue.begin());
		mApplyToMatching = false;
	}
	ImGui::SameLine();
	if (Widgets::AccentButton("Import", kButtonSize))
	{
		ImportFront();
	}
	ImGui::PopFont();
}

void AssetImportDialog::ImportFront()
{
	const auto pending = mQueue.front();
	mQueue.erase(mQueue.begin());
	pending.request->Import(mAssetManager, pending.outputDirectory);

	if (mApplyToMatching)
	{
		const auto typeHash = pending.request->SettingsClass().TypeHash();
		for (auto it = mQueue.begin(); it != mQueue.end();)
		{
			if (it->request->SettingsClass().TypeHash() == typeHash)
			{
				it->request->CopySettings(*pending.request);
				it->request->Import(mAssetManager, it->outputDirectory);
				delete it->request;
				it = mQueue.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
	delete pending.request;
	mApplyToMatching = false;
}

void AssetImportDialog::ClearQueue()
{
	for (const auto& pending : mQueue)
	{
		delete pending.request;
	}
	mQueue.clear();
}

uint32_t AssetImportDialog::CountMatching(const PendingImport& pending) const
{
	const auto typeHash = pending.request->SettingsClass().TypeHash();
	uint32_t count = 0u;
	for (size_t i = 1; i < mQueue.size(); ++i)
	{
		if (mQueue[i].request->SettingsClass().TypeHash() == typeHash)
		{
			count++;
		}
	}
	return count;
}
