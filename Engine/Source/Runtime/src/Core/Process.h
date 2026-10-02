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

};

} // namespace Gleam
