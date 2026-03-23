#pragma once
#include "AccelerationStructure.h"

namespace Gleam {

class RenderSceneProxy;
class CommandBuffer;
class GraphicsDevice;
class GPUAllocator;

class RayTracingScene
{
public:

	RayTracingScene(GraphicsDevice* device, GPUAllocator* allocator);
	
	~RayTracingScene();

	void BuildAccelerationStructure(const CommandBuffer* cmd, const RenderSceneProxy* sceneProxy);

private:

	GraphicsDevice* mDevice = nullptr;
	GPUAllocator* mAllocator = nullptr;
	TopLevelAccelerationStructure mTLAS;
};
	
} // namespace Gleam
