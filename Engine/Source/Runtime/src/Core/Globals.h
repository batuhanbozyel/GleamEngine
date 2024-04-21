#pragma once

namespace Gleam {

class Engine;
class Application;

struct Globals final
{
	static inline Engine* Engine;

	static inline Application* GameInstance;

	static inline TString ProjectName;

	static inline Filesystem::path StartupDirectory;

	static inline Filesystem::path ProjectDirectory;

	static inline Filesystem::path BuiltinAssetsDirectory;

	static inline Filesystem::path ProjectContentDirectory;
};

} // namespace Gleam
