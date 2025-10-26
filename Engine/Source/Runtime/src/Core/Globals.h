#pragma once
#include "Container/String.h"
#include "IO/Filesystem.h"

namespace Gleam {

class Engine;
class Application;

struct Globals final
{
	static inline Engine* Engine;

	static inline Application* GameInstance;

	static inline TString ProjectName;

	static inline Path StartupDirectory;

	static inline Path ProjectDirectory;

	static inline Path BuiltinAssetsDirectory;

	static inline Path ProjectContentDirectory;
};

} // namespace Gleam
