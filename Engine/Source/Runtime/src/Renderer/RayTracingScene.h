#pragma once
#include "AccelerationStructure.h"
#include "PipelineStateDescriptor.h"

namespace Gleam {

class RenderSceneProxy;
class CommandBuffer;
class GraphicsDevice;
class GPUAllocator;
class Material;

struct MaterialDescriptor;

enum class RayType;

class HitGroupRegistry
{
public:

	HitGroupRegistry();

	uint32_t Register(uint32_t surfaceHash);

	uint32_t GetIndex(uint32_t surfaceHash) const;

	bool Contains(uint32_t surfaceHash) const;

private:

	HashMap<uint32_t, uint32_t> mHashToIndex; // surfaceHash → hitGroupIndex

};

class RayTracingScene
{
public:

	RayTracingScene(GraphicsDevice* device, GPUAllocator* allocator);
	
	~RayTracingScene();

	AccelerationStructureView BuildAccelerationStructure(const CommandBuffer* cmd, const RenderSceneProxy* sceneProxy);

	void ReleaseAccelerationStructure();

	void RegisterShadingPipeline(const Material* material);

	const HitGroupRegistry& GetRegistry() const;

private:

	GraphicsDevice* mDevice = nullptr;
	GPUAllocator* mAllocator = nullptr;
	TopLevelAccelerationStructure mTLAS;
	HitGroupRegistry mHitGroupRegistry;
	
};

class HitGroupTable
{
public:

	HitGroupTable(const RayTracingScene* rtScene);

	HitGroupTable& AddPrimaryRay(uint32_t registryHash, HitGroupDescriptor&& descriptor);

	HitGroupTable& AddShadowRay(uint32_t registryHash, HitGroupDescriptor&& descriptor);

	bool Contains(uint32_t registryHash, RayType rayType) const;

	const TArray<HitGroupDescriptor>& GetDescriptors() const;

private:

	void AddRay(uint32_t registryHash, HitGroupDescriptor&& descriptor, RayType rayType);

	Ref<const HitGroupRegistry> mRegistry;
	TArray<HitGroupDescriptor> mHitGroups;

};
	
} // namespace Gleam
