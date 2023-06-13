//
//  MenuBar.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 19.05.2023.
//

#include "MenuBar.h"
#include "Gleam.h"

#include <imgui.h>

using namespace GEditor;

MenuBar::MenuBar()
{

}

void MenuBar::Render()
{
    if (!ImGui::BeginMenuBar()) { return; }
    
    
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Exit"))
            GameInstance->Quit();

        ImGui::EndMenu();
    }
    
    ImGui::EndMenuBar();
}
