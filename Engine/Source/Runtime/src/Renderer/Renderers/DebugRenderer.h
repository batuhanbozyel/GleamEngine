#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class Mesh;
class Shader;
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

struct DebugMesh
{
	const Mesh* mesh;
	Matrix4 transform;
	Color32 color;
};

class DebugRenderer final : public IRenderer
{
public:

    virtual void OnCreate(RendererContext& context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;
    
    void DrawLine(const Vector3& start, const Vector3& end, Color32 color, bool depthTest = true);

    void DrawTriangle(const Vector3& v1, const Vector3& v2, const Vector3& v3, Color32 color, bool depthTest = true);

    void DrawQuad(const Vector3& center, float width, float height, Color32 color, bool depthTest = true);

    void DrawBoundingBox(const BoundingBox& boundingBox, Color32 color, bool depthTest = true);

    void DrawBoundingBox(const BoundingBox& boundingBox, const Matrix4& transform, Color32 color, bool depthTest = true);

	void DrawMesh(const Mesh* mesh, const Matrix4& transform, Color32 color, bool depthTest = true);
    
    void UpdateCamera(const Camera& camera);

private:

	void RenderMeshes(const CommandBuffer* cmd, const TArray<DebugMesh>& debugMeshes, bool depthTest) const;

	uint32_t mLineBufferOffset = 0;
	uint32_t mTriangleBufferOffset = 0;
	uint32_t mDepthLineBufferOffset = 0;
	uint32_t mDepthTriangleBufferOffset = 0;

    TArray<DebugLine> mLines;
    TArray<DebugLine> mDepthLines;

    TArray<DebugTriangle> mTriangles;
    TArray<DebugTriangle> mDepthTriangles;

	TArray<DebugMesh> mDebugMeshes;
    TArray<DebugMesh> mDepthDebugMeshes;

    TArray<DebugVertex> mDebugVertices;

	RefCounted<Shader> mPrimitiveVertexShader;
	RefCounted<Shader> mMeshVertexShader;
	RefCounted<Shader> mFragmentShader;
    
    Scope<Buffer> mCameraBuffer;

};

} // namespace Gleam
