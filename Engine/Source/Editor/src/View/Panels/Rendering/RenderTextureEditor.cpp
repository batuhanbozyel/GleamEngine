#include "RenderTextureEditor.h"
#include "EAssets/EAssetManager.h"
#include "View/ViewStack.h"
#include "View/EditorFonts.h"
#include "View/Widgets/PropertyDrawer.h"

#include "Core/Globals.h"
#include "Core/Application.h"
#include "Assets/AssetManager.h"
#include "Renderer/Renderers/ImGuiRenderer.h"

#include <Runtime.Reflection.generated.h>

#include <imgui.h>

using namespace GEditor;

static constexpr float kColumnWidth = 110.0f;

RenderTextureEditor::RenderTextureEditor(EAssetManager* assetManager)
	: mAssetManager(assetManager)
{

}

void RenderTextureEditor::OnCreate(Gleam::Application* app)
{
	mFonts = &app->GetSubsystem<ViewStack>()->GetFonts();
}

void RenderTextureEditor::Open(const AssetItem& asset, const Gleam::Path& file)
{
	if (mAssetManager->LoadAsset<Gleam::RenderTextureDescriptor>(file, mDescriptor) == false)
	{
		return;
	}

	mToolbar.Reset(asset.name);
	mAsset = asset;
	mFile = file;
	mDirty = false;
	mIsOpen = true;
}

void RenderTextureEditor::OnAssetRenamed(const AssetItem& asset)
{
	if (mIsOpen and mAsset.reference == asset.reference)
	{
		mAsset = asset;
		mToolbar.Reset(asset.name);
	}
}

void RenderTextureEditor::OnAssetRemoved(const Gleam::Guid& guid)
{
	if (mIsOpen and mAsset.reference.guid == guid)
	{
		mIsOpen = false;
		mDirty = false;
	}
}

void RenderTextureEditor::Render(Gleam::ImGuiRenderer* imgui)
{
	if (mIsOpen == false)
	{
		return;
	}

	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		ImGui::SetNextWindowSize(ImVec2(520.0f, 360.0f), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Render Texture###RenderTextureEditor", &mIsOpen))
		{
			if (mToolbar.Draw(*mFonts, *mAssetManager, mFile.Parent(), mAsset, mDirty, true))
			{
				Save();
			}
			DrawProperties();
		}
		ImGui::End();
	});
}

void RenderTextureEditor::Save()
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

void RenderTextureEditor::DrawProperties()
{
	PropertyDrawer::BeginEditTracking();
	PropertyDrawer::DrawClassFields(&mDescriptor, Gleam::Reflection::GetClass<Gleam::RenderTextureDescriptor>(), kColumnWidth);

	// A texture needs at least one texel
	mDescriptor.size.width = Gleam::Math::Max(Gleam::Math::Round(mDescriptor.size.width), 1.0f);
	mDescriptor.size.height = Gleam::Math::Max(Gleam::Math::Round(mDescriptor.size.height), 1.0f);

	if (PropertyDrawer::EditDirty() or PropertyDrawer::EditCommitted())
	{
		mDirty = true;
	}
}
