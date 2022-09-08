#pragma once
#include "Mesh.h"
#include "Camera.h"
#include "Shader.h"
#include "Renderer.h"
#include "ShaderTypes.h"
#include "CommandBuffer.h"

namespace Gleam {

enum class PrimitiveTopology;

struct DebugLine
{
    DebugVertex start;
    DebugVertex end;
};

struct DebugTriangle
{
    DebugVertex vertex1;
    DebugVertex vertex2;
    DebugVertex vertex3;
};

class DebugRenderer final : public Renderer
{
public:

	DebugRenderer();

	~DebugRenderer();

	virtual void Render() override;

	void DrawLine(const Vector3& start, const Vector3& end, Color32 color, bool depthTest = true);

	void DrawTriangle(const Vector3& v1, const Vector3& v2, const Vector3& v3, Color32 color, bool depthTest = true);

	void DrawQuad(const Vector3& center, float width, float height, Color32 color, bool depthTest = true);
    
    void DrawMesh(const Mesh* mesh, const Matrix4& transform, Color32 color, bool depthTest = true);
    
    void DrawBoundingBox(const BoundingBox& boundingBox, Color32 color, bool depthTest = true);
    
    void DrawBoundingBox(const BoundingBox& boundingBox, const Matrix4& transform, Color32 color, bool depthTest = true);
    
    void UpdateView(Camera& camera)
    {
        mViewMatrix = camera.GetViewMatrix();
        mProjectionMatrix = camera.GetProjectionMatrix();
    }

private:
    
    void RenderPrimitive(uint32_t primitiveCount, uint32_t bufferOffset, PrimitiveTopology topology, bool depthTest) const;
    
    Matrix4 mViewMatrix;
    Matrix4 mProjectionMatrix;
    
	TArray<DebugLine> mLines;
	TArray<DebugLine> mDepthLines;

	TArray<DebugTriangle> mTriangles;
	TArray<DebugTriangle> mDepthTriangles;

	CommandBuffer mCommandBuffer;
	GraphicsShader mDebugProgram;
    GraphicsShader mDebugMeshProgram;
    
    TArray<DebugVertex> mStagingBuffer;
	TArray<Scope<VertexBuffer<DebugVertex>>> mVertexBuffers;

};

} // namespace Gleam
