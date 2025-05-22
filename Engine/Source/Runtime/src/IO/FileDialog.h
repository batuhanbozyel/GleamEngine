#pragma once
#include "Filesystem.h"
#include "Container/String.h"
#include "Container/Array.h"

namespace Gleam {

class FileDialog final
{
public:

	static TArray<Filesystem::Path> Open(const TWString& filterName = L"All Files", const TWString& filterExtensions = L"*.*");
};

} // namespace Gleam
