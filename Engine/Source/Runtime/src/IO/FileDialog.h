#pragma once
#include "Filesystem.h"

namespace Gleam {

class FileDialog final
{
public:

	static TArray<Filesystem::path> Open(const TWString& filterName = L"All Files", const TWString& filterExtensions = L"*.*");
};

} // namespace Gleam
