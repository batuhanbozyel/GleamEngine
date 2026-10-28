//
//  LaunchFailureDialog.h
//  Editor
//

#pragma once
#include "View/View.h"
#include "Container/String.h"

namespace GEditor {

struct EditorFonts;

struct LaunchFailure
{
	Gleam::TString title;
	Gleam::TString log;
};

class LaunchFailureDialog final : public View
{
public:

	explicit LaunchFailureDialog(const LaunchFailure& failure);

	virtual void OnCreate(Gleam::Application* app) override;

	virtual void Render(Gleam::ImGuiRenderer* imgui) override;

private:

	LaunchFailure mFailure;

	const EditorFonts* mFonts = nullptr;

	bool mOpenPopup = true;

	bool mScrollToBottom = true;
};

} // namespace GEditor
