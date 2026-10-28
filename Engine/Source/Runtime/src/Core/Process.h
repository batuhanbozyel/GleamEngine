#pragma once
#include "IO/Path.h"
#include "Container/Array.h"
#include "Container/String.h"

namespace Gleam {

class Process final
{
public:

	static bool Launch(const Path& executable, const TArray<TString>& arguments, const Path& workingDirectory);

	static Path ExecutablePath();

	static void RevealInFileBrowser(const Path& path);

	static void OpenWithDefaultApplication(const Path& path);

	// After a fatal crash the log is copied to crashLog and this executable is started again with the arguments
	static void RelaunchOnCrash(const Path& crashLog, const TArray<TString>& arguments);

};

} // namespace Gleam
