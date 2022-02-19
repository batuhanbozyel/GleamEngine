#pragma once
#include "Renderer.h"

namespace Gleam {

class DebugRenderer : public Renderer
{
public:

	DebugRenderer();

	~DebugRenderer();

	virtual void Render() override;

};

} // namespace Gleam