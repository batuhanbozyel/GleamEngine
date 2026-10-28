//
//  ScriptProject.cpp
//  Editor
//

#include "ScriptProject.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/Process.h"
#include "IO/Process.h"
#include "IO/Filesystem.h"

#include <cctype>

using namespace GEditor;

static Gleam::Path TemplatesDirectory()
{
	return Gleam::Globals::StartupDirectory / "Resources" / "Templates" / "Scripts";
}

static Gleam::TString Quote(const Gleam::TString& value)
{
	return "\"" + value + "\"";
}

static Gleam::TString InstantiateTemplate(const Gleam::Path& path, const Gleam::TArray<eastl::pair<Gleam::TString, Gleam::TString>>& tokens)
{
	auto text = Gleam::Filesystem::OpenRead(path, Gleam::FileType::Text)->Read();
	for (const auto& [token, value] : tokens)
	{
		for (auto pos = text.find(token); pos != Gleam::TString::npos; pos = text.find(token, pos + value.size()))
		{
			text.replace(pos, token.size(), value);
		}
	}
	return text;
}

static void WriteText(const Gleam::Path& path, const Gleam::TString& text)
{
	Gleam::Filesystem::Create(path, Gleam::FileType::Text)->Write(text);
}

ScriptProject::ScriptProject(const Gleam::Project& project)
	: mIdentifier(MakeIdentifier(project.name))
	, mProjectDirectory(project.path)
	, mIntermediateDirectory(project.path / "Intermediate")
	, mBuildDirectory(project.path / "Intermediate" / "Build")
{
	mScriptsWatch = Gleam::Globals::Engine->GetSubsystem<Gleam::FileWatcher>()->AddWatch(project.path / "Scripts", [this](const Gleam::Path& path, Gleam::FileWatchEvent event)
	{
		if (event != Gleam::FileWatchEvent::Modified)
		{
			mFileListChanged = true;
		}
	});
}

ScriptProject::~ScriptProject()
{
	Gleam::Globals::Engine->GetSubsystem<Gleam::FileWatcher>()->RemoveWatch(mScriptsWatch);
}

Gleam::TString ScriptProject::MakeIdentifier(const Gleam::TString& projectName)
{
	Gleam::TString identifier;
	for (const auto c : projectName)
	{
		identifier.push_back(std::isalnum(static_cast<unsigned char>(c)) ? c : '_');
	}
	if (identifier.empty() or std::isdigit(static_cast<unsigned char>(identifier.front())))
	{
		identifier.insert(identifier.begin(), '_');
	}
	return identifier;
}

void ScriptProject::CreateFiles(const Gleam::Project& project)
{
	const auto identifier = MakeIdentifier(project.name);
	auto macro = identifier;
	for (auto& c : macro)
	{
		c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
	}

	const auto scriptsDirectory = project.path / "Scripts";
	Gleam::Filesystem::CreateDirectories(scriptsDirectory);
	WriteText(scriptsDirectory / (identifier + ".cpp"), InstantiateTemplate(TemplatesDirectory() / "EntryPoint.cpp.in",
	{
		{ "@PROJECT_IDENTIFIER@", identifier },
		{ "@PROJECT_MACRO@", macro }
	}));
	WriteText(project.path / ".gitignore", "Intermediate/\n");
}

ScriptBuildResult ScriptProject::Generate() const
{
	Gleam::Filesystem::CreateDirectories(mIntermediateDirectory);
	WriteText(mIntermediateDirectory / "CMakeLists.txt", InstantiateTemplate(TemplatesDirectory() / "CMakeLists.txt.in",
	{
		{ "@ENGINE_DIR@", Gleam::Globals::StartupDirectory.String() },
		{ "@ENGINE_BUILD_DIR@", GLEAM_ENGINE_BUILD_DIR },
		{ "@PROJECT_IDENTIFIER@", mIdentifier },
		{ "@PROJECT_DIR@", mProjectDirectory.String() }
	}));

	auto command = Quote(GLEAM_CMAKE_COMMAND) + " -S " + Quote(mIntermediateDirectory.String()) + " -B " + Quote(mBuildDirectory.String()) + " -G " + Quote(GLEAM_CMAKE_GENERATOR);
	if (Gleam::TStringView(GLEAM_CMAKE_GENERATOR_PLATFORM).empty() == false)
	{
		command += " -A " + Quote(GLEAM_CMAKE_GENERATOR_PLATFORM);
	}

	const auto result = Gleam::RunProcess(command);
	return { .success = result.exitCode == 0, .output = result.output };
}

ScriptBuildResult ScriptProject::BuildReflection()
{
	return Build(mIdentifier + "Reflection");
}

ScriptBuildResult ScriptProject::Build(const Gleam::TString& target)
{
	if (mFileListChanged)
	{
		const auto generate = Generate();
		if (generate.success == false)
		{
			return generate;
		}
		mFileListChanged = false;
	}

	const auto command = Quote(GLEAM_CMAKE_COMMAND) + " --build " + Quote(mBuildDirectory.String()) + " --config " GLEAM_BUILD_CONFIG " --target " + target;
	const auto result = Gleam::RunProcess(command);
	return { .success = result.exitCode == 0, .output = result.output };
}

void ScriptProject::OpenSolution() const
{
#if defined(PLATFORM_MACOS)
	Gleam::Process::OpenWithDefaultApplication(mBuildDirectory / (mIdentifier + ".xcodeproj"));
#elif defined(PLATFORM_WINDOWS)
	Gleam::Process::OpenWithDefaultApplication(mBuildDirectory / (mIdentifier + ".sln"));
#endif
}
