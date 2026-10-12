#pragma once
#include "View/View.h"

namespace GEditor {

class ViewStack;

class Preferences final : public View
{
public:

	virtual void OnCreate(Gleam::Application* app) override;

    virtual void Render(Gleam::ImGuiRenderer* imgui) override;

    void Open() { mIsOpen = true; }

private:

	ViewStack* mViewStack = nullptr;

    bool mIsOpen = false;

};

} // namespace GEditor
