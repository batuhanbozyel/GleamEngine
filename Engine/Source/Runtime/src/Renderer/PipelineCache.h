#pragma once
#include "PipelineStateDescriptor.h"

namespace Gleam {

class GraphicsShader;
struct VertexLayoutDescriptor;
enum class PrimitiveTopology;

class PipelineCache
{
public:

	static void Init();

	static void Destroy();

	static NativeGraphicsHandle CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineStateDescriptor, const TArray<VertexLayoutDescriptor>& vertexDescriptors, const Ref<GraphicsShader>& shader, PrimitiveTopology topology);

	static void Flush();

private:

	//static inline Map<PipelineStateDescriptor, NativeGraphicsHandle> mGraphicsPipelineCache;

};

} // namespace Gleam