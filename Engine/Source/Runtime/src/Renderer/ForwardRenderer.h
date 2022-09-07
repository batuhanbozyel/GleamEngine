#pragma once
#include "Mesh.h"
#include "Camera.h"
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
    
    void DrawMesh(const Mesh* mesh, const Matrix4& transform);

    void UpdateView(Camera& camera)
    {
        mViewMatrix = camera.GetViewMatrix();
        mProjectionMatrix = camera.GetProjectionMatrix();
    }
    
private:

    Matrix4 mViewMatrix;
    Matrix4 mProjectionMatrix;
    
	CommandBuffer mCommandBuffer;
    GraphicsShader mForwardPassProgram;
    
    TArray<std::pair<const Mesh*, Matrix4>> mMeshes;

};

} // namespace Gleam
