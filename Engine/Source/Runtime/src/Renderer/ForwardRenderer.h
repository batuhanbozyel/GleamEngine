#pragma once
#include "Renderer.h"
#include "Buffer.h"
#include "Shader.h"
#include "CommandBuffer.h"
#include "GraphicsTypes.h"

namespace Gleam {

class GraphicsShader;

class ForwardRenderer final : public Renderer
{
public:

	ForwardRenderer();

	~ForwardRenderer();

	virtual void Render() override;

private:

	CommandBuffer mCommandBuffer;
    GraphicsShader mForwardPassProgram;
	VertexBuffer<Vertex> mVertexBuffer;
	IndexBuffer mIndexBuffer;

};

} // namespace Gleam
