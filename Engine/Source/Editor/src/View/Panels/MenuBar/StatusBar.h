#pragma once
#include "View/View.h"

namespace GEditor {

struct EditorFonts;

class StatusBar final : public View
{
public:

	virtual void OnCreate(Gleam::Application* app) override;

    virtual void Render(Gleam::ImGuiRenderer* imgui) override;

private:

	const EditorFonts* mFonts = nullptr;

};

} // namespace GEditor
