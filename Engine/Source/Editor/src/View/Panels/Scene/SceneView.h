//
//  SceneView.h
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#pragma once
#include "View.h"
#include "Gleam.h"

namespace GEditor {

class SceneView final : public View
{
public:
    
    SceneView();
    
    virtual void Render() override;
    
private:
    
    Gleam::RefCounted<Gleam::World> mEditWorld;
    
};

} // namespace GEditor
