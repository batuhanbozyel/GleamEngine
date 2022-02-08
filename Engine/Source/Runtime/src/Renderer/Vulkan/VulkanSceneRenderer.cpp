#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/SceneRenderer.h"
#include "VulkanUtils.h"

using namespace Gleam;

void SceneRenderer::Initialize()
{

}

void SceneRenderer::Shutdown()
{

}

void SceneRenderer::Render()
{
	ClearScreen({ 0.1f, 0.1f, 0.1f, 1.0f });
}

#endif