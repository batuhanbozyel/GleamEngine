#pragma once
#include "Core/EngineDefines.h"

#include <imgui.h>

namespace Gleam {

class Texture;
class RenderContext;
class RenderSurface;
class GraphicsDevice;

class ImGuiBackend
{
public:

	static void Init(RenderContext& context);

	static void Destroy();

	static void BeginFrame();

	static void EndFrame(NativeGraphicsHandle commandBuffer, NativeGraphicsHandle renderCommandEncoder);

	static ImTextureID GetImTextureIDForTexture(const Texture& texture);
    
private:
	
	static inline RenderSurface* mSurface = nullptr;
    
    static inline GraphicsDevice* mDevice = nullptr;

};

} // namespace Gleam
