#pragma once
#include "View/View.h"
#include "IO/Path.h"
#include "Container/Array.h"

namespace GEditor {

class EAssetManager;
class AssetImportRequest;
struct EditorFonts;

class AssetImportDialog final : public View
{
public:

	AssetImportDialog(EAssetManager* assetManager);

	~AssetImportDialog();

	virtual void OnCreate(Gleam::Application* app) override;

	virtual void Render(Gleam::ImGuiRenderer* imgui) override;

	void Enqueue(const Gleam::TArray<Gleam::Path>& files, const Gleam::Path& outputDirectory);

private:

	struct PendingImport
	{
		AssetImportRequest* request;
		Gleam::Path outputDirectory;
	};

	void DrawRequest();

	void ImportFront();

	void ClearQueue();

	uint32_t CountMatching(const PendingImport& pending) const;

	EAssetManager* mAssetManager;

	const EditorFonts* mFonts = nullptr;

	Gleam::TArray<PendingImport> mQueue;

	uint32_t mTotalCount = 0u;

	bool mApplyToMatching = false;

};

} // namespace GEditor
