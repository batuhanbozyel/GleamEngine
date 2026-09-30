#pragma once
#include "Renderer/Renderer.h"
#include "Renderer/Shaders/ShaderTypes.h"

#include "Math/BoundingBox.h"
#include "Math/Float4x4.h"
#include "Math/Color.h"

namespace Gleam {

class LineRenderer final : public IRenderer
{
public:

	virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual RenderStage GetStage() const override { return RenderStage::Transparent; }

	void DrawLine(const Float3& start, const Float3& end, Color32 color, float thickness = 1.0f, bool depthTest = true);

	void DrawTriangle(const Float3& v1, const Float3& v2, const Float3& v3, Color32 color, float thickness = 1.0f, bool depthTest = true);

	void DrawQuad(const Float3& center, float width, float height, Color32 color, float thickness = 1.0f, bool depthTest = true);

	void DrawBoundingBox(const BoundingBox& boundingBox, Color32 color, float thickness = 1.0f, bool depthTest = true);

	void DrawBoundingBox(const BoundingBox& boundingBox, const Float4x4& transform, Color32 color, float thickness = 1.0f, bool depthTest = true);

private:

	Buffer mLineBuffer;

	TArray<LineData> mLines;
	TArray<LineData> mDepthLines;

	GraphicsPipelineHandle mPipeline;
	GraphicsPipelineHandle mDepthPipeline;

	GraphicsDevice* mDevice = nullptr;
	GPUAllocator* mAllocator = nullptr;

};

} // namespace Gleam
