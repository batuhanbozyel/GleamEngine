#pragma once
#include "Renderer.h"

namespace Gleam {

class SceneRenderer : public Renderer
{
public:

	virtual void Initialize() override;

	virtual void Shutdown() override;

	virtual void Render() override;

};

} // namespace Gleam