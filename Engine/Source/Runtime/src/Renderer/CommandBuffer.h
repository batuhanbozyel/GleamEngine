#pragma once

namespace Gleam {

class IndexBuffer;
class GraphicsPipeline;

class CommandBuffer
{
public:

	CommandBuffer();
	~CommandBuffer();

	void Begin() const;

	void End() const;

	void SetViewport(uint32_t width, uint32_t height) const;

	void BindPipeline(const GraphicsPipeline& pipeline) const;

	void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t baseVertex = 0, uint32_t baseInstance = 0) const;

	void DrawIndexed(const IndexBuffer& indexBuffer, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t baseVertex = 0, uint32_t baseInstance = 0) const;

private:

	NativeGraphicsHandle GetCurrentCommandBuffer() const;

	TArray<NativeGraphicsHandle> mCommandBuffers;
    
};

} // namespace Gleam
