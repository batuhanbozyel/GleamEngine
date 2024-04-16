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

void MenuBar::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this]()
	{
		if (!ImGui::BeginMenuBar()) { return; }
		
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				Gleam::EventDispatcher<Gleam::AppCloseEvent>::Publish(Gleam::AppCloseEvent());
			}

			ImGui::EndMenu();
		}
		
		ImGui::EndMenuBar();
	});
}
