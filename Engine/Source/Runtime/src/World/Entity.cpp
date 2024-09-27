#include "gpch.h"
#include "Entity.h"

using namespace Gleam;

void Entity::UpdateTransform() const
{
	if (RequiresTransformUpdate())
	{
		mIsTransformDirty = false;
		mLocalTransform.matrix = Float4x4::TRS(mLocalTransform.position, mLocalTransform.rotation, mLocalTransform.scale);
		if (HasParent())
		{
			auto& parent = mRegistry->get<Entity>(mParent);
			mGlobalTransform.matrix = parent.GetWorldTransform() * mLocalTransform.matrix;
		}
		else
		{
			mGlobalTransform.matrix = mLocalTransform.matrix;
		}
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
}

void Entity::Rotate(const Quaternion& rotation)
{
	mIsTransformDirty = true;
	mLocalTransform.rotation *= rotation;
	mGlobalTransform.rotation *= rotation;
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
	mIsTransformDirty = true;
	mLocalTransform.scale *= scale;
	mGlobalTransform.scale *= scale;
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
}

void Entity::SetRotation(const Quaternion& rotation)
{
	mIsTransformDirty = true;
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
}

void Entity::SetScale(const Float3& scale)
{
	mIsTransformDirty = true;
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
}

NO_DISCARD const float4x4& Entity::GetWorldTransform() const
{
	UpdateTransform();
	return mGlobalTransform.matrix;
}

NO_DISCARD const float4x4& Entity::GetLocalTransform() const
{
	UpdateTransform();
	return mLocalTransform.matrix;
}