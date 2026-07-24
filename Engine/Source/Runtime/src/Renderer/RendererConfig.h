#pragma once
#include "TextureFormat.h"

namespace Gleam {

GSTRUCT(RendererConfig, "49EEFEB3-76C0-43DF-8E45-098E8ACCD8D9", Serializable, PrettyName("Renderer Config"))
{
	GFIELD("16E56502-4CBD-4C76-A788-D6017542AA84", Serializable, PrettyName("VSync"))
	bool vsync = true;

	GFIELD("58CFE66F-AF91-45AC-A6F4-6E6B4FBEE923", Serializable, PrettyName("Triple buffering enable"))
	bool tripleBufferingEnabled = true;
};

} // namespace Gleam
