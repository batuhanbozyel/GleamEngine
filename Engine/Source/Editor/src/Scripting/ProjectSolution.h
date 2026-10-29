//
//  ProjectSolution.h
//  Editor
//

#pragma once
#include "Core/Project.h"
#include "IO/Path.h"
#include "IO/FileWatcher.h"
#include "Container/String.h"

namespace GEditor {

struct SolutionBuildResult
{
	bool success = false;
	Gleam::TString output;
};

class ProjectSolution final
{
public:

	explicit ProjectSolution(const Gleam::Project& project);

	~ProjectSolution();

	// Letters and digits kept, anything else becomes '_', a leading digit gets a '_' prefix
	static Gleam::TString MakeIdentifier(const Gleam::TString& projectName);

	static void CreateFiles(const Gleam::Project& project);

	// Writes the build CMakeLists and configures the solution
	SolutionBuildResult Generate() const;

	SolutionBuildResult BuildReflection();

	Gleam::Path GetReflectionDatabase() const;

	void OpenSolution() const;

private:

	// Configures again first when script files were added, removed or renamed since the last configure
	SolutionBuildResult Build(const Gleam::TString& target);

	Gleam::TString mIdentifier;

	Gleam::Path mProjectDirectory;

	Gleam::Path mProjectFile;

	Gleam::Path mIntermediateDirectory;

	Gleam::Path mBuildDirectory;

	Gleam::FileWatcher::Handle* mScriptsWatch = nullptr;

	bool mFileListChanged = true;
};

} // namespace GEditor
