#pragma once
#include "World/EntityReference.h"

namespace Gleam {

class TransformSystem;

class Transform2D
{
	friend class TransformSystem;
public:
    
	virtual ~Transform2D() = default;

	const EntityReference& GetParent() const
	{
		return mParent;
	}

	void SetParent(const EntityReference& parent)
	{
		mParent = parent;
	}

	bool HasParent() const
	{
		return mParent;
	}

    void Translate(const Float2& translation)
    {
        mPosition += translation;
		mLocalTransform.m[12] += translation.x;
		mLocalTransform.m[13] += translation.y;

		mGlobalTransform.m[12] += translation.x;
		mGlobalTransform.m[13] += translation.y;
    }

    void Rotate(float rotation)
    {
        mIsTransformDirty = true;
        mRotation += rotation;
    }

	void Scale(const Float2& scale)
	{
		mIsTransformDirty = true;
		mScale *= scale;
	}

	void Scale(float scale)
	{
		Scale(Float2(scale));
	}

    void SetTranslation(const Float2& translation)
    {
		mGlobalTransform.m[12] = mGlobalTransform.m[12] - mPosition.x + translation.x;
		mGlobalTransform.m[13] = mGlobalTransform.m[13] - mPosition.y + translation.y;

        mPosition = translation;
		mLocalTransform.m[12] = mPosition.x;
		mLocalTransform.m[13] = mPosition.y;

    }

    void SetRotation(float rotation)
    {
        mIsTransformDirty = true;
        mRotation = rotation;
    }

	void SetScale(const Float2& scale)
    {
        mIsTransformDirty = true;
        mScale = scale;
    }

	void UpdateTransform(const Float4x4& local, const Float4x4& global)
	{
		mLocalTransform = local;
		mGlobalTransform = global;
	}

	NO_DISCARD FORCE_INLINE const Float4x4& GetWorldTransform()
	{
		return mGlobalTransform;
	}

    NO_DISCARD FORCE_INLINE const Float4x4& GetLocalTransform()
    {
        return mLocalTransform;
    }

    NO_DISCARD FORCE_INLINE const Float2& GetLocalPosition() const
    {
        return mPosition;
    }

    NO_DISCARD FORCE_INLINE float GetLocalRotation() const
    {
        return mRotation;
    }

	NO_DISCARD FORCE_INLINE const Float2& GetLocalScale() const
    {
        return mScale;
    }
    
private:
    
	EntityReference mParent = {};
    
    Float2 mPosition = Float2(0.0f, 0.0f);
    float mRotation = 0.0f;
    Float2 mScale = Float2(1.0f, 1.0f);

	Float4x4 mLocalTransform = Float4x4();
	Float4x4 mGlobalTransform = Float4x4();
    bool mIsTransformDirty = true;
    
};

} // namespace Gleam
