//
//  WorldViewport.h
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#pragma once
#include "View/View.h"
#include "View/Widgets/TransformGizmo.h"
#include "Math/Size.h"

namespace Gleam {
class EntityManager;
struct ImGuiPassData;
} // namespace Gleam

namespace GEditor {

class InfiniteGridRenderer;
class EditorCameraController;
class ViewModeRenderer;

class WorldViewport final : public View
{
public:
    
	virtual void OnCreate(Gleam::World* world) override;

	virtual void OnDestroy(Gleam::World* world) override;
    
    virtual void Update() override;
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) override;
    
    const Gleam::Size& GetViewportSize() const
    {
        return mViewportSize;
    }
    
private:

	void DrawToolbar();

	void DrawViewport(Gleam::ImGuiRenderer* imgui, const Gleam::ImGuiPassData& passData);

	void DrawGizmo(const Gleam::Float2& imageMin, const Gleam::Float2& imageSize);

	void Resize(Gleam::EntityManager& entityManager, const Gleam::Size& size);

    bool mCursorVisible = true;

	bool mViewportSizeChanged = false;
    
	InfiniteGridRenderer* mGridRenderer = nullptr;

    EditorCameraController* mCameraController = nullptr;

	Gleam::EntityHandle mCamera = Gleam::InvalidEntity;

	Gleam::EntityHandle mSelectedEntity = Gleam::InvalidEntity;

	TransformGizmo mGizmo;
    
    Gleam::Size mViewportSize = Gleam::Size::zero;
    
    Gleam::World* mEditWorld = nullptr;
    
};

} // namespace GEditor
