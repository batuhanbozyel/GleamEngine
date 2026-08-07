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
#include "World/Entity.h"

namespace Gleam {
class EntityManager;
struct ImGuiPassData;
} // namespace Gleam

namespace GEditor {

class InfiniteGridRenderer;
class EditorCameraController;
class ViewModeRenderer;
class SelectionSystem;
class SelectionOutlineRenderer;

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

	void DrawTransformGizmo(const Gleam::Float2& imageMin, const Gleam::Float2& imageSize);

	void UpdateSelectionMarquee(const Gleam::Float2& imageMin, const Gleam::Float2& imageSize, const Gleam::Size& targetSize, bool viewportHovered);

	Gleam::TArray<Gleam::EntityHandle> GatherGizmoTargets(const Gleam::EntityManager& entityManager) const;

	void Resize(Gleam::EntityManager& entityManager, const Gleam::Size& size);

    bool mCursorVisible = true;

	bool mViewportSizeChanged = false;

	bool mMarqueeDragging = false;

	bool mMarqueeMoved = false;

	Gleam::Float2 mMarqueeStart = Gleam::Float2::zero;
    
	InfiniteGridRenderer* mGridRenderer = nullptr;

	SelectionOutlineRenderer* mSelectionOutlineRenderer = nullptr;

    EditorCameraController* mCameraController = nullptr;

	Gleam::EntityHandle mCamera = Gleam::InvalidEntity;

	SelectionSystem* mSelectionSystem = nullptr;

	TransformGizmo mTransformGizmo;
    
    Gleam::Size mViewportSize = Gleam::Size::zero;
    
    Gleam::World* mEditWorld = nullptr;
    
};

} // namespace GEditor
