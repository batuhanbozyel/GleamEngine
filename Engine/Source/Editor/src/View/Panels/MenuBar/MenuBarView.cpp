//
//  MenuBarView.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 19.05.2023.
//

#include "MenuBarView.h"
#include "Gleam.h"

#include <imgui.h>

using namespace GEditor;

MenuBarView::MenuBarView()
{

}

void MenuBarView::Render()
{
    if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
				GameInstance->Quit();

			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}