#pragma once
#include <Reflection/Macro.h>

#include "Renderer/Renderer.h"
#include "Renderer/Shaders/ShaderTypes.h"

#include "Math/BoundingBox.h"
#include "Math/Float4x4.h"
#include "Math/Color.h"

namespace Gleam {

class Mesh;
class LineRenderer;

GSTRUCT(DebugRendererSettings, "E5250750-F7AC-41BF-8AC7-07EB720128A6", Serializable, PrettyName("Debug Lines"))
{
	GFIELD("BAB1CC87-512B-4610-AA49-BA649D30AB8D", Serializable, PrettyName("Antialiased"))
	bool antialiasedLines = true;

	GFIELD("8BEB12C7-99D1-4C5B-BD0C-2C78F5094D9F", Serializable, PrettyName("Thickness"))
	float lineThickness = 2.0f;
};

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

	explicit DebugRenderer(LineRenderer* lineRenderer);

    virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual RenderStage GetStage() const override { return RenderStage::Transparent; }
    
    void DrawLine(const Float3& start, const Float3& end, Color32 color, bool depthTest = true);

    void DrawTriangle(const Float3& v1, const Float3& v2, const Float3& v3, Color32 color, bool depthTest = true);

    void DrawQuad(const Float3& center, float width, float height, Color32 color, bool depthTest = true);

    void DrawBoundingBox(const BoundingBox& boundingBox, Color32 color, bool depthTest = true);

    void DrawBoundingBox(const BoundingBox& boundingBox, const Float4x4& transform, Color32 color, bool depthTest = true);

	void DrawMesh(const Mesh* mesh, const Float4x4& transform, Color32 color, bool depthTest = true);

	const DebugRendererSettings& GetSettings() const
	{
		return mSettings;
	}

	void SetSettings(const DebugRendererSettings& settings)
	{
		mSettings = settings;
	}

private:

	void RenderMeshes(const CommandBuffer* cmd, const CameraUniforms& cameraData, const TArray<DebugMesh>& debugMeshes, bool depthTest) const;

	Buffer mVertexBuffer;

    TArray<DebugLine> mLines;
    TArray<DebugLine> mDepthLines;

	TArray<DebugMesh> mDebugMeshes;
    TArray<DebugMesh> mDepthDebugMeshes;

	GraphicsPipelineHandle mPrimitivePipeline;
	GraphicsPipelineHandle mPrimitiveDepthPipeline;

	GraphicsPipelineHandle mMeshPipeline;
	GraphicsPipelineHandle mMeshDepthPipeline;

	LineRenderer* mLineRenderer = nullptr;

	DebugRendererSettings mSettings;

	GraphicsDevice* mDevice = nullptr;
	GPUAllocator* mAllocator = nullptr;

};

} // namespace Gleam
