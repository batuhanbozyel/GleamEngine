#pragma once
#include "View.h"
#include "World/World.h"

namespace Gleam {

class SceneView : public View
{
public:

	virtual void OnUpdate() override;

	virtual void OnFixedUpdate() override;

	virtual void OnRender(RendererContext& ctx, RenderTargetIdentifier target) override;

private:

	World mWorld;

};

} // namespace Gleam
