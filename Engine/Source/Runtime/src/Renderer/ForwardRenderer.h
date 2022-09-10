#pragma once
#include "Mesh.h"
#include "Shader.h"
#include "Renderer.h"
#include "CommandBuffer.h"

namespace Gleam {

class ForwardRenderer final : public Renderer
{
public:

    ForwardRenderer();

    ~ForwardRenderer();

    virtual void Render() override;

    void DrawMesh(const Mesh* mesh, const Matrix4& transform);
    
private:
    
    CommandBuffer mCommandBuffer;
    GraphicsShader mForwardPassProgram;

    TArray<std::pair<const Mesh*, Matrix4>> mMeshes;

};

} // namespace Gleam
