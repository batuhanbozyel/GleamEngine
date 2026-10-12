#pragma once
#include "PhysicsWorld.h"
#include "World/ComponentSystem.h"
#include "World/Entity.h"
#include "Assets/AssetReference.h"
#include "Container/Hash.h"
#include "Container/Pointer.h"

namespace Gleam {

struct RigidBodyRecord
{
	EntityHandle entity = InvalidEntity;

	RigidBodyHandle body = {};
	RigidBodyType type = RigidBodyType::Dynamic;
	RigidBodyPose previousPose = {};
	RigidBodyPose currentPose = {};

	float scale = 1.0f;
	Tick lastWriteTick = 0;
	bool isInterpolating = false;

	TArray<AssetReference> convexMeshes;
	TArray<AssetReference> triangleMeshes;
};

struct MeshColliderBlobs
{
	TArray<BinaryBuffer> blobs;
	uint32_t refCount = 0;
};

using MeshColliderCache = HashMap<AssetReference, MeshColliderBlobs>;

class PhysicsSystem final : public ComponentSystem
{
	static constexpr uint32_t InvalidRecordIndex = ~0u;
public:

	virtual void OnCreate(EntityManager& entityManager) override;

	virtual void OnFixedUpdate(EntityManager& entityManager) override;

	virtual void OnUpdate(EntityManager& entityManager) override;

	virtual void OnDestroy(EntityManager& entityManager) override;

	PhysicsWorld& GetPhysicsWorld()
	{
		return *mPhysicsWorld;
	}

	const PhysicsWorld& GetPhysicsWorld() const
	{
		return *mPhysicsWorld;
	}

	void SetGravity(const Float3& gravity);

	Float3 GetGravity() const
	{
		return mGravity;
	}

	RigidBodyHandle GetRigidBody(EntityHandle entity) const;

private:

	void UpdateRigidBodyPoses();

	void ResetRigidBodyPose(RigidBodyRecord& record, const RigidBodyPose& pose);

	void ApplyRigidBodyTransform(EntityManager& entityManager, RigidBodyRecord& record, float alpha);

	void RestoreDynamicDescendants(EntityManager& entityManager, const Entity& entity, float alpha);

	uint32_t FindRigidBodyIndex(EntityHandle entity) const;

	RigidBodyRecord CreateRigidBodyRecord(const Entity& entity, const RigidBody& rigidBody);

	void ReplaceRigidBodyRecord(const Entity& entity, const RigidBody& rigidBody, uint32_t index);

	void DestroyRigidBodyRecord(const RigidBodyRecord& record);

	void OnRigidBodyRemoved(EntityHandle entity);

	const TArray<BinaryBuffer>& CreateConvexMeshes(const AssetReference& ref);

	void DestroyConvexMesh(const AssetReference& ref);

	const TArray<BinaryBuffer>& CreateTriangleMeshes(const AssetReference& ref);

	void DestroyTriangleMesh(const AssetReference& ref);

	Scope<PhysicsWorld> mPhysicsWorld;

	TArray<RigidBodyRecord> mRigidBodies;

	TArray<uint32_t> mRigidBodyIndices;

	MeshColliderCache mConvexHulls;

	MeshColliderCache mTriangleMeshes;

	ChangeCursor mChangeCursor;

	ComponentCallback mRigidBodyRemoved;

	Float3 mGravity = Float3{ 0.0f, -9.81f, 0.0f };

};

} // namespace Gleam
