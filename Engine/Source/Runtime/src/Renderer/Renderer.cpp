//
//  Renderer.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 31.10.2022.
//

#include "gpch.h"
#include "Renderer.h"
#include "CommandBuffer.h"

using namespace Gleam;

Renderer::Renderer(RendererContext& context)
    : mContext(context)
{
    
}

void Renderer::Execute()
{
	RenderingData renderingData;
	renderingData.cameraBuffer = *mCameraBuffers[frameIndex];
	renderingData.frameIndex = frameIndex;

	CommandBuffer cmd();
	cmd.Begin();

	for (const auto& renderPass : mRenderPasses)
	{
		const auto& passData = renderPass->Configure(mContext);
		cmd.BeginRenderPass(passData.renderPassDesc);
		cmd.SetViewport(passData.renderPassDesc.size);
		
		renderPass->Execute(cmd);
		
		cmd.EndRenderPass();
	}

	cmd.End();
	cmd.Commit();
}