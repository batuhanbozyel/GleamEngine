#pragma once
#include "Renderer.h"

namespace Gleam {

class DebugRenderer final : public Renderer
{
public:

	DebugRenderer();

	~DebugRenderer();

	virtual void Render() override;
    
};

} // namespace Gleam
