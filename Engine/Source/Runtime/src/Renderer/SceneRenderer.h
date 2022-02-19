#pragma once
#include "Renderer.h"
#include "Buffer.h"

namespace Gleam {

class Buffer;
class GraphicsShader;

class SceneRenderer : public Renderer
{
public:

	SceneRenderer();

	~SceneRenderer();

	virtual void Render() override;

private:

	Buffer mVertexBuffer;
	Buffer mIndexBuffer;

};

} // namespace Gleam