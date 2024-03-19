#pragma once
#include "World/Entity.h"

namespace Gleam {

class Transform2D
{
public:
    
    virtual ~Transform2D() = default;
    
    void SetParent(Entity parent)
    {
        mParent = parent;
    }

    void Translate(const Vector2& translation)
    {
        mPosition += translation;
        mCachedTransform.m[12] += mPosition.x;
        mCachedTransform.m[13] += mPosition.y;
    }

    void Rotate(float rotation)
    {
        mIsTransformDirty = true;
        mRotation += rotation;
    }

	void Scale(const Vector2& scale)
	{
		mIsTransformDirty = true;
		mScale *= scale;
	}

	void Scale(float scale)
	{
		Scale(Vector2(scale));
	}

    void SetTranslation(const Vector2& translation)
    {
        mPosition = translation;
        mCachedTransform.m[12] = mPosition.x;
        mCachedTransform.m[13] = mPosition.y;
    }

    void SetRotation(float rotation)
    {
        mIsTransformDirty = true;
        mRotation = rotation;
    }

	void SetScale(const Vector2& scale)
    {
        mIsTransformDirty = true;
        mScale = scale;
    }

    NO_DISCARD FORCE_INLINE const Matrix4& GetLocalTransform()
    {
        if (mIsTransformDirty)
        {
            mIsTransformDirty = false;
            mCachedTransform = Matrix4::TRS(mPosition, Quaternion(mRotation), mScale);
        }
        return mCachedTransform;
    }

    NO_DISCARD FORCE_INLINE const Vector2& GetLocalPosition() const
    {
        return mPosition;
    }

    NO_DISCARD FORCE_INLINE float GetLocalRotation() const
    {
        return mRotation;
    }

	NO_DISCARD FORCE_INLINE const Vector2& GetLocalScale() const
    {
        return mScale;
    }
    
private:
    
    Entity mParent = InvalidEntity;
    
    Vector2 mPosition = Vector2(0.0f, 0.0f);
    float mRotation = 0.0f;
    Vector2 mScale = Vector2(1.0f, 1.0f);

	Matrix4 mCachedTransform = Matrix4();
    bool mIsTransformDirty = true;
    
};

} // namespace Gleam
