//
//  TransformGizmo.h
//  Editor
//

#pragma once
#include "Math/Float4x4.h"
#include "World/Components/Transform.h"

namespace GEditor {

enum class GizmoOperation
{
	Translate,
	Rotate,
	Scale
};

enum class GizmoSpace
{
	World,
	Local
};

// Camera and viewport state required to project the gizmo onto the viewport image.
// Rect coordinates are ImGui screen space, matching ImGui::GetItemRectMin of the image.
struct GizmoViewport
{
	Gleam::Float4x4 viewProjection;
	Gleam::Float4x4 invViewProjection;
	Gleam::Float3   cameraPosition = Gleam::Float3::zero;

	// Projection matrix (1,1) element, keeps the gizmo the same pixel size at any distance
	float projectionScaleY = 1.0f;

	Gleam::Float2 rectMin = Gleam::Float2::zero;
	Gleam::Float2 rectSize = Gleam::Float2::zero;
};

class TransformGizmo
{
public:

	bool Manipulate(const GizmoViewport& viewport, bool inputEnabled, Gleam::Transform& transform);
	
	bool IsHovered() const
	{
		return mHoveredHandle != Handle::None;
	}

	bool IsDragging() const
	{
		return mActiveHandle != Handle::None;
	}

	GizmoOperation GetOperation() const
	{
		return mOperation;
	}

	void SetOperation(GizmoOperation operation)
	{
		mOperation = operation;
	}

	GizmoSpace GetSpace() const
	{
		return mSpace;
	}

	void SetSpace(GizmoSpace space)
	{
		mSpace = space;
	}

private:

	enum class Handle
	{
		None,
		AxisX,
		AxisY,
		AxisZ,
		PlaneYZ,
		PlaneZX,
		PlaneXY,
		ScreenTranslate,
		RotateX,
		RotateY,
		RotateZ,
		Scale
	};

	struct Frame
	{
		Gleam::Float3 origin = Gleam::Float3::zero;
		Gleam::Float3 axis[3] = { Gleam::Float3::right, Gleam::Float3::up, Gleam::Float3::forward };
		Gleam::Float2 originScreen = Gleam::Float2::zero;

		// World radius of the handles and the screen radius it was derived from
		float size = 1.0f;
		float pixelSize = 0.0f;

		// Screen radius relative to the one the handle thicknesses were authored against
		float detailScale = 1.0f;
	};

	Handle HitTest(const GizmoViewport& viewport, const Frame& frame, const Gleam::Float2& mouse) const;

	void BeginDrag(const GizmoViewport& viewport, const Frame& frame, const Gleam::Float2& mouse, const Gleam::Transform& transform);

	bool UpdateDrag(const GizmoViewport& viewport, const Frame& frame, const Gleam::Float2& mouse, bool snap, Gleam::Transform& transform);

	void DrawTranslate(const GizmoViewport& viewport, const Frame& frame) const;

	void DrawRotate(const GizmoViewport& viewport, const Frame& frame) const;

	uint32_t BuildRingArc(const GizmoViewport& viewport, const Frame& frame, uint32_t axisIndex, Gleam::Float2* screenPoints) const;

	void DrawRotationPlane(const GizmoViewport& viewport, const Frame& frame, uint32_t axisIndex, bool highlighted, float opacity) const;

	void DrawScale(const GizmoViewport& viewport, const Frame& frame) const;

	void DrawDragLabel(const Frame& frame) const;

	GizmoOperation mOperation = GizmoOperation::Translate;

	GizmoSpace mSpace = GizmoSpace::World;

	Handle mHoveredHandle = Handle::None;

	Handle mActiveHandle = Handle::None;

	Gleam::Transform mStartTransform;

	Gleam::Float3 mDragAxis = Gleam::Float3::zero;

	Gleam::Float3 mDragPlaneNormal = Gleam::Float3::zero;

	Gleam::Float3 mDragStartPoint = Gleam::Float3::zero;

	Gleam::Float3 mDragOffset = Gleam::Float3::zero;

	Gleam::Float3 mDragPrevDirection = Gleam::Float3::zero;

	float mDragStartDistance = 0.0f;

	float mRotationDelta = 0.0f;

	float mScaleFactor = 1.0f;

	float mGizmoSize = 1.0f;

};

} // namespace GEditor
