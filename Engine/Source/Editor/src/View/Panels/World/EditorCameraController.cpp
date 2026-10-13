//
//  EditorCameraController.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#include "EditorCameraController.h"
#include "EditorCamera.h"

#include <Editor.Reflection.generated.h>

#include "Core/Timestep.h"

#include "World/EntityManager.h"
#include "World/Components/Camera.h"

using namespace GEditor;

static constexpr float kLookSensitivity = 0.15f;
static constexpr float kMaxPitch = 89.0f;
static constexpr float kSpeedStep = 1.2f;
static constexpr float kMinSpeed = 0.1f;
static constexpr float kMaxSpeed = 500.0f;
static constexpr float kBoostMultiplier = 4.0f;
static constexpr float kDollyFactor = 0.85f;
static constexpr float kDollyDragScale = 0.02f;
static constexpr float kMinDollyStep = 0.05f;
static constexpr float kMinDistance = 0.05f;
static constexpr float kMinFrameRadius = 0.5f;
static constexpr float kFrameSharpness = 18.0f;

static Gleam::Quaternion GetRotation(const Gleam::EditorCamera& editorCamera)
{
	return Gleam::Quaternion(Gleam::Math::Deg2Rad(Gleam::Float3{ editorCamera.pitch, editorCamera.yaw, 0.0f }));
}

static void Rotate(Gleam::EditorCamera& editorCamera, const Gleam::Float2& mouseDelta)
{
	editorCamera.yaw += mouseDelta.x * kLookSensitivity;
	editorCamera.pitch = Gleam::Math::Clamp(editorCamera.pitch + mouseDelta.y * kLookSensitivity, -kMaxPitch, kMaxPitch);
}

// Steps proportionally to the pivot distance; the floor keeps zooming in moving past the pivot instead of stalling at it
static float Dolly(Gleam::EditorCamera& editorCamera, float amount)
{
	float step = editorCamera.distance * (1.0f - Gleam::Math::Pow(kDollyFactor, amount));
	if (amount > 0.0f)
	{
		step = Gleam::Math::Max(step, kMinDollyStep * amount);
	}
	editorCamera.distance = Gleam::Math::Max(editorCamera.distance - step, kMinDistance);
	return step;
}

EditorCameraController::EditorCameraController(Gleam::EntityHandle cameraEntity)
	: mCameraEntity(cameraEntity)
{

}

void EditorCameraController::BeginNavigation(CameraNavigation navigation)
{
	mNavigation = navigation;
	mFraming = false;
}

void EditorCameraController::EndNavigation()
{
	mNavigation = CameraNavigation::None;
}

void EditorCameraController::Update(Gleam::EntityManager& entityManager, const EditorCameraInput& input, float viewportHeight)
{
	if (mNavigation == CameraNavigation::None and input.scroll == 0.0f and mFraming == false)
	{
		return;
	}

	auto& entity = entityManager.GetComponent<Gleam::Entity>(mCameraEntity);
	auto& editorCamera = entityManager.GetComponent<Gleam::EditorCamera>(mCameraEntity);
	const auto& camera = entityManager.GetComponent<Gleam::Camera>(mCameraEntity);
	const float deltaTime = static_cast<float>(Gleam::Timestep::deltaTime);

	auto rotation = GetRotation(editorCamera);
	auto position = entity.GetLocalPosition();
	const auto pivot = position + rotation * Gleam::Float3::forward * editorCamera.distance;

	switch (mNavigation)
	{
		case CameraNavigation::None:
		{
			mFraming = false;
			position += rotation * Gleam::Float3::forward * Dolly(editorCamera, input.scroll);
			break;
		}
		case CameraNavigation::Fly:
		{
			Rotate(editorCamera, input.mouseDelta);
			rotation = GetRotation(editorCamera);
			editorCamera.speed = Gleam::Math::Clamp(editorCamera.speed * Gleam::Math::Pow(kSpeedStep, input.scroll), kMinSpeed, kMaxSpeed);

			const float speed = editorCamera.speed * (input.boost ? kBoostMultiplier : 1.0f);
			const auto direction = rotation * Gleam::Float3{ input.move.x, 0.0f, input.move.z } + Gleam::Float3::up * input.move.y;
			position += direction * speed * deltaTime;
			break;
		}
		case CameraNavigation::Orbit:
		{
			Rotate(editorCamera, input.mouseDelta);
			rotation = GetRotation(editorCamera);
			position = pivot - rotation * Gleam::Float3::forward * editorCamera.distance;
			break;
		}
		case CameraNavigation::Pan:
		{
			// Scaled so geometry at the pivot depth stays under the cursor
			const float viewHeight = camera.projectionType == Gleam::ProjectionType::Perspective
				? 2.0f * editorCamera.distance * Gleam::Math::Tan(Gleam::Math::Deg2Rad(camera.fov) * 0.5f)
				: 2.0f * camera.orthographicSize;
			const float scale = viewHeight / viewportHeight;
			position += rotation * Gleam::Float3{ -input.mouseDelta.x * scale, input.mouseDelta.y * scale, 0.0f };
			break;
		}
		case CameraNavigation::Dolly:
		{
			const float amount = (input.mouseDelta.x - input.mouseDelta.y) * kDollyDragScale;
			position += rotation * Gleam::Float3::forward * Dolly(editorCamera, amount);
			break;
		}
	}

	if (mFraming)
	{
		const float t = 1.0f - Gleam::Math::Exp(-kFrameSharpness * deltaTime);
		const auto framePivot = pivot + (mFramePivot - pivot) * t;
		editorCamera.distance += (mFrameDistance - editorCamera.distance) * t;
		position = framePivot - rotation * Gleam::Float3::forward * editorCamera.distance;
		mFraming = Gleam::Math::LengthSquared(mFramePivot - framePivot) > 1e-6f or Gleam::Math::Abs(mFrameDistance - editorCamera.distance) > 1e-3f;
	}

	entity.SetTranslation(position);
	entity.SetRotation(rotation);
}

void EditorCameraController::Frame(const Gleam::EntityManager& entityManager, const Gleam::BoundingBox& bounds)
{
	const auto& camera = entityManager.GetComponent<Gleam::Camera>(mCameraEntity);
	const float radius = Gleam::Math::Max(Gleam::Math::Length(bounds.Extent()) * 0.5f, kMinFrameRadius);
	mFramePivot = (bounds.min + bounds.max) * 0.5f;
	mFrameDistance = radius / Gleam::Math::Sin(Gleam::Math::Deg2Rad(camera.fov) * 0.5f);
	mFraming = true;
}
