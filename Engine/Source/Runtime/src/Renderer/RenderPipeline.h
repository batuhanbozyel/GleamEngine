#pragma once
#include "RendererList.h"
#include "RenderPipelineDescriptor.h"
#include "Assets/Asset.h"

namespace Gleam {

class RenderPipeline final : public Asset
{
public:

	RenderPipeline(const AssetReference& reference, const AssetHeader& header, const RenderPipelineDescriptor& descriptor);

	~RenderPipeline();

	void RegisterShadingPipeline(const Material* material) const;

	template<RendererType T>
	T* GetRenderer() const
	{
		return mRenderers.Find<T>();
	}

	const RendererList& GetRenderers() const
	{
		return mRenderers;
	}

private:

	RendererList mRenderers;

};

} // namespace Gleam
