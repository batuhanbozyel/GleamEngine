#pragma once
#include "Core/GUID.h"
#include "View/GleamTheme.h"

namespace GEditor {

enum class AssetCategory
{
	Other,
	Prefab,
	Mesh,
	Material,
	MaterialInstance,
	Texture,
	World,
	InputActions
};

struct AssetIcon
{
	const char* text = "ASSET";
	ImVec4 color = Gleam::Theme::TextDim;
	AssetCategory category = AssetCategory::Other;
};

AssetIcon GetAssetIcon(const Gleam::Guid& type);

} // namespace GEditor
