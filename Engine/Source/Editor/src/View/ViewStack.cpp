//
//  ViewStack.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 27.03.2023.
//

#include "ViewStack.h"
#include "ImGui/ImGuiRenderer.h"

using namespace GEditor;

void ViewStack::OnCreate()
{
	GameInstance->GetRenderPipeline()->AddRenderer<ImGuiRenderer>();
}

void ViewStack::OnDestroy()
{
	GameInstance->GetRenderPipeline()->RemoveRenderer<ImGuiRenderer>();
	mViews.clear();
}
