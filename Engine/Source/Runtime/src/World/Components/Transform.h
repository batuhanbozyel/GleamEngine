#pragma once
#include "World/Entity.h"

namespace Gleam {

class Transform
{
public:

    virtual ~Transform() = default;
    
    Entity GetParent() const
    {
        return mParent;
    }
    
    void SetParent(Entity parent)
    {
        mParent = parent;
    }
    
    bool HasParent() const
    {
        return mParent.IsValid();
    }
    
    void Translate(const Float3& translation)
    {
        mPosition += translation;
        mCachedTransform.m[12] += mPosition.x;
        mCachedTransform.m[13] += mPosition.y;
        mCachedTransform.m[14] += mPosition.z;
    }

    void Rotate(const Quaternion& rotation)
    {
        mIsTransformDirty = true;
        mRotation *= rotation;
    }

    void Rotate(const Float3& eulers)
    {
        Rotate(Quaternion(eulers));
    }

    void Rotate(float xAngle, float yAngle, float zAngle)
    {
        Rotate(Float3{xAngle, yAngle, zAngle});
    }

	void Scale(const Float3& scale)
	{
		mIsTransformDirty = true;
        mScale *= scale;
	}

	void Scale(float scale)
	{
		Scale(Float3(scale));
	}

    void SetTranslation(const Float3& translation)
    {
        mPosition = translation;
        mCachedTransform.m[12] = mPosition.x;
        mCachedTransform.m[13] = mPosition.y;
        mCachedTransform.m[14] = mPosition.z;
    }

    void SetRotation(const Quaternion& rotation)
    {
        mIsTransformDirty = true;
        mRotation = rotation;
    }

	void SetScale(const Float3& scale)
	{
		mIsTransformDirty = true;
        mScale = scale;
	}
    
    NO_DISCARD FORCE_INLINE Float4x4 GetWorldTransform() const
    {
        if (mParent.IsValid())
        {
            const auto& parent = mParent.GetComponent<Transform>();
            return parent.GetWorldTransform() * GetLocalTransform();
        }
        return GetLocalTransform();
    }

    NO_DISCARD FORCE_INLINE const Float4x4& GetLocalTransform() const
    {
        if (mIsTransformDirty)
        {
            mIsTransformDirty = false;
            mCachedTransform = Float4x4::TRS(mPosition, mRotation, mScale);
        }
        return mCachedTransform;
    }

    NO_DISCARD FORCE_INLINE Float3 GetWorldPosition() const
    {
        if (mParent.IsValid())
        {
            const auto& parent = mParent.GetComponent<Transform>();
            auto position = parent.GetWorldTransform() * Float4(mPosition, 1.0f);
            return { position.x, position.y, position.z };
        }
        return mPosition;
    }
    
    NO_DISCARD FORCE_INLINE const Float3& GetLocalPosition() const
    {
        return mPosition;
    }

    NO_DISCARD FORCE_INLINE Quaternion GetWorldRotation() const
    {
        if (mParent.IsValid())
        {
            const auto& parent = mParent.GetComponent<Transform>();
            return parent.GetWorldTransform() * mRotation;
        }
        return mRotation;
    }
    
    NO_DISCARD FORCE_INLINE const Quaternion& GetLocalRotation() const
    {
        return mRotation;
    }

	NO_DISCARD FORCE_INLINE Float3 GetWorldScale() const
    {
        if (mParent.IsValid())
        {
            const auto& parent = mParent.GetComponent<Transform>();
            auto scale = parent.GetWorldTransform() * Float4(mScale, 1.0f);
            return { scale.x, scale.y, scale.z };
        }
        return mScale;
    }

    NO_DISCARD FORCE_INLINE const Float3& GetLocalScale() const
    {
        return mScale;
    }

    NO_DISCARD FORCE_INLINE Float3 ForwardVector() const
    {
        return mRotation * Float3::forward;
    }

    NO_DISCARD FORCE_INLINE Float3 UpVector() const
    {
        return mRotation * Float3::up;
    }

    NO_DISCARD FORCE_INLINE Float3 RightVector() const
    {
        return mRotation * Float3::right;
    }

    NO_DISCARD FORCE_INLINE Float3 EulerAngles() const
    {
        return mRotation.EulerAngles();
    }
    
private:
    
    Entity mParent = {};
    
    Float3 mPosition = Float3(0.0f, 0.0f, 0.0f);
    Quaternion mRotation = Quaternion::identity;
    Float3 mScale = Float3(1.0f, 1.0f, 1.0f);

    mutable Float4x4 mCachedTransform = Float4x4();
    mutable bool mIsTransformDirty = true;
    
};

} // namespace Gleam
