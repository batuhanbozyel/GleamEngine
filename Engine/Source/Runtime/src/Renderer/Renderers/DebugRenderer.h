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

struct DebugMesh
{
	const Mesh* mesh;
	Float4x4 transform;
	Color32 color;
};

class DebugRenderer final : public IRenderer
{
public:

    virtual void OnCreate(GraphicsDevice* device) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;
    
    void DrawLine(const Float3& start, const Float3& end, Color32 color, bool depthTest = true);

    void DrawTriangle(const Float3& v1, const Float3& v2, const Float3& v3, Color32 color, bool depthTest = true);

    void DrawQuad(const Float3& center, float width, float height, Color32 color, bool depthTest = true);

    void DrawBoundingBox(const BoundingBox& boundingBox, Color32 color, bool depthTest = true);

    void DrawBoundingBox(const BoundingBox& boundingBox, const Float4x4& transform, Color32 color, bool depthTest = true);

	void DrawMesh(const Mesh* mesh, const Float4x4& transform, Color32 color, bool depthTest = true);

private:

	void RenderMeshes(const CommandBuffer* cmd, const BufferHandle& cameraBuffer, const TArray<DebugMesh>& debugMeshes, bool depthTest) const;

	uint32_t mLineBufferOffset = 0;
	uint32_t mDepthLineBufferOffset = 0;

    TArray<DebugLine> mLines;
    TArray<DebugLine> mDepthLines;

	TArray<DebugMesh> mDebugMeshes;
    TArray<DebugMesh> mDepthDebugMeshes;

    TArray<DebugVertex> mDebugVertices;

	Shader mPrimitiveVertexShader;
	Shader mMeshVertexShader;
	Shader mFragmentShader;

};

} // namespace Gleam
