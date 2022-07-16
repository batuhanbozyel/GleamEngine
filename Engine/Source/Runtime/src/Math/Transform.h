#pragma once

namespace Gleam {

class Transform
{
public:

    NO_DISCARD FORCE_INLINE const Matrix4& GetTransform()
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
    
    NO_DISCARD FORCE_INLINE Vector3 ForwardVector() const
    {
        return mRotation * Vector3::forward;
    }
    
    NO_DISCARD FORCE_INLINE Vector3 UpVector() const
    {
        return mRotation * Vector3::up;
    }
    
    NO_DISCARD FORCE_INLINE Vector3 EularAngles() const
    {
        return mRotation.EularAngles();
    }
    
    void Translate(const Vector3& translation)
    {
        mPosition += translation;
        mCachedTransform[0][3] += mPosition.x;
        mCachedTransform[1][3] += mPosition.y;
        mCachedTransform[2][3] += mPosition.z;
    }
    
    void Rotate(const Quaternion& quat)
    {
        mIsTransformDirty = true;
        mRotation *= quat;
    }
    
    void Rotate(const Vector3& eulers)
    {
        mIsTransformDirty = true;
        mRotation *= Quaternion(eulers);
    }
    
    void Rotate(float xAngle, float yAngle, float zAngle)
    {
        Rotate({xAngle, yAngle, zAngle});
    }
    
private:
    
    Vector3 mPosition = Vector3(0.0f, 0.0f, 0.0f);
    Quaternion mRotation = Quaternion::identity;
    Vector3 mScale = Vector3(1.0f, 1.0f, 1.0f);
    
    Matrix4 mCachedTransform;
    bool mIsTransformDirty = true;
    
};

} // namespace Gleam
