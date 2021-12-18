#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include <SDL_metal.h>

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Metal.hpp>

#include "Core/ApplicationConfig.h"
#include "Renderer/GraphicsContext.h"

using namespace Gleam;

struct MetalContext
{

} sContext;

void GraphicsContext::Create(SDL_Window* window, const TString& appName, const Version& appVersion)
{

}

void GraphicsContext::Destroy()
{

}

#endif
