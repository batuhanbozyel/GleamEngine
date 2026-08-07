#pragma once
#include "Renderer/Renderer.h"
#include "Math/Color.h"

namespace Gleam {
class CopyCommandBuffer;
} // namespace Gleam

namespace GEditor {

class SelectionSystem;

class SelectionOutlineRenderer final : public Gleam::IRenderer
{
	static constexpr size_t InstanceMaskBytes = INSTANCE_MASK_UINTS * sizeof(uint32_t);
public:

	SelectionOutlineRenderer(SelectionSystem* selection)
		: mSelectionSystem(selection)
	{
	}

	virtual void OnCreate(const Gleam::RenderContext& context) override;

	virtual void OnDestroy(const Gleam::RenderContext& context) override;

	virtual void AddRenderPasses(Gleam::RenderGraph& graph, Gleam::RenderGraphBlackboard& blackboard) override;

	virtual Gleam::RenderStage GetStage() const override { return Gleam::RenderStage::PostProcess; }

	void SetOutlineColor(const Gleam::Color& color) { mOutlineColor = color; }

	const Gleam::Color& GetOutlineColor() const { return mOutlineColor; }

	void SetOutlineWidth(float width) { mOutlineWidth = width; }

	float GetOutlineWidth() const { return mOutlineWidth; }

private:

	SelectionSystem* mSelectionSystem = nullptr;

	Gleam::CopyCommandBuffer* mCopyCommandBuffer = nullptr;

	Gleam::ComputePipelineHandle mMaskPipeline;

	Gleam::GraphicsPipelineHandle mOutlinePipeline;

	Gleam::Buffer mInstanceMaskBuffer = {};

	Gleam::Color mOutlineColor = Gleam::Color(0.84f, 0.58f, 0.18f, 1.0f);

	float mOutlineWidth = 3.0f;

};

} // namespace GEditor
