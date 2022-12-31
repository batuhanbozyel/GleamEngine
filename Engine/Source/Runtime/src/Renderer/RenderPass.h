#pragma once

namespace Gleam {

class RenderPass
{
public:

	virtual RenderPassDescriptor Configure(RendererContext& context) = 0;

	virtual void Execute(const CommandBuffer& cmd, const RenderingData& passData) = 0;

	virtual void Finish() = 0;

};

} // namespace Gleam