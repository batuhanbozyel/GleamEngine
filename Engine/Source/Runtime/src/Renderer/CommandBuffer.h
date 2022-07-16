#pragma once

namespace Gleam {

class Buffer;
class IndexBuffer;

struct GraphicsShader;
struct RenderPassDescriptor;
struct PipelineStateDescriptor;
    
enum class ShaderStage;

class CommandBuffer final
{
public:

	CommandBuffer();
	~CommandBuffer();

	void BeginRenderPass(const RenderPassDescriptor& renderPassDesc) const;

	void EndRenderPass() const;
    
    void BindPipeline(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program) const;

	void SetViewport(uint32_t width, uint32_t height) const;
    
    void SetVertexBuffer(const Buffer& buffer, uint32_t index = 0, uint32_t offset = 0) const;
    
    void SetFragmentBuffer(const Buffer& buffer, uint32_t index = 0, uint32_t offset = 0) const;
    
    template<typename T>
    void SetPushConstant(const T& t, ShaderStage stage, uint32_t index = 0) const
    {
        SetPushConstant(&t, sizeof(T), stage, index);
    }

	void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t baseVertex = 0, uint32_t baseInstance = 0) const;

	void DrawIndexed(const IndexBuffer& indexBuffer, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t baseVertex = 0, uint32_t baseInstance = 0) const;

	void CopyBuffer(const Buffer& src, const Buffer& dst, uint32_t size, uint32_t srcOffset = 0, uint32_t dstOffset = 0) const;
    
    void Begin() const;

	void End() const;
    
    void Commit() const;

private:
    
    void SetPushConstant(const void* data, uint32_t size, ShaderStage stage, uint32_t index = 0) const;
    
    class Impl;
    Scope<Impl> mHandle;
    
};

} // namespace Gleam
