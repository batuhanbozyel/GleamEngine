#pragma once
#include "Renderer/Model.h"

namespace Gleam {

class ModelImporter final
{
public:

	static Model Import(const Filesystem::path& path);

private:

	static Model ImportObj(const Filesystem::path& path);

};

} // namespace Gleam
