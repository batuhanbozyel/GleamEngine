//
//  WorldRenderer.h
//  Runtime
//
//  Created by Batuhan Bozyel on 20.10.2022.
//

#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class WorldRenderer : public Renderer
{
public:

	virtual void Render(const CommandBuffer& cmd) override;

	void Update(Camera& camera);

private:

	Scope<UniformBuffer<CameraUniforms, MemoryType::Dynamic>>> mCameraBuffer;

};

} // namespace Gleam
