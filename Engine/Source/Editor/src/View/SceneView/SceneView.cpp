//
//  SceneView.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#include "SceneView.h"
#include "SceneViewController.h"

using namespace GEditor;

SceneView::SceneView()
{
    Gleam::RenderPipeline::Get()->AddRenderer<Gleam::DebugRenderer>();
    
    mEditWorld = Gleam::World::active;
	mEditWorld->AddSystem<EditorSceneViewController>();

    mPlayWorld = Gleam::World::Create();
}

SceneView::~SceneView()
{
    Gleam::RenderPipeline::Get()->RemoveRenderer<Gleam::DebugRenderer>();
}

void SceneView::Render()
{
    
}
