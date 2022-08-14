#pragma once
#include "Renderer/Model.h"

namespace Gleam {

class ModelImporter final
{
public:

	static Model Import(const std::filesystem::path& path);

private:

	static Model ImportObj(const std::filesystem::path& path);

};

} // namespace Gleam