#pragma once
#include "Gleam.h"

#ifdef USE_METAL_RENDERER
#define IMGUI_IMAGE_HANDLE(x) ((__bridge ImTextureID)(x))
#else
#define IMGUI_IMAGE_HANDLE(x) ((ImTextureID)(x))
#endif

namespace GEditor {

class ImGuiBackend
{
public:

	ImGuiBackend();

	~ImGuiBackend();

	void BeginFrame() const;

	void EndFrame(NativeGraphicsHandle commandBuffer, NativeGraphicsHandle renderCommandEncoder) const;

};

} // namespace GEditor
