#pragma once
#include "View/View.h"
#include "EAssets/AssetRegistry.h"
#include "View/Widgets/AssetEditorToolbar.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderPipelineDescriptor.h"

namespace GEditor {

class EAssetManager;
class EditorInputSystem;
struct EditorFonts;

class RenderPipelineEditor final : public View
{
public:

	RenderPipelineEditor(EAssetManager* assetManager);

	virtual void OnCreate(Gleam::Application* app) override;

	virtual void Render(Gleam::ImGuiRenderer* imgui) override;

	void Open(const AssetItem& asset, const Gleam::Path& file);

	void OnAssetRenamed(const AssetItem& asset);

	void OnAssetRemoved(const Gleam::Guid& guid);

private:

	struct RowEdits
	{
		int32_t removed = -1;
		int32_t moveFrom = -1;
		int32_t moveTo = -1;
	};

	void Save();

	void DrawRenderers();

	void DrawFilterRow();

	bool DrawStageHeader(const char* label, uint32_t count, bool collapsed);

	void DrawRow(int32_t index, const Gleam::Reflection::ClassDescription& classDesc, RowEdits& edits);

	void DrawEmptyStage(Gleam::RenderStage stage);

	void DrawFooter(uint32_t rendererCount, uint32_t sharedCount);

	void OpenRendererPicker(int32_t stage);

	void DrawRendererPicker();

	EAssetManager* mAssetManager;

	EditorInputSystem* mEditorInput = nullptr;

	const EditorFonts* mFonts = nullptr;

	AssetEditorToolbar mToolbar;

	AssetItem mAsset;

	Gleam::Path mFile;

	Gleam::RenderPipelineDescriptor mDescriptor;

	char mFilter[128] = "";

	char mSearch[128] = "";

	uint32_t mCollapsedStages = 0u;

	int32_t mSelectedRow = -1;

	// -1 lists every stage
	int32_t mPickerStage = -1;

	bool mPickerRequested = false;

	bool mReadOnly = false;

	bool mIsOpen = false;

	bool mDirty = false;

};

} // namespace GEditor
