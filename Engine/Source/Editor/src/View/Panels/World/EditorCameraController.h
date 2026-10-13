//
//  EditorCameraController.h
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#pragma once
#include "World/Entity.h"
#include "Math/BoundingBox.h"
#include "Math/Vector2.h"

namespace Gleam {
class EntityManager;
} // namespace Gleam

namespace GEditor {

enum class CameraNavigation
{
	None,
	Fly,
	Orbit,
	Pan,
	Dolly
};

struct EditorCameraInput
{
	Gleam::Float2 mouseDelta = Gleam::Float2::zero;
	// x: camera right, y: world up, z: camera forward
	Gleam::Float3 move = Gleam::Float3::zero;
	float scroll = 0.0f;
	bool boost = false;
};

class EditorCameraController
{
public:

	EditorCameraController(Gleam::EntityHandle cameraEntity);

	void BeginNavigation(CameraNavigation navigation);

	void EndNavigation();

	void Update(Gleam::EntityManager& entityManager, const EditorCameraInput& input, float viewportHeight);

	void Frame(const Gleam::EntityManager& entityManager, const Gleam::BoundingBox& bounds);

	CameraNavigation GetNavigation() const
	{
		return mNavigation;
	}

private:

	CameraNavigation mNavigation = CameraNavigation::None;

	bool mFraming = false;

	Gleam::Float3 mFramePivot = Gleam::Float3::zero;

	float mFrameDistance = 0.0f;

	Gleam::EntityHandle mCameraEntity;

};

} // namespace GEditor
