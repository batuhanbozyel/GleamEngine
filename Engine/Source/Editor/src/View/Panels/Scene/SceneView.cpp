//
//  SceneView.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#include "SceneView.h"
#include "SceneViewController.h"

#include <imgui.h>

using namespace GEditor;

SceneView::SceneView()
{
    GameInstance->GetRenderPipeline()->AddRenderer<Gleam::DebugRenderer>();
    
    mEditWorld = Gleam::World::active;
	mEditWorld->AddSystem<SceneViewController>();
}

void SceneView::Render()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
    ImGui::Begin("Scene View");
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    
//    ImGui::Image(reinterpret_cast<void*>((uint64_t)s_Data.LastFrame), viewportSize, ImVec2(0, 1), ImVec2(1, 0));
//    Tunti::Renderer::OnWindowResize(viewportSize.x, viewportSize.y);
//    s_Data.SceneEditorCamera.SetViewportSize(viewportSize.x, viewportSize.y);
//
//    s_Data.IsSceneHovered = ImGui::IsItemHovered();
    
    ImGui::End();
    ImGui::PopStyleVar();
}
