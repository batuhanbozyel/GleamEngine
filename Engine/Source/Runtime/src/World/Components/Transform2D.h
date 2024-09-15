#pragma once
#include "World/Entity.h"

namespace Gleam {

class Transform2D
{
public:
    
    virtual ~Transform2D() = default;

	Entity GetParent() const
	{
		return mParent;
	}

    void SetParent(Entity parent)
    {
        mParent = parent;
    }

    void Translate(const Float2& translation)
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
        mPosition = translation;
        mCachedTransform.m[12] = mPosition.x;
        mCachedTransform.m[13] = mPosition.y;
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

    NO_DISCARD FORCE_INLINE const Float4x4& GetLocalTransform()
    {
        if (mIsTransformDirty)
        {
            mIsTransformDirty = false;
            mCachedTransform = Float4x4::TRS(mPosition, Quaternion(mRotation), mScale);
        }
        return mCachedTransform;
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
    
	Entity mParent = {};
    
    Float2 mPosition = Float2(0.0f, 0.0f);
    float mRotation = 0.0f;
    Float2 mScale = Float2(1.0f, 1.0f);

	Float4x4 mCachedTransform = Float4x4();
    bool mIsTransformDirty = true;
    
};

} // namespace Gleam
