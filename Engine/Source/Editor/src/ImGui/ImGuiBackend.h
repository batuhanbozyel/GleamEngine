#pragma once
#include "Gleam.h"

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
