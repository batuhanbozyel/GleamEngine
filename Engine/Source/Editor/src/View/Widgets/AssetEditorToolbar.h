#pragma once
#include "EAssets/AssetRegistry.h"

#include "IO/Path.h"
#include "Container/String.h"

namespace GEditor {

class EAssetManager;
struct EditorFonts;

class AssetEditorToolbar
{
public:

	void Reset(const Gleam::TString& name);

	bool Draw(const EditorFonts& fonts, const EAssetManager& assetManager, const Gleam::Path& directory, const AssetItem& asset, bool& dirty, bool canSave);

	const Gleam::TString& GetName() const;

private:

	Gleam::TString mName;

	char mNameBuffer[128] = "";

	bool mEditing = false;

};

} // namespace GEditor
