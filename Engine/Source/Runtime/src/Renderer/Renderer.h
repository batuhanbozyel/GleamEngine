//
//  Renderer.h
//  Runtime
//
//  Created by Batuhan Bozyel on 31.10.2022.
//

#pragma once
#include "World/System.h"
#include "RendererContext.h"
#include "RenderTarget.h"

namespace Gleam {

struct RenderingData
{
	RenderTargetIdentifier renderTarget = SwapchainTarget;
};

class Renderer : public ISystem
{
protected:

	virtual void Render(RendererContext& context, const RenderingData& renderData) = 0;

};

} // namespace Gleam
