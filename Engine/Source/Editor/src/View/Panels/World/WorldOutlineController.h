//
//  WorldOutlineController.h
//  Editor
//
//  Created by Batuhan Bozyel on 25.05.2023.
//

#pragma once
#include "Gleam.h"

namespace GEditor {

class WorldOutlineController : public Gleam::ComponentSystem
{
public:
    
    void ImportAsset(Gleam::FileType type);
    
    virtual void OnCreate(Gleam::EntityManager& entityManager) override;
    
    virtual void OnUpdate(Gleam::EntityManager& entityManager) override;
    
private:
    
    Gleam::TArray<Gleam::Model> mModelImportQueue;
    
};

} // namespace GEditor
