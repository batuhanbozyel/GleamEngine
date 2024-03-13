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
    
    virtual void OnCreate(Gleam::EntityManager& entityManager) override;
    
    virtual void OnUpdate(Gleam::EntityManager& entityManager) override;
    
private:
    
};

} // namespace GEditor
