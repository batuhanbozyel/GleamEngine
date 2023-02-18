#pragma once
#include "RenderTarget.h"
#include "PipelineStateDescriptor.h"

namespace Gleam {

class Application;
class RendererContext;

class View
{
    friend class Application;

public:

	Transform2D& GetTransform()
	{
		return mTransform;
	}

	const Transform2D& GetTransform() const
	{
		return mTransform;
	}

	BlendState& GetBlendState()
	{
		return mBlendState;
	}

	const BlendState& GetBlendState() const
	{
		return mBlendState;
	}

protected:

	virtual void OnCreate() {}

	virtual void OnUpdate() {}
    
    virtual void OnFixedUpdate() {}

	virtual void OnRender(RendererContext& ctx, RenderTargetIdentifier target) {}

	virtual void OnDestroy() {}

	BlendState mBlendState;

	Transform2D mTransform;

};

} // namespace Gleam
