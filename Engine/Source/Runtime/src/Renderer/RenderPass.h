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

class RenderPass
{
public:

	RenderPass(const TArray<Subpass>& subpasses, const TArray<RenderTexture>& attachments, int depthAttachmentIndex = -1);
	~RenderPass();

	NativeGraphicsHandle GetRenderPass() const
	{
		return mRenderPass;
	}

private:

	NativeGraphicsHandle mRenderPass;

};

} // namespace Gleam

