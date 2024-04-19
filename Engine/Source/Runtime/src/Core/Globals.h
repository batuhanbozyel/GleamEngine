#pragma once

namespace Gleam {

struct Globals final
{
	static inline TString ProjectName;

	static inline Filesystem::path StartupDirectory;

	static inline Filesystem::path ProjectDirectory;

	static inline Filesystem::path BuiltinAssetsDirectory;

	static inline Filesystem::path ProjectContentDirectory;
};

} // namespace Gleam
