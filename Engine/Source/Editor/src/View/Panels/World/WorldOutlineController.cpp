//
//  WorldOutlineController.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 25.05.2023.
//

#include "WorldOutlineController.h"

#include "WorldViewport.h"

using namespace GEditor;

void WorldOutlineController::OnCreate(Gleam::EntityManager& entityManager)
{
    
}

void WorldOutlineController::OnUpdate(Gleam::EntityManager& entityManager)
{
    for (const auto& model : mModelImportQueue)
    {
        auto entity = entityManager.CreateEntity();
        entityManager.AddComponent<Gleam::MeshRenderer>(entity, model);
        entityManager.AddComponent<Gleam::TString>(entity, "GameObject");
    }
    mModelImportQueue.clear();
}

void WorldOutlineController::ImportAsset(Gleam::FileType type)
{
    const auto& selectedFiles = Gleam::IOUtils::OpenFileDialog(Gleam::FileType::Model);
    for (const auto& file : selectedFiles)
    {
        mModelImportQueue.emplace_back(Gleam::AssetLibrary<Gleam::Model>::Import(file));
    }
}
