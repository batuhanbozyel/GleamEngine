#pragma once
#include "View/View.h"
#include "EAssets/AssetRegistry.h"
#include "View/Widgets/AssetEditorToolbar.h"
#include "Renderer/TextureDescriptor.h"

namespace GEditor {

class EAssetManager;
struct EditorFonts;

class RenderTextureEditor final : public View
{
public:

	RenderTextureEditor(EAssetManager* assetManager);

	virtual void OnCreate(Gleam::Application* app) override;

	virtual void Render(Gleam::ImGuiRenderer* imgui) override;

	void Open(const AssetItem& asset, const Gleam::Path& file);

	void OnAssetRenamed(const AssetItem& asset);

	void OnAssetRemoved(const Gleam::Guid& guid);

private:

	void Save();

	void DrawProperties();

	EAssetManager* mAssetManager;

	const EditorFonts* mFonts = nullptr;

	AssetEditorToolbar mToolbar;

	AssetItem mAsset;

	Gleam::Path mFile;

	Gleam::RenderTextureDescriptor mDescriptor;

	bool mIsOpen = false;

	bool mDirty = false;

};

} // namespace GEditor
