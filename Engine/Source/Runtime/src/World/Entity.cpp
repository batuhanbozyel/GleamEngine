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

void Entity::SetParent(const EntityHandle parent)
{
	// Remove entity from old parent's children
	if (HasParent())
	{
		auto& parentEntity = GetParentEntity();
		auto it = std::remove(parentEntity.mChildren.begin(), parentEntity.mChildren.end(), mHandle);
		parentEntity.mChildren.erase(it);
	}

	mParent = parent;

	// Add entity to the parent's children
	if (parent != InvalidEntity)
	{
		auto& parentEntity = GetParentEntity();
		parentEntity.mChildren.push_back(mHandle);

		mGlobalTransform = parentEntity.GetWorldTransform() * mLocalTransform;
	}
	else
	{
		mGlobalTransform = mLocalTransform;
	}

	for (auto child : mChildren)
	{
		auto& childEntity = GetChildEntity(child);
		childEntity.mGlobalTransform = mGlobalTransform * childEntity.mLocalTransform;
	}
}

void Entity::Translate(const Float3& translation)
{
	mLocalTransform.position += translation;
	mGlobalTransform.position += translation;

	for (auto child : mChildren)
	{
		auto& childEntity = GetChildEntity(child);
		childEntity.mGlobalTransform.position += translation;
	}
}

void Entity::Rotate(const Quaternion& rotation)
{
	mLocalTransform.rotation *= rotation;
	mGlobalTransform.rotation *= rotation;

	for (auto child : mChildren)
	{
		auto& childEntity = GetChildEntity(child);
		childEntity.mGlobalTransform.rotation *= rotation;
	}
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
	mGlobalTransform.scale *= scale;

	for (auto child : mChildren)
	{
		auto& childEntity = GetChildEntity(child);
		childEntity.mGlobalTransform.scale *= scale;
	}
}

void Entity::SetTranslation(const Float3& translation)
{
	mGlobalTransform.position = mGlobalTransform.position - mLocalTransform.position + translation;
	mLocalTransform.position = translation;

	for (auto child : mChildren)
	{
		auto& childEntity = GetChildEntity(child);
		childEntity.mGlobalTransform.position = childEntity.mLocalTransform.position + mGlobalTransform.position;
	}
}

void Entity::SetRotation(const Quaternion& rotation)
{
	mLocalTransform.rotation = rotation;

	if (HasParent())
	{
		auto& parent = GetParentEntity();
		mGlobalTransform.rotation = parent.GetWorldRotation() * mLocalTransform.rotation;
	}
	else
	{
		mGlobalTransform.rotation = mLocalTransform.rotation;
	}

	for (auto child : mChildren)
	{
		auto& childEntity = GetChildEntity(child);
		childEntity.mGlobalTransform.rotation = GetWorldRotation() * childEntity.mLocalTransform.rotation;
	}
}

void Entity::SetScale(float scale)
{
	mLocalTransform.scale = scale;

	if (HasParent())
	{
		auto& parent = GetParentEntity();
		mGlobalTransform.scale = parent.GetWorldScale() * mLocalTransform.scale;
	}
	else
	{
		mGlobalTransform.scale = mLocalTransform.scale;
	}

	for (auto child : mChildren)
	{
		auto& childEntity = GetChildEntity(child);
		childEntity.mGlobalTransform.scale = GetWorldScale() * childEntity.mLocalTransform.scale;
	}
}

void Entity::SetLocalTransform(const Transform& transform)
{
	mLocalTransform = transform;
	if (HasParent())
	{
		auto& parent = GetParentEntity();
		mGlobalTransform = parent.GetWorldTransform() * mLocalTransform;
	}
	else
	{
		mGlobalTransform = mLocalTransform;
	}

	for (auto child : mChildren)
	{
		auto& childEntity = GetChildEntity(child);
		childEntity.mGlobalTransform = mGlobalTransform * childEntity.mLocalTransform;
	}
}