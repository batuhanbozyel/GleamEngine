#pragma once
#include "World/EntityReference.h"

namespace Gleam {

class TransformSystem;

class Transform
{
	friend class TransformSystem;
public:

    virtual ~Transform() = default;
    
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
    
    void Translate(const Float3& translation)
    {
        mPosition += translation;
		mGlobalPosition += translation;

        mLocalTransform.m[12] += mPosition.x;
        mLocalTransform.m[13] += mPosition.y;
        mLocalTransform.m[14] += mPosition.z;

		mGlobalTransform.m[12] += mPosition.x;
		mGlobalTransform.m[13] += mPosition.y;
		mGlobalTransform.m[14] += mPosition.z;
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
		mGlobalPosition = mGlobalPosition - mPosition + translation;
        mPosition = translation;

        mLocalTransform.m[12] = mPosition.x;
        mLocalTransform.m[13] = mPosition.y;
        mLocalTransform.m[14] = mPosition.z;

		mGlobalTransform.m[12] = mGlobalPosition.x;
		mGlobalTransform.m[13] = mGlobalPosition.y;
		mGlobalTransform.m[14] = mGlobalPosition.z;
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

	void UpdateTransform(const Float4x4& local, const Float4x4& global)
	{
		mIsTransformDirty = false;
		mLocalTransform = local;
		mGlobalTransform = global;
		Math::Decompose(mGlobalTransform, mGlobalPosition, mGlobalRotation, mGlobalScale);
	}
    
    NO_DISCARD FORCE_INLINE const Float4x4& GetWorldTransform() const
    {
        return mGlobalTransform;
    }

    NO_DISCARD FORCE_INLINE const Float4x4& GetLocalTransform() const
    {
        return mLocalTransform;
    }

    NO_DISCARD FORCE_INLINE const Float3& GetWorldPosition() const
    {
		return mGlobalPosition;
    }
    
    NO_DISCARD FORCE_INLINE const Float3& GetLocalPosition() const
    {
        return mPosition;
    }

    NO_DISCARD FORCE_INLINE const Quaternion& GetWorldRotation() const
    {
        return mGlobalRotation;
    }
    
    NO_DISCARD FORCE_INLINE const Quaternion& GetLocalRotation() const
    {
        return mRotation;
    }

	NO_DISCARD FORCE_INLINE const Float3& GetWorldScale() const
    {
        return mGlobalScale;
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
    
	EntityReference mParent = {};
    
    Float3 mPosition = Float3(0.0f, 0.0f, 0.0f);
    Quaternion mRotation = Quaternion::identity;
    Float3 mScale = Float3(1.0f, 1.0f, 1.0f);

	Float3 mGlobalPosition = Float3(0.0f, 0.0f, 0.0f);
	Quaternion mGlobalRotation = Quaternion::identity;
	Float3 mGlobalScale = Float3(1.0f, 1.0f, 1.0f);

    Float4x4 mLocalTransform = Float4x4();
	Float4x4 mGlobalTransform = Float4x4();
    bool mIsTransformDirty = true;
    
};

} // namespace Gleam
