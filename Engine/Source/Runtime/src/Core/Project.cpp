#include "gpch.h"
#include "Project.h"

#include "Serialization/JSONSerializer.h"

using namespace Gleam;

Project Project::Load(const Path& path)
{
	Project project;
	if (Filesystem::Exists(path))
	{
		auto file = Filesystem::OpenRead(path, FileType::Text);
		JSONSerializer serializer;
		project = serializer.Deserialize<Project>(file->GetStream());
	}
	project.path = path.Parent();
	return project;
}
