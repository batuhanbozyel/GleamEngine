#pragma once
#include "Model.h"

namespace GEditor {

class ModelImporter final
{
public:

	static Model ImportObj(const Gleam::Filesystem::path& path);

};

} // namespace GEditor
