#include "gpch.h"
#include "TransformSystem.h"
#include "World/Components/Transform.h"

using namespace Gleam;

void TransformSystem::OnUpdate(EntityManager& entityManager)
{
	entityManager.ForEach<Entity, Transform>([&](const Entity& entity, Transform& transform)
	{
		if (entity.IsActive())
		{
			UpdateTransform(entityManager, transform);
		}
	});

	entityManager.ForEach<Entity, Camera>([&](const Entity& entity, Camera& camera)
	{
		if (entity.IsActive())
		{
			UpdateTransform(entityManager, camera);

			if (camera.mProjectionMatrixDirty)
			{
				camera.RecalculateProjectionMatrix();
			}
			if (camera.mViewMatrixDirty)
			{
				camera.RecalculateViewMatrix();
			}
		}
	});
}

void TransformSystem::UpdateTransform(EntityManager& entityManager, Transform& transform) const
{
	if (RequiresUpdate(entityManager, transform))
	{
		const auto& translation = transform.GetLocalPosition();
		const auto& rotation = transform.GetLocalRotation();
		const auto& scale = transform.GetLocalScale();

		auto localTransform = Float4x4::TRS(translation, rotation, scale);
		auto globalTransform = localTransform;
		if (transform.HasParent())
		{
			auto parent = entityManager[transform.GetParent()];
			auto& parentTransform = entityManager.GetComponent<Transform>(parent);
			UpdateTransform(entityManager, parentTransform);
			globalTransform = parentTransform.GetWorldTransform() * globalTransform;
		}
		transform.UpdateTransform(localTransform, globalTransform);
	}
}

bool TransformSystem::RequiresUpdate(EntityManager& entityManager, const Transform& transform) const
{
	if (transform.mIsTransformDirty)
	{
		return true;
	}

	if (transform.HasParent())
	{
		auto parent = entityManager[transform.GetParent()];
		const auto& parentTransform = entityManager.GetComponent<Transform>(parent);
		return RequiresUpdate(entityManager, parentTransform);
	}
	return false;
}