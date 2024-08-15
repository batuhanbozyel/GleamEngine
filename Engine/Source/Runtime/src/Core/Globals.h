#pragma once

namespace Gleam {

class Engine;
class Application;

struct Globals final
{
	static inline Engine* Engine;

	static inline Application* GameInstance;

	static inline TString ProjectName;

	static inline Filesystem::Path StartupDirectory;

	static inline Filesystem::Path ProjectDirectory;

	static inline Filesystem::Path BuiltinAssetsDirectory;

	static inline Filesystem::Path ProjectContentDirectory;
};

} // namespace Gleam
