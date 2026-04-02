#pragma once
#include "AccelerationStructure.h"
#include "PipelineStateDescriptor.h"

namespace Gleam {

class RenderSceneProxy;
class CommandBuffer;
class GraphicsDevice;
class GPUAllocator;

struct MaterialDescriptor;

using HitGroupEntry = TArray<HitGroupDescriptor, (size_t)DispatchRayType::COUNT>;
class HitGroupRegistry
{
public:

	HitGroupRegistry();

	uint32_t Register(uint32_t materialHash, const HitGroupEntry& entry);

	uint32_t GetIndex(uint32_t materialHash) const;

	bool Contains(uint32_t materialHash) const;

	const TArray<HitGroupDescriptor>& GetHitGroups() const { return mHitGroups; }

private:

	TArray<HitGroupDescriptor> mHitGroups;
	HashMap<uint32_t, uint32_t> mHashToIndex; // pipelineHash → hitGroupIndex
};

class RayTracingScene
{
public:

	RayTracingScene(GraphicsDevice* device, GPUAllocator* allocator);
	
	~RayTracingScene();

	AccelerationStructureView BuildAccelerationStructure(const CommandBuffer* cmd, const RenderSceneProxy* sceneProxy);

	void ReleaseAccelerationStructure();

	void RegisterShadingPipeline(const MaterialDescriptor& material, uint32_t hash);

	const HitGroupRegistry& GetHitGroupRegistry() const { return mHitGroupRegistry; }

private:

	GraphicsDevice* mDevice = nullptr;
	GPUAllocator* mAllocator = nullptr;
	TopLevelAccelerationStructure mTLAS;
	HitGroupRegistry mHitGroupRegistry;
};
	
} // namespace Gleam
