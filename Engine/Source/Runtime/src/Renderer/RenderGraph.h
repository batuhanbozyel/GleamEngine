#pragma once
#include "RenderGraphResource.h"

namespace Gleam {

class RenderGraph;

template<typename PassData>
using RenderFunc = std::function<void(const PassData& passData, const RenderGraphContext& renderGraphContext)>;

struct RenderGraphContext
{
	const CommandBuffer& cmd;

	RenderGraphContext(const CommandBuffer& cmd)
		: cmd(cmd)
	{

	}
};

class RenderGraphBuilder final
{
	friend class RenderGraph;

	struct RenderFuncWrapper
	{
		std::any passData;
		std::function<void(const RenderGraphContext& renderGraphContext)> impl;
	};

public:

	GLEAM_NONCOPYABLE(RenderGraphBuilder);

	RenderGraphBuilder(const TStringView name)
		: mName(name)
	{

	}

	template<typename PassData>
	void SetRenderFunc(RenderFunc<PassData>&& renderFunc)
	{
		mRenderFunc.impl = [this, renderFunc = move(renderFunc)](const RenderGraphContext& renderGraphContext) mutable
		{
			renderFunc(std::any_cast<const PassData&>(mRenderFunc.passData), renderGraphContext);
		};
	}

private:

	TString mName;

	RenderFuncWrapper mRenderFunc;

};

class RenderGraph final
{
public:

	void Execute(const CommandBuffer& cmd);

	template<typename PassData>
	RenderGraphBuilder& AddRenderPass(const TStringView name, std::function<void(RenderGraphBuilder& builder, PassData& passData)>&& setup)
	{
		auto& builder = mBuilders.emplace_back(name);

		PassData passData;
		setup(builder, passData);
		builder.mRenderFunc.passData = passData;

		return builder;
	}

private:

	TArray<RenderGraphBuilder> mBuilders;

};

} // namespace Gleam