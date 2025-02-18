#include "gpch.h"
#include "Entity.h"

using namespace Gleam;

void Entity::SetParent(const EntityHandle parent)
{
	// Remove entity from old parent's children
	if (HasParent())
	{
		auto& parentEntity = mRegistry->get<Entity>(mParent);
		auto it = std::remove(parentEntity.mChildren.begin(), parentEntity.mChildren.end(), mHandle);
		parentEntity.mChildren.erase(it);
	}

	mParent = parent;

	// Add entity to the parent's children
	if (parent != InvalidEntity)
	{
		auto& parentEntity = mRegistry->get<Entity>(mParent);
		parentEntity.mChildren.push_back(mHandle);
	}
	UpdateTransform();
}

void Entity::UpdateTransform() const
{
	mIsTransformDirty = false;
	mLocalTransform.matrix = Float4x4::TRS(mLocalTransform.position, mLocalTransform.rotation, mLocalTransform.scale);
	if (HasParent())
	{
		auto& parent = mRegistry->get<Entity>(mParent);
		mGlobalTransform.matrix = parent.GetWorldTransform().matrix * mLocalTransform.matrix;
		Math::Decompose(mGlobalTransform.matrix, mGlobalTransform.position, mGlobalTransform.rotation, mGlobalTransform.scale);
	}
	else
	{
		mGlobalTransform = mLocalTransform;
	}
}

bool Entity::RequiresTransformUpdate() const
{
	if (HasParent())
	{
		auto& parent = mRegistry->get<Entity>(mParent);
		return mIsTransformDirty || parent.RequiresTransformUpdate();
	}
	return mIsTransformDirty;
}

void Entity::Translate(const Float3& translation)
{
	mLocalTransform.position += translation;
	mGlobalTransform.position += translation;

	mLocalTransform.matrix.m[12] += translation.x;
	mLocalTransform.matrix.m[13] += translation.y;
	mLocalTransform.matrix.m[14] += translation.z;

	mGlobalTransform.matrix.m[12] += translation.x;
	mGlobalTransform.matrix.m[13] += translation.y;
	mGlobalTransform.matrix.m[14] += translation.z;

	for (auto child : mChildren)
	{
		auto& childEntity = mRegistry->get<Entity>(child);
		childEntity.mGlobalTransform.position += translation;
		childEntity.SetDirty();
	}
}

void Entity::Rotate(const Quaternion& rotation)
{
	mLocalTransform.rotation *= rotation;
	mGlobalTransform.rotation *= rotation;

	for (auto child : mChildren)
	{
		auto& childEntity = mRegistry->get<Entity>(child);
		childEntity.mGlobalTransform.rotation *= rotation;
	}
	SetDirty();
}

void Entity::Rotate(const Float3& eulers)
{
	Rotate(Quaternion(eulers));
}

void Entity::Rotate(float xAngle, float yAngle, float zAngle)
{
	Rotate(Float3{ xAngle, yAngle, zAngle });
}

void Entity::Scale(const Float3& scale)
{
	mLocalTransform.scale *= scale;
	mGlobalTransform.scale *= scale;

	for (auto child : mChildren)
	{
		auto& childEntity = mRegistry->get<Entity>(child);
		childEntity.mGlobalTransform.scale *= scale;
	}
	SetDirty();
}

void Entity::Scale(float scale)
{
	Scale(Float3(scale));
}

void Entity::SetTranslation(const Float3& translation)
{
	mGlobalTransform.position = mGlobalTransform.position - mLocalTransform.position + translation;
	mLocalTransform.position = translation;

	mLocalTransform.matrix.m[12] = mLocalTransform.position.x;
	mLocalTransform.matrix.m[13] = mLocalTransform.position.y;
	mLocalTransform.matrix.m[14] = mLocalTransform.position.z;

	mGlobalTransform.matrix.m[12] = mGlobalTransform.position.x;
	mGlobalTransform.matrix.m[13] = mGlobalTransform.position.y;
	mGlobalTransform.matrix.m[14] = mGlobalTransform.position.z;

	for (auto child : mChildren)
	{
		auto& childEntity = mRegistry->get<Entity>(child);
		childEntity.mGlobalTransform.position = childEntity.mLocalTransform.position + mGlobalTransform.position;
		childEntity.SetDirty();
	}
}

void Entity::SetRotation(const Quaternion& rotation)
{
	mLocalTransform.rotation = rotation;

	if (HasParent())
	{
		auto& parent = mRegistry->get<Entity>(mParent);
		mGlobalTransform.rotation = parent.GetWorldRotation() * mLocalTransform.rotation;
	}
	else
	{
		mGlobalTransform.rotation = mLocalTransform.rotation;
	}

	for (auto child : mChildren)
	{
		auto& childEntity = mRegistry->get<Entity>(child);
		childEntity.mGlobalTransform.rotation = GetWorldRotation() * childEntity.mLocalTransform.rotation;
	}
	SetDirty();
}

void Entity::SetScale(const Float3& scale)
{
	mLocalTransform.scale = scale;

	if (HasParent())
	{
		auto& parent = mRegistry->get<Entity>(mParent);
		mGlobalTransform.scale = parent.GetWorldScale() * mLocalTransform.scale;
	}
	else
	{
		mGlobalTransform.scale = mLocalTransform.scale;
	}

	for (auto child : mChildren)
	{
		auto& childEntity = mRegistry->get<Entity>(child);
		childEntity.mGlobalTransform.scale = GetWorldScale() * childEntity.mLocalTransform.scale;
	}
	SetDirty();
}

void Entity::SetLocalTransform(const Float4x4& transform)
{
	mLocalTransform.matrix = transform;
	Math::Decompose(transform, mLocalTransform.position, mLocalTransform.rotation, mLocalTransform.scale);

	if (HasParent())
	{
		auto& parent = mRegistry->get<Entity>(mParent);
		mGlobalTransform.matrix = parent.GetWorldTransform().matrix * mLocalTransform.matrix;
	}
	else
	{
		mGlobalTransform.matrix = mLocalTransform.matrix;
	}

	for (auto child : mChildren)
	{
		auto& childEntity = mRegistry->get<Entity>(child);
		childEntity.SetDirty();
	}
}

void Entity::SetDirty()
{
	mIsTransformDirty = true;
	for (auto child : mChildren)
	{
		auto& childEntity = mRegistry->get<Entity>(child);
		childEntity.SetDirty();
	}
}