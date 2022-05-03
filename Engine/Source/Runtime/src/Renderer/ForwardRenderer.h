#pragma once
#include "Renderer.h"
#include "Buffer.h"
#include "Shader.h"
#include "GraphicsTypes.h"

namespace Gleam {

class GraphicsShader;

class ForwardRenderer : public Renderer
{
public:

	ForwardRenderer();

	~ForwardRenderer();

	virtual void Render() override;

private:
    
    GraphicsShader mForwardPassProgram;
	VertexBuffer<Vertex> mVertexBuffer;
	IndexBuffer mIndexBuffer;

};

} // namespace Gleam
