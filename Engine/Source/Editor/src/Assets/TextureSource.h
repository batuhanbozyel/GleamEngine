#pragma once

namespace GEditor {

struct TextureSource
{
	struct ImportSettings
	{
		bool combineMeshes = false;
	};

	static TextureSource Import(const Gleam::Filesystem::path& path);
};

} // namespace GEditor
