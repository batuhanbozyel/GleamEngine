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
    mEditWorld = Gleam::World::active;
	mEditWorld->AddSystem<SceneViewController>();
    mEditWorld->GetRenderPipeline().AddRenderer<Gleam::WorldRenderer>();
    mEditWorld->GetRenderPipeline().AddRenderer<Gleam::DebugRenderer>();
    mEditWorld->GetRenderPipeline().AddRenderer<Gleam::CompositeRenderer>();

    mPlayWorld = Gleam::World::Create();
}

SceneView::~SceneView()
{
    mEditWorld->GetRenderPipeline().RemoveRenderer<Gleam::CompositeRenderer>();
    mEditWorld->GetRenderPipeline().RemoveRenderer<Gleam::DebugRenderer>();
    mEditWorld->GetRenderPipeline().RemoveRenderer<Gleam::WorldRenderer>();
}

void SceneView::Render()
{
    
}
