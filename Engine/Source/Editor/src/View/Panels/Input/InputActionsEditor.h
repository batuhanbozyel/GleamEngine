#pragma once
#include "View/View.h"
#include "EAssets/AssetRegistry.h"
#include "Input/InputActionDescriptor.h"

namespace Gleam {
class InputUser;
} // namespace Gleam

namespace GEditor {

class EAssetManager;
class EditorInputSystem;
struct EditorFonts;

class InputActionsEditor final : public View
{
public:

	InputActionsEditor(EAssetManager* assetManager);

	virtual void OnCreate(Gleam::Application* app) override;

	virtual void Render(Gleam::ImGuiRenderer* imgui) override;

	void Open(const AssetItem& asset, const Gleam::Path& file);

	void OnAssetRenamed(const AssetItem& asset);

	void OnAssetRemoved(const Gleam::Guid& guid);

private:

	struct ListenTarget
	{
		int32_t binding = -1;
		uint32_t part = 0;
		bool armed = false;
	};

	void Save();

	void DrawToolbar();

	void DrawMaps();

	void DrawActions();

	void DrawProperties();

	void DrawActionProperties(Gleam::InputActionDescriptor& action);

	bool DrawBinding(Gleam::InputBindingDescriptor& binding, uint32_t index);

	void DrawControl(const char* label, Gleam::InputControl& control, uint32_t binding, uint32_t part);

	void DrawProcessors(Gleam::TArray<Gleam::InputProcessor>& processors);

	void DrawInteractions(Gleam::TArray<Gleam::InputInteraction>& interactions);

	bool DrawListHeader(const char* label);

	void UpdateListening();

	void StartListening(int32_t binding, uint32_t part);

	void StopListening();

	Gleam::InputActionMapDescriptor* GetSelectedMap();

	Gleam::InputActionDescriptor* GetSelectedAction();

	void SelectMap(int32_t index);

	void SelectAction(int32_t index);

	EAssetManager* mAssetManager;

	EditorInputSystem* mEditorInput = nullptr;

	const EditorFonts* mFonts = nullptr;

	AssetItem mAsset;

	Gleam::Path mFile;

	Gleam::TString mName;

	char mNameBuffer[128] = "";

	bool mEditingName = false;

	Gleam::InputActionAssetDescriptor mDescriptor;

	int32_t mSelectedMap = -1;

	int32_t mSelectedAction = -1;

	ListenTarget mListening;

	Gleam::TArray<Gleam::InputUser*> mMutedUsers;

	bool mIsOpen = false;

	bool mDirty = false;

};

} // namespace GEditor
