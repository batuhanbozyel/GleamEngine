#pragma once

namespace Gleam {

class RenderTexture;

enum class PipelineBindPoint
{
	Graphics,
	Compute
};

class Subpass
{
public:



private:



};

class RenderPass : public GraphicsObject
{
public:

	RenderPass(const TArray<Subpass>& subpasses, const TArray<RenderTexture>& attachments, int depthAttachmentIndex = -1);
	~RenderPass();

private:


};

} // namespace Gleam

