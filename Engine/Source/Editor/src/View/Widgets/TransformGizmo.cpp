//
//  TransformGizmo.cpp
//  Editor
//

#include "TransformGizmo.h"

#include <imgui.h>
#include <imgui_internal.h>

using namespace GEditor;

namespace {

// Handle thicknesses below are authored against this radius and scale with the size actually used
constexpr float kGizmoReferencePixels  = 100.0f;
constexpr float kGizmoViewportFraction = 0.15f;
constexpr float kGizmoMinPixels        = 36.0f;
constexpr float kGizmoMaxPixels        = 96.0f;
constexpr float kPickThickness       = 8.0f;
constexpr float kAxisThickness       = 3.0f;
constexpr float kRingThickness       = 3.0f;
constexpr float kArrowHeadStart      = 0.82f;
constexpr float kArrowHeadWidth      = 9.0f;
constexpr float kPlaneHandleStart    = 0.28f;
constexpr float kPlaneHandleEnd      = 0.52f;
constexpr float kCenterHandlePixels  = 8.0f;
constexpr float kScaleHandlePixels   = 6.0f;
constexpr float kParallelAxisCutoff  = 0.985f;
constexpr float kRingGrazingCutoff   = 0.08f;
constexpr float kRingFadeStart       = 0.30f;
constexpr float kRingBandThickness   = 9.0f;
constexpr float kArcSpan             = Gleam::Math::PI * 0.5f;
constexpr int   kArcSegments         = 32;
constexpr int   kPlaneGridLines      = 7;

constexpr float kTranslateSnap = 0.25f;
constexpr float kRotateSnap    = Gleam::Math::Deg2Rad(15.0f);
constexpr float kScaleSnap     = 0.1f;

constexpr ImU32 kAxisColor[3] =
{
	IM_COL32(214,  69,  69, 255),
	IM_COL32(112, 191,  62, 255),
	IM_COL32( 62, 124, 214, 255)
};

constexpr ImU32 kActiveColor    = IM_COL32(255, 196,  40, 255);
constexpr ImU32 kScreenColor    = IM_COL32(222, 222, 222, 255);
constexpr ImU32 kLabelColor     = IM_COL32(255, 255, 255, 255);
constexpr float kBandEdgeShade  = 0.55f;
constexpr ImU32 kPlaneShadeColor = IM_COL32( 12,  12,  12,  95);

constexpr uint32_t kPlaneFillAlpha       = 20;
constexpr uint32_t kPlaneGridAlpha       = 70;
constexpr uint32_t kPlaneGridActiveAlpha = 175;

ImVec2 ToImVec2(const Gleam::Float2& value)
{
	return ImVec2(value.x, value.y);
}

ImU32 WithAlpha(ImU32 color, uint32_t alpha)
{
	return (color & ~IM_COL32_A_MASK) | (alpha << IM_COL32_A_SHIFT);
}

ImU32 ScaleAlpha(ImU32 color, float scale)
{
	return WithAlpha(color, uint32_t(float((color >> IM_COL32_A_SHIFT) & 0xFFu) * scale));
}

ImU32 ShadeColor(ImU32 color, float scale)
{
	const uint32_t red = uint32_t(float((color >> IM_COL32_R_SHIFT) & 0xFFu) * scale);
	const uint32_t green = uint32_t(float((color >> IM_COL32_G_SHIFT) & 0xFFu) * scale);
	const uint32_t blue = uint32_t(float((color >> IM_COL32_B_SHIFT) & 0xFFu) * scale);
	return IM_COL32(red, green, blue, (color >> IM_COL32_A_SHIFT) & 0xFFu);
}

bool WorldToScreen(const GizmoViewport& viewport, const Gleam::Float3& world, Gleam::Float2& screen, float& clipW)
{
	const Gleam::Float4 clip = viewport.viewProjection * Gleam::Float4(world, 1.0f);
	if (clip.w <= Gleam::Math::Epsilon)
	{
		return false;
	}

	const float invW = 1.0f / clip.w;
	screen.x = viewport.rectMin.x + (clip.x * invW * 0.5f + 0.5f) * viewport.rectSize.x;
	screen.y = viewport.rectMin.y + (0.5f - clip.y * invW * 0.5f) * viewport.rectSize.y;
	clipW = clip.w;
	return true;
}

bool WorldToScreen(const GizmoViewport& viewport, const Gleam::Float3& world, Gleam::Float2& screen)
{
	float clipW = 0.0f;
	return WorldToScreen(viewport, world, screen, clipW);
}

// Sized off the viewport so the gizmo does not swallow a small editor window on a laptop display
float GizmoPixelSize(const GizmoViewport& viewport)
{
	const float shortEdge = Gleam::Math::Min(viewport.rectSize.x, viewport.rectSize.y);
	return Gleam::Math::Clamp(shortEdge * kGizmoViewportFraction, kGizmoMinPixels, kGizmoMaxPixels);
}

// World units covering the given amount of pixels at the depth the gizmo sits at.
// Ortho projections leave clipW at 1, which the same expression handles.
float PixelsToWorld(const GizmoViewport& viewport, float pixels, float clipW)
{
	return 2.0f * pixels / viewport.rectSize.y * clipW / viewport.projectionScaleY;
}

void ScreenToRay(const GizmoViewport& viewport, const Gleam::Float2& screen, Gleam::Float3& origin, Gleam::Float3& direction)
{
	const float ndcX = (screen.x - viewport.rectMin.x) / viewport.rectSize.x * 2.0f - 1.0f;
	const float ndcY = 1.0f - (screen.y - viewport.rectMin.y) / viewport.rectSize.y * 2.0f;

	const Gleam::Float4 nearClip = viewport.invViewProjection * Gleam::Float4(Gleam::Float3{ ndcX, ndcY, 0.0f }, 1.0f);
	const Gleam::Float4 farClip  = viewport.invViewProjection * Gleam::Float4(Gleam::Float3{ ndcX, ndcY, 1.0f }, 1.0f);

	const Gleam::Float3 nearPoint{ nearClip.x / nearClip.w, nearClip.y / nearClip.w, nearClip.z / nearClip.w };
	const Gleam::Float3 farPoint{ farClip.x / farClip.w, farClip.y / farClip.w, farClip.z / farClip.w };

	origin = nearPoint;
	direction = Gleam::Math::Normalize(farPoint - nearPoint);
}

bool RayPlaneIntersection(const Gleam::Float3& rayOrigin, const Gleam::Float3& rayDirection,
                          const Gleam::Float3& planePoint, const Gleam::Float3& planeNormal, Gleam::Float3& hit)
{
	const float denominator = Gleam::Math::Dot(rayDirection, planeNormal);
	if (Gleam::Math::Abs(denominator) < 1.0e-5f)
	{
		return false;
	}

	const float t = Gleam::Math::Dot(planePoint - rayOrigin, planeNormal) / denominator;
	if (t < 0.0f)
	{
		return false;
	}

	hit = rayOrigin + rayDirection * t;
	return true;
}

// Plane containing the axis whose normal points as much towards the camera as possible,
// which keeps axis dragging stable no matter where the camera sits.
Gleam::Float3 AxisDragPlaneNormal(const Gleam::Float3& axis, const Gleam::Float3& origin, const Gleam::Float3& cameraPosition)
{
	const Gleam::Float3 toCamera = cameraPosition - origin;
	const Gleam::Float3 perpendicular = toCamera - axis * Gleam::Math::Dot(toCamera, axis);
	if (Gleam::Math::LengthSquared(perpendicular) < 1.0e-8f)
	{
		return axis;
	}
	return Gleam::Math::Normalize(perpendicular);
}

// Unreal keeps each arc in a fixed quadrant of its ring and switches between the eight octant views
// as the camera crosses an axis plane, rather than sliding the arc around to track the viewer.
float RingArcStartAngle(const GizmoViewport& viewport, const Gleam::Float3& origin,
                        const Gleam::Float3& tangentU, const Gleam::Float3& tangentV)
{
	const Gleam::Float3 toCamera = viewport.cameraPosition - origin;
	const float projectedU = Gleam::Math::Dot(toCamera, tangentU);
	const float projectedV = Gleam::Math::Dot(toCamera, tangentV);

	if (projectedU >= 0.0f)
	{
		return projectedV >= 0.0f ? 0.0f : kArcSpan * 3.0f;
	}
	return projectedV >= 0.0f ? kArcSpan : kArcSpan * 2.0f;
}

float DistanceToSegment(const Gleam::Float2& point, const Gleam::Float2& start, const Gleam::Float2& end)
{
	const Gleam::Float2 segment = end - start;
	const float lengthSquared = Gleam::Math::Dot(segment, segment);
	if (lengthSquared < 1.0e-6f)
	{
		return Gleam::Math::Length(point - start);
	}

	const float t = Gleam::Math::Clamp(Gleam::Math::Dot(point - start, segment) / lengthSquared, 0.0f, 1.0f);
	return Gleam::Math::Length(point - (start + segment * t));
}

bool PointInQuad(const Gleam::Float2& point, const Gleam::Float2 corners[4])
{
	bool positive = false;
	bool negative = false;
	for (uint32_t i = 0; i < 4; ++i)
	{
		const Gleam::Float2 edge = corners[(i + 1) % 4] - corners[i];
		const Gleam::Float2 toPoint = point - corners[i];
		const float cross = edge.x * toPoint.y - edge.y * toPoint.x;
		positive |= cross > 0.0f;
		negative |= cross < 0.0f;
	}
	return not (positive && negative);
}

float SignedAngle(const Gleam::Float3& from, const Gleam::Float3& to, const Gleam::Float3& axis)
{
	const float sine = Gleam::Math::Dot(Gleam::Math::Cross(from, to), axis);
	const float cosine = Gleam::Math::Dot(from, to);
	return Gleam::Math::Atan2(sine, cosine);
}

Gleam::Quaternion AxisAngle(const Gleam::Float3& axis, float radians)
{
	const float halfAngle = radians * 0.5f;
	const float sine = Gleam::Math::Sin(halfAngle);
	return Gleam::Quaternion(Gleam::Math::Cos(halfAngle), axis.x * sine, axis.y * sine, axis.z * sine);
}

float SnapTo(float value, float step)
{
	return Gleam::Math::Round(value / step) * step;
}

} // namespace

bool TransformGizmo::Manipulate(const GizmoViewport& viewport, bool inputEnabled, Gleam::Transform& transform)
{
	if (viewport.rectSize.x < 1.0f || viewport.rectSize.y < 1.0f)
	{
		return false;
	}

	Frame frame;
	frame.origin = transform.position;

	float originClipW = 0.0f;
	if (not WorldToScreen(viewport, frame.origin, frame.originScreen, originClipW))
	{
		mHoveredHandle = Handle::None;
		mActiveHandle = Handle::None;
		return false;
	}

	if (mSpace == GizmoSpace::Local && mOperation != GizmoOperation::Scale)
	{
		frame.axis[0] = transform.rotation * Gleam::Float3::right;
		frame.axis[1] = transform.rotation * Gleam::Float3::up;
		frame.axis[2] = transform.rotation * Gleam::Float3::forward;
	}

	frame.pixelSize = GizmoPixelSize(viewport);
	frame.detailScale = frame.pixelSize / kGizmoReferencePixels;

	// Frozen while dragging so the handles stay under the cursor as the object moves in depth
	if (not IsDragging())
	{
		mGizmoSize = PixelsToWorld(viewport, frame.pixelSize, originClipW);
	}
	frame.size = mGizmoSize;

	const auto& io = ImGui::GetIO();
	const Gleam::Float2 mouse{ io.MousePos.x, io.MousePos.y };

	bool modified = false;
	if (IsDragging())
	{
		if (io.MouseDown[ImGuiMouseButton_Left])
		{
			modified = UpdateDrag(viewport, frame, mouse, io.KeyCtrl, transform);
			frame.origin = transform.position;
			WorldToScreen(viewport, frame.origin, frame.originScreen);
		}
		else
		{
			mActiveHandle = Handle::None;
		}
	}
	else
	{
		mHoveredHandle = inputEnabled ? HitTest(viewport, frame, mouse) : Handle::None;
		if (mHoveredHandle != Handle::None && io.MouseClicked[ImGuiMouseButton_Left])
		{
			BeginDrag(viewport, frame, mouse, transform);
		}
	}

	auto drawList = ImGui::GetWindowDrawList();
	drawList->PushClipRect(ToImVec2(viewport.rectMin), ToImVec2(viewport.rectMin + viewport.rectSize), true);
	switch (mOperation)
	{
		case GizmoOperation::Translate: DrawTranslate(viewport, frame); break;
		case GizmoOperation::Rotate:    DrawRotate(viewport, frame); break;
		case GizmoOperation::Scale:     DrawScale(viewport, frame); break;
	}

	if (IsDragging())
	{
		DrawDragLabel(frame);
	}
	drawList->PopClipRect();

	return modified;
}

TransformGizmo::Handle TransformGizmo::HitTest(const GizmoViewport& viewport, const Frame& frame, const Gleam::Float2& mouse) const
{
	if (mOperation == GizmoOperation::Scale)
	{
		if (Gleam::Math::Length(mouse - frame.originScreen) < kCenterHandlePixels * frame.detailScale + kPickThickness)
		{
			return Handle::Scale;
		}

		const float ringDistance = Gleam::Math::Abs(Gleam::Math::Length(mouse - frame.originScreen) - frame.pixelSize);
		return ringDistance < kPickThickness ? Handle::Scale : Handle::None;
	}

	if (mOperation == GizmoOperation::Rotate)
	{
		const Gleam::Float3 viewDirection = Gleam::Math::Normalize(frame.origin - viewport.cameraPosition);
		const float pickRadius = kRingBandThickness * frame.detailScale * 0.5f + kPickThickness * 0.5f;

		Handle picked = Handle::None;
		float pickedDistance = Gleam::Math::Infinity;
		for (uint32_t i = 0; i < 3; ++i)
		{
			// A ring this close to edge on has no usable drag plane, grabbing it would cancel immediately
			if (Gleam::Math::Abs(Gleam::Math::Dot(frame.axis[i], viewDirection)) < kRingGrazingCutoff)
			{
				continue;
			}

			Gleam::Float2 arc[kArcSegments + 1];
			const uint32_t count = BuildRingArc(viewport, frame, i, arc);

			// The arcs share corners, so the nearest one wins instead of the first one tested
			for (uint32_t point = 1; point < count; ++point)
			{
				const float distance = DistanceToSegment(mouse, arc[point - 1], arc[point]);
				if (distance < pickRadius && distance < pickedDistance)
				{
					pickedDistance = distance;
					picked = static_cast<Handle>(static_cast<uint32_t>(Handle::RotateX) + i);
				}
			}
		}

		return picked;
	}

	if (Gleam::Math::Length(mouse - frame.originScreen) < kCenterHandlePixels * frame.detailScale)
	{
		return Handle::ScreenTranslate;
	}

	for (uint32_t i = 0; i < 3; ++i)
	{
		const Gleam::Float3 tangentU = frame.axis[(i + 1) % 3];
		const Gleam::Float3 tangentV = frame.axis[(i + 2) % 3];

		Gleam::Float2 corners[4];
		bool valid = WorldToScreen(viewport, frame.origin + (tangentU + tangentV) * (frame.size * kPlaneHandleStart), corners[0])
			&& WorldToScreen(viewport, frame.origin + (tangentU * kPlaneHandleEnd + tangentV * kPlaneHandleStart) * frame.size, corners[1])
			&& WorldToScreen(viewport, frame.origin + (tangentU + tangentV) * (frame.size * kPlaneHandleEnd), corners[2])
			&& WorldToScreen(viewport, frame.origin + (tangentU * kPlaneHandleStart + tangentV * kPlaneHandleEnd) * frame.size, corners[3]);

		if (valid && PointInQuad(mouse, corners))
		{
			return static_cast<Handle>(static_cast<uint32_t>(Handle::PlaneYZ) + i);
		}
	}

	for (uint32_t i = 0; i < 3; ++i)
	{
		const Gleam::Float3 viewDirection = Gleam::Math::Normalize(frame.origin - viewport.cameraPosition);
		if (Gleam::Math::Abs(Gleam::Math::Dot(frame.axis[i], viewDirection)) > kParallelAxisCutoff)
		{
			continue;
		}

		Gleam::Float2 tip;
		if (WorldToScreen(viewport, frame.origin + frame.axis[i] * frame.size, tip)
			&& DistanceToSegment(mouse, frame.originScreen, tip) < kPickThickness)
		{
			return static_cast<Handle>(static_cast<uint32_t>(Handle::AxisX) + i);
		}
	}

	return Handle::None;
}

void TransformGizmo::BeginDrag(const GizmoViewport& viewport, const Frame& frame, const Gleam::Float2& mouse, const Gleam::Transform& transform)
{
	mActiveHandle = mHoveredHandle;
	mStartTransform = transform;
	mRotationDelta = 0.0f;
	mScaleFactor = 1.0f;

	Gleam::Float3 rayOrigin;
	Gleam::Float3 rayDirection;
	ScreenToRay(viewport, mouse, rayOrigin, rayDirection);

	const Gleam::Float3 viewAxis = Gleam::Math::Normalize(frame.origin - viewport.cameraPosition);
	const uint32_t axisIndex = static_cast<uint32_t>(mActiveHandle) - static_cast<uint32_t>(Handle::AxisX);
	const uint32_t planeIndex = static_cast<uint32_t>(mActiveHandle) - static_cast<uint32_t>(Handle::PlaneYZ);
	const uint32_t ringIndex = static_cast<uint32_t>(mActiveHandle) - static_cast<uint32_t>(Handle::RotateX);

	switch (mActiveHandle)
	{
		case Handle::AxisX:
		case Handle::AxisY:
		case Handle::AxisZ:
		{
			mDragAxis = frame.axis[axisIndex];
			mDragPlaneNormal = AxisDragPlaneNormal(mDragAxis, frame.origin, viewport.cameraPosition);
			break;
		}
		case Handle::PlaneYZ:
		case Handle::PlaneZX:
		case Handle::PlaneXY:
		{
			mDragAxis = Gleam::Float3::zero;
			mDragPlaneNormal = frame.axis[planeIndex];
			break;
		}
		case Handle::ScreenTranslate:
		{
			mDragAxis = Gleam::Float3::zero;
			mDragPlaneNormal = viewAxis;
			break;
		}
		case Handle::RotateX:
		case Handle::RotateY:
		case Handle::RotateZ:
		{
			mDragAxis = frame.axis[ringIndex];
			mDragPlaneNormal = mDragAxis;
			break;
		}
		case Handle::Scale:
		{
			mDragStartDistance = Gleam::Math::Max(Gleam::Math::Length(mouse - frame.originScreen), 1.0f);
			return;
		}
		default: return;
	}

	if (RayPlaneIntersection(rayOrigin, rayDirection, frame.origin, mDragPlaneNormal, mDragStartPoint))
	{
		mDragOffset = mStartTransform.position - mDragStartPoint;
		mDragPrevDirection = Gleam::Math::Normalize(mDragStartPoint - frame.origin);
	}
	else
	{
		mActiveHandle = Handle::None;
	}
}

bool TransformGizmo::UpdateDrag(const GizmoViewport& viewport, const Frame& frame, const Gleam::Float2& mouse, bool snap, Gleam::Transform& transform)
{
	Gleam::Float3 rayOrigin;
	Gleam::Float3 rayDirection;
	ScreenToRay(viewport, mouse, rayOrigin, rayDirection);

	if (mActiveHandle == Handle::Scale)
	{
		const float distance = Gleam::Math::Max(Gleam::Math::Length(mouse - frame.originScreen), 1.0f);
		mScaleFactor = distance / mDragStartDistance;
		if (snap)
		{
			mScaleFactor = SnapTo(mScaleFactor, kScaleSnap);
		}

		const float scale = Gleam::Math::Max(mStartTransform.scale * mScaleFactor, 1.0e-4f);
		if (scale == transform.scale)
		{
			return false;
		}

		transform.scale = scale;
		return true;
	}

	Gleam::Float3 hit;
	if (not RayPlaneIntersection(rayOrigin, rayDirection, mStartTransform.position, mDragPlaneNormal, hit))
	{
		return false;
	}

	if (mOperation == GizmoOperation::Rotate)
	{
		// Dragging across the centre leaves no direction to measure the angle from
		const Gleam::Float3 radial = hit - mStartTransform.position;
		if (Gleam::Math::LengthSquared(radial) < Gleam::Math::Epsilon)
		{
			return false;
		}

		const Gleam::Float3 direction = Gleam::Math::Normalize(radial);
		mRotationDelta += SignedAngle(mDragPrevDirection, direction, mDragAxis);
		mDragPrevDirection = direction;

		const float angle = snap ? SnapTo(mRotationDelta, kRotateSnap) : mRotationDelta;
		const Gleam::Quaternion rotation = AxisAngle(mDragAxis, angle) * mStartTransform.rotation;
		if (rotation == transform.rotation)
		{
			return false;
		}

		transform.rotation = rotation;
		return true;
	}

	Gleam::Float3 position = hit + mDragOffset;
	if (mDragAxis != Gleam::Float3::zero)
	{
		const float projected = Gleam::Math::Dot(position - mStartTransform.position, mDragAxis);
		position = mStartTransform.position + mDragAxis * projected;
	}

	if (snap)
	{
		const Gleam::Float3 delta = position - mStartTransform.position;
		position = mStartTransform.position + Gleam::Float3
		{
			SnapTo(delta.x, kTranslateSnap),
			SnapTo(delta.y, kTranslateSnap),
			SnapTo(delta.z, kTranslateSnap)
		};
	}

	if (position == transform.position)
	{
		return false;
	}

	transform.position = position;
	return true;
}

void TransformGizmo::DrawTranslate(const GizmoViewport& viewport, const Frame& frame) const
{
	auto drawList = ImGui::GetWindowDrawList();
	const Gleam::Float3 viewDirection = Gleam::Math::Normalize(frame.origin - viewport.cameraPosition);

	for (uint32_t i = 0; i < 3; ++i)
	{
		const Handle planeHandle = static_cast<Handle>(static_cast<uint32_t>(Handle::PlaneYZ) + i);
		const bool highlighted = mHoveredHandle == planeHandle || mActiveHandle == planeHandle;
		const Gleam::Float3 tangentU = frame.axis[(i + 1) % 3];
		const Gleam::Float3 tangentV = frame.axis[(i + 2) % 3];

		Gleam::Float2 corners[4];
		const bool valid = WorldToScreen(viewport, frame.origin + (tangentU + tangentV) * (frame.size * kPlaneHandleStart), corners[0])
			&& WorldToScreen(viewport, frame.origin + (tangentU * kPlaneHandleEnd + tangentV * kPlaneHandleStart) * frame.size, corners[1])
			&& WorldToScreen(viewport, frame.origin + (tangentU + tangentV) * (frame.size * kPlaneHandleEnd), corners[2])
			&& WorldToScreen(viewport, frame.origin + (tangentU * kPlaneHandleStart + tangentV * kPlaneHandleEnd) * frame.size, corners[3]);

		if (valid == false)
		{
			continue;
		}

		const ImU32 color = highlighted ? kActiveColor : kAxisColor[i];
		const ImVec2 points[4] = { ToImVec2(corners[0]), ToImVec2(corners[1]), ToImVec2(corners[2]), ToImVec2(corners[3]) };
		drawList->AddConvexPolyFilled(points, 4, (color & 0x00FFFFFF) | (highlighted ? 0x99000000 : 0x55000000));
		drawList->AddPolyline(points, 4, color, ImDrawFlags_Closed, 1.5f);
	}

	for (uint32_t i = 0; i < 3; ++i)
	{
		const Handle axisHandle = static_cast<Handle>(static_cast<uint32_t>(Handle::AxisX) + i);
		const bool highlighted = mHoveredHandle == axisHandle || mActiveHandle == axisHandle;
		if (Gleam::Math::Abs(Gleam::Math::Dot(frame.axis[i], viewDirection)) > kParallelAxisCutoff)
		{
			continue;
		}

		Gleam::Float2 headBase;
		Gleam::Float2 tip;
		if (WorldToScreen(viewport, frame.origin + frame.axis[i] * (frame.size * kArrowHeadStart), headBase) == false
			|| WorldToScreen(viewport, frame.origin + frame.axis[i] * frame.size, tip) == false)
		{
			continue;
		}

		const ImU32 color = highlighted ? kActiveColor : kAxisColor[i];
		drawList->AddLine(ToImVec2(frame.originScreen), ToImVec2(headBase), color, Gleam::Math::Max(kAxisThickness * frame.detailScale, 2.0f));

		const Gleam::Float2 forward = tip - headBase;
		const float length = Gleam::Math::Length(forward);
		if (length < 1.0e-3f)
		{
			continue;
		}

		const Gleam::Float2 direction = forward / length;
		const Gleam::Float2 side{ -direction.y, direction.x };
		const ImVec2 head[3] =
		{
			ToImVec2(tip),
			ToImVec2(headBase + side * (kArrowHeadWidth * frame.detailScale * 0.5f)),
			ToImVec2(headBase - side * (kArrowHeadWidth * frame.detailScale * 0.5f))
		};
		drawList->AddConvexPolyFilled(head, 3, color);
	}

	const bool centerHighlighted = mHoveredHandle == Handle::ScreenTranslate || mActiveHandle == Handle::ScreenTranslate;
	drawList->AddCircleFilled(ToImVec2(frame.originScreen), kCenterHandlePixels * frame.detailScale * 0.6f, centerHighlighted ? kActiveColor : kScreenColor);
}

void TransformGizmo::DrawRotate(const GizmoViewport& viewport, const Frame& frame) const
{
	auto drawList = ImGui::GetWindowDrawList();
	const Gleam::Float3 viewDirection = Gleam::Math::Normalize(frame.origin - viewport.cameraPosition);

	// A ring approaching edge on projects to a stub that reads as noise rather than an arc, so it
	// fades out over the range where it also stops being pickable
	float opacity[3];
	for (uint32_t i = 0; i < 3; ++i)
	{
		const float facing = Gleam::Math::Abs(Gleam::Math::Dot(frame.axis[i], viewDirection));
		opacity[i] = Gleam::Math::Clamp((facing - kRingGrazingCutoff) / (kRingFadeStart - kRingGrazingCutoff), 0.0f, 1.0f);
	}

	// All three patches stay visible, the hovered one just reads stronger. Drawn before the arcs so
	// none of them end up underneath a patch
	for (uint32_t i = 0; i < 3; ++i)
	{
		const Handle ringHandle = static_cast<Handle>(static_cast<uint32_t>(Handle::RotateX) + i);
		const bool highlighted = mHoveredHandle == ringHandle || mActiveHandle == ringHandle;
		DrawRotationPlane(viewport, frame, i, highlighted, opacity[i]);
	}

	for (uint32_t i = 0; i < 3; ++i)
	{
		const Handle ringHandle = static_cast<Handle>(static_cast<uint32_t>(Handle::RotateX) + i);
		const bool highlighted = mHoveredHandle == ringHandle || mActiveHandle == ringHandle;
		if (opacity[i] <= 0.0f)
		{
			continue;
		}

		Gleam::Float2 arc[kArcSegments + 1];
		const uint32_t count = BuildRingArc(viewport, frame, i, arc);
		if (count == 0)
		{
			continue;
		}

		ImVec2 points[kArcSegments + 1];
		for (uint32_t point = 0; point < count; ++point)
		{
			points[point] = ToImVec2(arc[point]);
		}

		// Banded arc over a darker shade of its own colour, so overlapping arcs stay separable
		const ImU32 color = WithAlpha(highlighted ? kActiveColor : kAxisColor[i], uint32_t(255.0f * opacity[i]));
		const ImU32 edgeColor = ShadeColor(color, kBandEdgeShade);
		const float thickness = kRingBandThickness * frame.detailScale;
		const float edgeThickness = thickness + 2.0f * frame.detailScale;

		drawList->AddPolyline(points, count, edgeColor, ImDrawFlags_None, edgeThickness);
		drawList->AddPolyline(points, count, color, ImDrawFlags_None, thickness);

		// Polylines are flat capped, so the corners the arcs share would notch without rounding the ends
		drawList->AddCircleFilled(points[0], thickness * 0.5f, color);
		drawList->AddCircleFilled(points[count - 1], thickness * 0.5f, color);
	}

	if (IsDragging() && mOperation == GizmoOperation::Rotate)
	{
		Gleam::Float2 start;
		if (WorldToScreen(viewport, mStartTransform.position + mDragPrevDirection * frame.size, start))
		{
			drawList->AddLine(ToImVec2(frame.originScreen), ToImVec2(start), kActiveColor, 1.5f);
		}
	}
}

uint32_t TransformGizmo::BuildRingArc(const GizmoViewport& viewport, const Frame& frame, uint32_t axisIndex, Gleam::Float2* screenPoints) const
{
	const Gleam::Float3 tangentU = frame.axis[(axisIndex + 1) % 3];
	const Gleam::Float3 tangentV = frame.axis[(axisIndex + 2) % 3];
	const float startAngle = RingArcStartAngle(viewport, frame.origin, tangentU, tangentV);

	uint32_t count = 0;
	for (int32_t segment = 0; segment <= kArcSegments; ++segment)
	{
		const float angle = startAngle + kArcSpan * float(segment) / float(kArcSegments);
		const Gleam::Float3 point = frame.origin + (tangentU * Gleam::Math::Cos(angle) + tangentV * Gleam::Math::Sin(angle)) * frame.size;
		if (WorldToScreen(viewport, point, screenPoints[count]) == false)
		{
			return 0;
		}
		++count;
	}
	return count;
}

void TransformGizmo::DrawRotationPlane(const GizmoViewport& viewport, const Frame& frame, uint32_t axisIndex, bool highlighted, float opacity) const
{
	if (opacity <= 0.0f)
	{
		return;
	}

	// Built from the arc itself so the patch fills exactly what the arc encloses
	Gleam::Float2 arc[kArcSegments + 1];
	const uint32_t arcCount = BuildRingArc(viewport, frame, axisIndex, arc);
	if (arcCount == 0)
	{
		return;
	}

	auto drawList = ImGui::GetWindowDrawList();

	ImVec2 sector[kArcSegments + 2];
	sector[0] = ToImVec2(frame.originScreen);
	for (uint32_t point = 0; point < arcCount; ++point)
	{
		sector[point + 1] = ToImVec2(arc[point]);
	}
	// Hovered patch drops to a dark backdrop so its grid reads against whatever is behind the gizmo
	const ImU32 axisColor = kAxisColor[axisIndex];
	const ImU32 fillColor = highlighted ? kPlaneShadeColor : WithAlpha(axisColor, kPlaneFillAlpha);
	drawList->AddConvexPolyFilled(sector, arcCount + 1, ScaleAlpha(fillColor, opacity));

	const Gleam::Float3 tangentU = frame.axis[(axisIndex + 1) % 3];
	const Gleam::Float3 tangentV = frame.axis[(axisIndex + 2) % 3];
	const float startAngle = RingArcStartAngle(viewport, frame.origin, tangentU, tangentV);
	const Gleam::Float3 dirA = tangentU * Gleam::Math::Cos(startAngle) + tangentV * Gleam::Math::Sin(startAngle);
	const Gleam::Float3 dirB = tangentU * Gleam::Math::Cos(startAngle + kArcSpan) + tangentV * Gleam::Math::Sin(startAngle + kArcSpan);
	const ImU32 gridColor = ScaleAlpha(WithAlpha(axisColor, highlighted ? kPlaneGridActiveAlpha : kPlaneGridAlpha), opacity);

	for (int32_t line = 0; line < kPlaneGridLines; ++line)
	{
		// Each line runs to the arc, so the grid ends on the curve instead of squaring off the patch
		const float offset = frame.size * float(line) / float(kPlaneGridLines);
		const float extent = Gleam::Math::Sqrt(Gleam::Math::Max(frame.size * frame.size - offset * offset, 0.0f));

		Gleam::Float2 start;
		Gleam::Float2 end;
		if (WorldToScreen(viewport, frame.origin + dirA * offset, start)
			&& WorldToScreen(viewport, frame.origin + dirA * offset + dirB * extent, end))
		{
			drawList->AddLine(ToImVec2(start), ToImVec2(end), gridColor, 1.0f);
		}

		if (WorldToScreen(viewport, frame.origin + dirB * offset, start)
			&& WorldToScreen(viewport, frame.origin + dirB * offset + dirA * extent, end))
		{
			drawList->AddLine(ToImVec2(start), ToImVec2(end), gridColor, 1.0f);
		}
	}
}

void TransformGizmo::DrawScale(const GizmoViewport& viewport, const Frame& frame) const
{
	auto drawList = ImGui::GetWindowDrawList();
	const bool highlighted = mHoveredHandle == Handle::Scale || mActiveHandle == Handle::Scale;
	const ImU32 color = highlighted ? kActiveColor : kScreenColor;

	// Uniform scaling only, so the handle is a ring that is grabbed and pulled instead of per axis boxes
	drawList->AddCircle(ToImVec2(frame.originScreen), frame.pixelSize, color, 0, kRingThickness * frame.detailScale);

	for (uint32_t i = 0; i < 4; ++i)
	{
		const float angle = Gleam::Math::PI * 0.25f + Gleam::Math::PI * 0.5f * float(i);
		const Gleam::Float2 handle = frame.originScreen + Gleam::Float2{ Gleam::Math::Cos(angle), Gleam::Math::Sin(angle) } * frame.pixelSize;
		const float handleExtent = kScaleHandlePixels * frame.detailScale * 0.5f;
		drawList->AddRectFilled(ToImVec2(handle - Gleam::Float2{ handleExtent, handleExtent }),
			ToImVec2(handle + Gleam::Float2{ handleExtent, handleExtent }), color);
	}

	if (IsDragging())
	{
		drawList->AddCircle(ToImVec2(frame.originScreen), frame.pixelSize * mScaleFactor, kActiveColor, 0, 1.5f);
	}

	drawList->AddCircleFilled(ToImVec2(frame.originScreen), kCenterHandlePixels * frame.detailScale * 0.6f, color);
}

void TransformGizmo::DrawDragLabel(const Frame& frame) const
{
	char label[64] = {};
	switch (mOperation)
	{
		case GizmoOperation::Translate:
		{
			const Gleam::Float3 delta = frame.origin - mStartTransform.position;
			ImFormatString(label, IM_ARRAYSIZE(label), "%.2f, %.2f, %.2f", delta.x, delta.y, delta.z);
			break;
		}
		case GizmoOperation::Rotate:
		{
			ImFormatString(label, IM_ARRAYSIZE(label), "%.1f deg", Gleam::Math::Rad2Deg(mRotationDelta));
			break;
		}
		case GizmoOperation::Scale:
		{
			ImFormatString(label, IM_ARRAYSIZE(label), "x%.3f", mScaleFactor);
			break;
		}
	}

	auto drawList = ImGui::GetWindowDrawList();
	const ImVec2 textSize = ImGui::CalcTextSize(label);
	const ImVec2 position(frame.originScreen.x + kCenterHandlePixels * frame.detailScale + 8.0f, frame.originScreen.y - textSize.y - 8.0f);
	drawList->AddRectFilled(ImVec2(position.x - 4.0f, position.y - 2.0f),
		ImVec2(position.x + textSize.x + 4.0f, position.y + textSize.y + 2.0f), IM_COL32(20, 20, 20, 200), 3.0f);
	drawList->AddText(position, kLabelColor, label);
}
