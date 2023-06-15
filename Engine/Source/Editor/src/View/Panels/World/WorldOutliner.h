//
//  WorldOutliner.h
//  Editor
//
//  Created by Batuhan Bozyel on 25.05.2023.
//

#pragma once
#include "View/View.h"
#include "Gleam.h"

namespace GEditor {

class WorldOutlineController;

class WorldOutliner final : public View
{
public:
    
    WorldOutliner();
    
    virtual void Render() override;
    
private:
    
    Gleam::Entity mSelectedEntity = {};
    
    WorldOutlineController* mController;
    
    Gleam::RefCounted<Gleam::World> mEditWorld;
    
};

} // namespace GEditor
