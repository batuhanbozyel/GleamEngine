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

struct
{
	MTL::Device* Device;
} mContext;

/************************************************************************/
/*	RendererContext                           */
/************************************************************************/
RendererContext::RendererContext(const TString& appName, const Version& appVersion, const RendererProperties& props)
    : mProperties(props)
{

}
/************************************************************************/
/*	~RendererContext                          */
/************************************************************************/
RendererContext::~RendererContext()
{

}
/************************************************************************/
/*	GetDevice                                 */
/************************************************************************/
GraphicsDevice RendererContext::GetDevice() const
{
	return mContext.Device;
}
/************************************************************************/
/*    BeginFrame                              */
/************************************************************************/
void RendererContext::BeginFrame()
{
    
}
/************************************************************************/
/*    EndFrame                                */
/************************************************************************/
void RendererContext::EndFrame()
{
    
}
/************************************************************************/
/*    ClearScreen                             */
/************************************************************************/
void RendererContext::ClearScreen(const Color &color) const
{
    
}
#endif
