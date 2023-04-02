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

class SceneView : public View
{
public:
    
    SceneView();
    
    ~SceneView();
    
    virtual void Render() override;
    
private:
    
    Gleam::Entity mPlayCameraEntity;
    
    Gleam::Entity mEditorCameraEntity;
    
    Gleam::RefCounted<Gleam::World> mEditWorld;
    
    Gleam::RefCounted<Gleam::World> mPlayWorld;
    
};

} // namespace GEditor
