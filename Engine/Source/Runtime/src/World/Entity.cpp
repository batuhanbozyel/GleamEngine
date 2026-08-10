#include "gpch.h"
#include "Entity.h"

using namespace Gleam;

Entity& Entity::GetParentEntity() const
{
	GLEAM_ASSERT(IsValid(), "Entity is invalid!");
	GLEAM_ASSERT(HasComponent<Entity>(), "Entity does not have the component!");
	if constexpr (Reflection::Traits::IsReflected<Entity>::value)
	{
		const auto& classDesc = Reflection::GetClass<Entity>();
		return mRegistry->storage<Entity>(classDesc.TypeHash()).get(mParent);
	}
	else
	{
		return mRegistry->get<Entity>(mParent);
	}
}

Entity& Entity::GetChildEntity(EntityHandle child) const
{
	GLEAM_ASSERT(IsValid(), "Entity is invalid!");
	GLEAM_ASSERT(HasComponent<Entity>(), "Entity does not have the component!");
	if constexpr (Reflection::Traits::IsReflected<Entity>::value)
	{
		const auto& classDesc = Reflection::GetClass<Entity>();
		return mRegistry->storage<Entity>(classDesc.TypeHash()).get(child);
	}
	else
	{
		return mRegistry->get<Entity>(child);
	}
}

void Entity::UpdateTransformHierarchy()
{
	if (HasParent())
	{
		mGlobalTransform = GetParentEntity().GetWorldTransform() * mLocalTransform;
	}
	else
	{
		mGlobalTransform = mLocalTransform;
	}

	UpdateChildTransforms();
}

void Entity::UpdateChildTransforms()
{
	for (auto child : mChildren)
	{
		auto& childEntity = GetChildEntity(child);
		childEntity.mGlobalTransform = mGlobalTransform * childEntity.mLocalTransform;
		childEntity.UpdateChildTransforms();
	}
}

void Entity::SetParent(const EntityHandle parent)
{
	// Remove entity from old parent's children
	if (HasParent())
	{
		auto& parentEntity = GetParentEntity();
		auto it = eastl::remove(parentEntity.mChildren.begin(), parentEntity.mChildren.end(), mHandle);
		parentEntity.mChildren.erase(it);
	}

	mParent = parent;

	// Add entity to the parent's children
	if (parent != InvalidEntity)
	{
		GetParentEntity().mChildren.push_back(mHandle);
	}

	UpdateTransformHierarchy();
}

void Entity::Translate(const Float3& translation)
{
	mLocalTransform.position += translation;
	UpdateTransformHierarchy();
}

void Entity::Rotate(const Quaternion& rotation)
{
	mLocalTransform.rotation *= rotation;
	UpdateTransformHierarchy();
}

void Entity::Rotate(const Float3& eulers)
{
	Rotate(Quaternion(eulers));
}

void Entity::Rotate(float xAngle, float yAngle, float zAngle)
{
	Rotate(Float3{ xAngle, yAngle, zAngle });
}

void Entity::Scale(float scale)
{
	mLocalTransform.scale *= scale;
	UpdateTransformHierarchy();
}

void Entity::SetTranslation(const Float3& translation)
{
	mLocalTransform.position = translation;
	UpdateTransformHierarchy();
}

void Entity::SetRotation(const Quaternion& rotation)
{
	mLocalTransform.rotation = rotation;
	UpdateTransformHierarchy();
}

void Entity::SetScale(float scale)
{
	mLocalTransform.scale = scale;
	UpdateTransformHierarchy();
}

void Entity::SetLocalTransform(const Transform& transform)
{
	mLocalTransform = transform;
	UpdateTransformHierarchy();
}
