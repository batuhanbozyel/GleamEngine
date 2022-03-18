#pragma once
#include "Renderer.h"
#include "Buffer.h"
#include "CommandBuffer.h"
#include "GraphicsTypes.h"

namespace Gleam {

class GraphicsShader;

class TileForwardRenderer : public Renderer
{
public:

	TileForwardRenderer();

	~TileForwardRenderer();

	virtual void Render() override;

private:

	CommandBuffer mCommandBuffer;
	VertexBuffer<Vertex> mVertexBuffer;
	IndexBuffer mIndexBuffer;

};

} // namespace Gleam