#pragma once
#include "Core/GUID.h"
#include "Math/Color.h"

namespace GEditor {

struct AssetIcon
{
	const char* text = "?";
	Gleam::Color color = Gleam::Color(0.5f, 0.5f, 0.5f, 1.0f);
};

AssetIcon GetAssetIcon(const Gleam::Guid& type);

} // namespace GEditor
