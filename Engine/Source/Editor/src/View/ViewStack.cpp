//
//  ViewStack.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 27.03.2023.
//

#include "ViewStack.h"
#include "ImGui/ImGuiRenderer.h"

using namespace GEditor;

void ViewStack::OnCreate(Gleam::EntityManager& entityManager)
{
	GameInstance->GetSubsystem<Gleam::RenderSystem>()->AddRenderer<ImGuiRenderer>();
}

void ViewStack::OnDestroy(Gleam::EntityManager& entityManager)
{
	GameInstance->GetSubsystem<Gleam::RenderSystem>()->RemoveRenderer<ImGuiRenderer>();
	mViews.clear();
}

void ViewStack::OnUpdate(Gleam::EntityManager& entityManager)
{
    for (auto view : mViews)
    {
        view->Update();
    }
}
