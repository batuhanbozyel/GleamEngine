//
//  WorldViewport.h
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#pragma once
#include "View/View.h"
#include "Math/Size.h"

namespace Gleam {
class EntityManager;
} // namespace Gleam

namespace GEditor {

class EditorCameraController;

class WorldViewport final : public View
{
public:
    
	virtual void Init(Gleam::World* world) override;
    
    virtual void Update() override;
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) override;
    
    const Gleam::Size& GetViewportSize() const
    {
        return mViewportSize;
    }
    
private:

	void Resize(Gleam::EntityManager& entityManager, const Gleam::Size& size);
    
    bool mCursorVisible = true;

	bool mViewportSizeChanged = false;
    
    EditorCameraController* mCameraController = nullptr;

	Gleam::EntityHandle mCamera;
    
    Gleam::Size mViewportSize;
    
    Gleam::World* mEditWorld;
    
};

} // namespace GEditor
