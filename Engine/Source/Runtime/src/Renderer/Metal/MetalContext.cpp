#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include <SDL_metal.h>

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include "Core/Window.h"
#include "Core/ApplicationConfig.h"
#include "Renderer/RendererContext.h"

using namespace Gleam;

RendererContext::RendererContext(const Window& window, const TString& appName, const Version& appVersion, const RendererProperties& props)
{

}

RendererContext::~RendererContext()
{

}

#endif
