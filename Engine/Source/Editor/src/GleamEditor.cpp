// EntryPoint
#include "Core/EntryPoint.h"

#include "World/WorldManager.h"

#include "Launcher/GleamLauncher.h"
#include "EAssets/EAssetManager.h"
#include "Config/EditorConfigSystem.h"
#include "Selection/SelectionSystem.h"
#include "Undo/UndoSystem.h"
#include "View/ViewStack.h"
#include "World/World.h"
#include "Physics/PhysicsSystem.h"

#include "View/Panels/MenuBar/MenuBar.h"
#include "View/Panels/World/WorldViewport.h"
#include "View/Panels/World/WorldOutliner.h"
#include "View/Panels/Entity/EntityInspector.h"
#include "View/Panels/Project/ContentBrowser.h"
#include "View/Panels/Project/ProjectSettings.h"

namespace GEditor {

class GleamEditor : public Gleam::Application
{
public:

	GleamEditor(const Gleam::Project& project)
        : Gleam::Application(project)
	{
		auto assetManager = AddSubsystem<EAssetManager>(Gleam::Globals::ProjectContentDirectory);
		Gleam::Globals::Engine->AddSubsystem<EditorConfigSystem>();

		auto worldManager = GetSubsystem<Gleam::WorldManager>();
		mEditWorld = worldManager->GetActiveWorld();
		mEditWorld->GetSystem<Gleam::PhysicsSystem>()->Enabled = false;
		mEditWorld->AddSubsystem<UndoSystem>();
		mEditWorld->AddSubsystem<SelectionSystem>();

		auto viewStack = AddSubsystem<ViewStack>("Editor.ini");
		viewStack->AddView<MenuBar>(mEditWorld);
		viewStack->AddView<WorldViewport>(mEditWorld);
		viewStack->AddView<WorldOutliner>(mEditWorld);
		viewStack->AddView<EntityInspector>(mEditWorld);
		viewStack->AddView<ContentBrowser>(assetManager);
		viewStack->AddView<ProjectSettings>();
	}
    
	~GleamEditor()
	{
		RemoveSubsystem<ViewStack>();
		mEditWorld->RemoveSubsystem<SelectionSystem>();
		mEditWorld->RemoveSubsystem<UndoSystem>();
		Gleam::Globals::Engine->RemoveSubsystem<EditorConfigSystem>();
		RemoveSubsystem<EAssetManager>();
	}

private:

	Gleam::World* mEditWorld;
    
};

} // namespace GEditor

static Gleam::Path GetProjectFile(const Gleam::CommandLine& cli)
{
	auto projectFile = Gleam::Path(cli.GetParam<Gleam::TString>("project", Gleam::TString()));
	if (not projectFile.Empty() and not Gleam::Filesystem::Exists(projectFile))
	{
		GLEAM_CORE_ERROR("Project file does not exist: {0}", projectFile.String());
		projectFile = Gleam::Path();
	}
	return projectFile;
}

Gleam::Application* Gleam::CreateApplicationInstance(const Gleam::CommandLine& cli)
{
	auto projectFile = GetProjectFile(cli);
	if (projectFile.Empty())
	{
		return new GEditor::GleamLauncher();
	}
	else
	{
		return new GEditor::GleamEditor(GEditor::GleamLauncher::OpenProject(projectFile));
	}
}
