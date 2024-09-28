#pragma once
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

namespace Gleam {

using EntityHandle = entt::entity;
static constexpr EntityHandle InvalidEntity = entt::null;

template<typename ... Excludes>
using Exclude = entt::exclude_t<Excludes...>;

class Entity
{
public:

	GLEAM_NONCOPYABLE(Entity);
    
    Entity() = default;
    
    Entity(EntityHandle handle, entt::registry* registry, const Guid& guid)
		: mHandle(handle), mRegistry(registry), mGuid(guid)
	{

	}

	EntityHandle GetParent() const
	{
		return mParent;
	}

	void SetParent(const EntityHandle parent);

	bool HasParent() const
	{
		return mParent != InvalidEntity;
	}

	void SetActive(bool active)
	{
		mActive = active;
	}

	bool IsValid() const
	{
		return mHandle != InvalidEntity && mRegistry != nullptr;
	}

	bool IsActive() const
	{
		return mActive;
	}

	const Guid& GetGuid() const
	{
		return mGuid;
	}

	template<typename T, typename ... Args>
	T& AddComponent(Args&&... args)
	{
        GLEAM_ASSERT(IsValid(), "Entity is invalid!");
		GLEAM_ASSERT(!HasComponent<T>(), "Entity already has the component!");
		const auto& desc = Reflection::GetClass<T>(); // hacky fix to register components to the database
		return mRegistry->emplace<T>(mHandle, std::forward<Args>(args)...);
	}
    
    template<typename T, typename ... Args>
    void SetComponent(Args&&... args)
    {
        GLEAM_ASSERT(IsValid(), "Entity is invalid!");
        GLEAM_ASSERT(!HasComponent<T>(), "Entity already has the component!");
        mRegistry->emplace_or_replace<T>(mHandle, std::forward<Args>(args)...);
    }

	template<typename T>
	void RemoveComponent()
	{
        GLEAM_ASSERT(IsValid(), "Entity is invalid!");
		GLEAM_ASSERT(HasComponent<T>(), "Entity does not have the component!");
		mRegistry->remove<T>(mHandle);
	}

	template<typename T>
	bool HasComponent() const
	{
        GLEAM_ASSERT(IsValid(), "Entity is invalid!");
		return mRegistry->all_of<T>(mHandle);
	}

	template<typename T>
	T& GetComponent()
	{
        GLEAM_ASSERT(IsValid(), "Entity is invalid!");
		GLEAM_ASSERT(HasComponent<T>(), "Entity does not have the component!");
		return mRegistry->get<T>(mHandle);
	}
    
    template<typename T>
    const T& GetComponent() const
    {
        GLEAM_ASSERT(IsValid(), "Entity is invalid!");
        GLEAM_ASSERT(HasComponent<T>(), "Entity does not have the component!");
        return mRegistry->get<T>(mHandle);
    }

	void Translate(const Float3& translation);

	void Rotate(const Quaternion& rotation);

	void Rotate(const Float3& eulers);

	void Rotate(float xAngle, float yAngle, float zAngle);
	void Scale(const Float3& scale);

	void Scale(float scale);

	void SetTranslation(const Float3& translation);

	void SetRotation(const Quaternion& rotation);

	void SetScale(const Float3& scale);

	NO_DISCARD FORCE_INLINE const Transform& GetWorldTransform() const
	{
		UpdateTransform();
		return mGlobalTransform;
	}

	NO_DISCARD FORCE_INLINE const Transform& GetLocalTransform() const
	{
		UpdateTransform();
		return mLocalTransform;
	}

	NO_DISCARD FORCE_INLINE const Float3& GetWorldPosition() const
	{
		return mGlobalTransform.position;
	}

	NO_DISCARD FORCE_INLINE const Float3& GetLocalPosition() const
	{
		return mLocalTransform.position;
	}

	NO_DISCARD FORCE_INLINE const Quaternion& GetWorldRotation() const
	{
		return mGlobalTransform.rotation;
	}

	NO_DISCARD FORCE_INLINE const Quaternion& GetLocalRotation() const
	{
		return mLocalTransform.rotation;
	}

	NO_DISCARD FORCE_INLINE const Float3& GetWorldScale() const
	{
		return mGlobalTransform.scale;
	}

	NO_DISCARD FORCE_INLINE const Float3& GetLocalScale() const
	{
		return mLocalTransform.scale;
	}

	NO_DISCARD FORCE_INLINE Float3 ForwardVector() const
	{
		return mGlobalTransform.rotation * Float3::forward;
	}

	NO_DISCARD FORCE_INLINE Float3 UpVector() const
	{
		return mGlobalTransform.rotation * Float3::up;
	}

	NO_DISCARD FORCE_INLINE Float3 RightVector() const
	{
		return mGlobalTransform.rotation * Float3::right;
	}

	operator EntityHandle() const
	{
		return mHandle;
	}

private:

	void UpdateTransform() const;

	bool RequiresTransformUpdate() const;
    
    bool mActive = true;

	Transform mLocalTransform;

	Transform mGlobalTransform;

	Guid mGuid = Guid::InvalidGuid();
    
	TArray<EntityHandle> mChildren;

	EntityHandle mParent = InvalidEntity;

    EntityHandle mHandle = InvalidEntity;
    
    entt::registry* mRegistry = nullptr;

	mutable bool mIsTransformDirty = true;
};

} // namespace Gleam

GLEAM_TYPE(Gleam::Entity, Guid("9662B020-8A90-47FE-8C12-2D46316A6590"))
GLEAM_END