#pragma once
#include "Renderer/Model.h"

namespace Gleam {

class ModelImporter final
{
public:

    static Model ImportObj(const Filesystem::path& path);

};

} // namespace Gleam
