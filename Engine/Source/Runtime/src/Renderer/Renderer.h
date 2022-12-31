//
//  Renderer.h
//  Runtime
//
//  Created by Batuhan Bozyel on 31.10.2022.
//

#pragma once
#include "RendererContext.h"
#include "RenderPass.h"

#include "Components/MeshRenderer.h"

namespace Gleam {

struct MeshQueueElemet
{
	MeshRenderer meshRenderer;
	Matrix4 transform;
};

class Renderer : public ISystem
{
public:

	Renderer(RendererContext& context);

	virtual void Prepare() override = 0;	

	virtual void Execute() override = 0;

	virtual void Finish() override = 0;

private:

	RendererContext& mContext;

};

} // namespace Gleam
