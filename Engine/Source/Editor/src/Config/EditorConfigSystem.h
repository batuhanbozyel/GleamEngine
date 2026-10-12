#pragma once
#include "Core/ConfigSystem.h"
#include "Core/Globals.h"

namespace GEditor {

class EditorConfigSystem final : public Gleam::ConfigSystem
{
public:

	EditorConfigSystem()
		: Gleam::ConfigSystem(UserDirectory(Gleam::Globals::ProjectName, Gleam::Globals::ProjectDirectory) / "Editor.config")
	{

	}

	static Gleam::Path UserDirectory(const Gleam::TString& projectName, const Gleam::Path& projectDirectory)
	{
		const auto hash = Gleam::StringUtils::Hash(projectDirectory.String());
		return Gleam::Globals::UserDataDirectory / fmt::format("{}-{:08X}", projectName.c_str(), hash);
	}

};

} // namespace GEditor
