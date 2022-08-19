#pragma once
#include "Buffer.h"
#include "Shader.h"
#include "Renderer.h"
#include "ShaderTypes.h"
#include "CommandBuffer.h"

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
	VertexBuffer<Vector3> mPositionBuffer;
    VertexBuffer<InterleavedMeshVertex> mInterleavedBuffer;
	IndexBuffer mIndexBuffer;

};

} // namespace Gleam
