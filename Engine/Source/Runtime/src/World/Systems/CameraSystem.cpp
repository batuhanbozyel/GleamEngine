#include "gpch.h"
#include "CameraSystem.h"

#include "World/EntityManager.h"
#include "World/Components/Camera.h"

using namespace Gleam;

EntityHandle CameraSystem::GetMainCamera(const EntityManager& entityManager) const
{
	EntityHandle mainCamera = InvalidEntity;
	int32_t mainPriority = std::numeric_limits<int32_t>::min();
	entityManager.ForEach<Entity, Camera>([&](const Entity& entity, const Camera& camera)
	{
		if (entity.IsActive() and camera.targetTexture.guid == Guid::InvalidGuid() and camera.priority >= mainPriority)
		{
			mainCamera = entity;
			mainPriority = camera.priority;
		}
	});
	return mainCamera;
}
