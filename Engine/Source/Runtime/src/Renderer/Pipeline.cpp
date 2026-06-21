#include "gpch.h"
#include "Pipeline.h"
#include "RenderSystem.h"
#include "GraphicsDevice.h"

#include "Core/Globals.h"
#include "Core/Engine.h"

using namespace Gleam;

const GraphicsPipeline& GraphicsPipelineHandle::GetPipeline() const
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	return renderSystem->GetDevice()->GetGraphicsPipeline(*this);
}

const ComputePipeline& ComputePipelineHandle::GetPipeline() const
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	return renderSystem->GetDevice()->GetComputePipeline(*this);
}

const RayTracingPipeline& RayTracingPipelineHandle::GetPipeline() const
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	return renderSystem->GetDevice()->GetRayTracingPipeline(*this);
}

const MeshPipeline& MeshPipelineHandle::GetPipeline() const
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	return renderSystem->GetDevice()->GetMeshPipeline(*this);
}