#pragma once
#include "Layer.h"
#include "Renderer/ForwardRenderer.h"

namespace Gleam {

class SceneLayer : public Layer
{
public:

	virtual void OnAttach() override;

	virtual void OnUpdate() override;
    
    virtual void OnFixedUpdate() override;

	virtual void OnRender() override;

	virtual void OnDetach() override;

private:

	Scope<ForwardRenderer> mRenderer;

};

} // namespace Gleam
