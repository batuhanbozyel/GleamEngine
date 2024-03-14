#pragma once

namespace Gleam {

class Transform
{
public:

    virtual ~Transform() = default;
    
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

    NO_DISCARD FORCE_INLINE const Matrix4& GetTransform() const
    {
        if (mIsTransformDirty)
        {
            mIsTransformDirty = false;
            mCachedTransform = Matrix4::TRS(mPosition, mRotation, mScale);
        }
        return mCachedTransform;
    }

    NO_DISCARD FORCE_INLINE const Vector3& GetWorldPosition() const
    {
        return mPosition;
    }

    NO_DISCARD FORCE_INLINE const Quaternion& GetWorldRotation() const
    {
        return mRotation;
    }

	NO_DISCARD FORCE_INLINE const Vector3& GetWorldScale() const
    {
        return mScale;
    }
    
    NO_DISCARD FORCE_INLINE const Vector3& GetLocalPosition() const
    {
        return mPosition;
    }

    NO_DISCARD FORCE_INLINE const Quaternion& GetLocalRotation() const
    {
        return mRotation;
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
    
    Vector3 mPosition = Vector3(0.0f, 0.0f, 0.0f);
    Quaternion mRotation = Quaternion::identity;
    Vector3 mScale = Vector3(1.0f, 1.0f, 1.0f);

    mutable Matrix4 mCachedTransform = Matrix4();
    mutable bool mIsTransformDirty = true;
    
};

} // namespace Gleam
