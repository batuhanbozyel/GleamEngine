//
//  ProjectSettings.h
//  Editor
//

#pragma once
#include "View/View.h"

#include <cstdint>

namespace GEditor {

class ProjectSettings final : public View
{
public:

    virtual void Render(Gleam::ImGuiRenderer* imgui) override;

    void Open() { mIsOpen = true; }

private:

    void DrawCategoryList();

    void DrawSettingsContent();

    uint32_t mSelectedConfig = 0;

    bool mIsOpen = false;

};

} // namespace GEditor
