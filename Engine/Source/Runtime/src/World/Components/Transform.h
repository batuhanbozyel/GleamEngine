#pragma once
#include "World/Entity.h"

namespace Gleam {

class Transform
{
public:

    virtual ~Transform() = default;
    
    void SetParent(Entity parent)
    {
        mParent = parent;
    }
    
    void Translate(const Vector3& translation)
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

    void Rotate(const Vector3& eulers)
    {
        Rotate(Quaternion(eulers));
    }

    void Rotate(float xAngle, float yAngle, float zAngle)
    {
        Rotate(Vector3{xAngle, yAngle, zAngle});
    }

	void Scale(const Vector3& scale)
	{
		mIsTransformDirty = true;
        mScale *= scale;
	}

	void Scale(float scale)
	{
		Scale(Vector3(scale));
	}

    void SetTranslation(const Vector3& translation)
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

	void SetScale(const Vector3& scale)
	{
		mIsTransformDirty = true;
        mScale = scale;
	}
    
    NO_DISCARD FORCE_INLINE Matrix4 GetWorldTransform() const
    {
        if (mParent.IsValid())
        {
            const auto& parent = mParent.GetComponent<Transform>();
            return parent.GetWorldTransform() * GetLocalTransform();
        }
        return GetLocalTransform();
    }

    NO_DISCARD FORCE_INLINE const Matrix4& GetLocalTransform() const
    {
        if (mIsTransformDirty)
        {
            mIsTransformDirty = false;
            mCachedTransform = Matrix4::TRS(mPosition, mRotation, mScale);
        }
        return mCachedTransform;
    }

    NO_DISCARD FORCE_INLINE Vector3 GetWorldPosition() const
    {
        if (mParent.IsValid())
        {
            const auto& parent = mParent.GetComponent<Transform>();
            auto position = parent.GetWorldTransform() * Vector4(mPosition, 1.0f);
            return { position.x, position.y, position.z };
        }
        return mPosition;
    }
    
    NO_DISCARD FORCE_INLINE const Vector3& GetLocalPosition() const
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

	NO_DISCARD FORCE_INLINE Vector3 GetWorldScale() const
    {
        if (mParent.IsValid())
        {
            const auto& parent = mParent.GetComponent<Transform>();
            auto scale = parent.GetWorldTransform() * Vector4(mScale, 1.0f);
            return { scale.x, scale.y, scale.z };
        }
        return mScale;
    }

    NO_DISCARD FORCE_INLINE const Vector3& GetLocalScale() const
    {
        return mScale;
    }

    NO_DISCARD FORCE_INLINE Vector3 ForwardVector() const
    {
        return mRotation * Vector3::forward;
    }

    NO_DISCARD FORCE_INLINE Vector3 UpVector() const
    {
        return mRotation * Vector3::up;
    }

    NO_DISCARD FORCE_INLINE Vector3 RightVector() const
    {
        return mRotation * Vector3::right;
    }

    NO_DISCARD FORCE_INLINE Vector3 EulerAngles() const
    {
        return mRotation.EulerAngles();
    }
    
private:
    
    Entity mParent = {};
    
    Vector3 mPosition = Vector3(0.0f, 0.0f, 0.0f);
    Quaternion mRotation = Quaternion::identity;
    Vector3 mScale = Vector3(1.0f, 1.0f, 1.0f);

    mutable Matrix4 mCachedTransform = Matrix4();
    mutable bool mIsTransformDirty = true;
    
};

} // namespace Gleam
