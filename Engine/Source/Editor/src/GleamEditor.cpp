// EntryPoint
#include "Core/EntryPoint.h"

#include "World/WorldManager.h"

#include "Launcher/GleamLauncher.h"
#include "EAssets/EAssetManager.h"
#include "EWorld/EWorldManager.h"
#include "EInput/EditorInputSystem.h"
#include "Config/EditorConfigSystem.h"
#include "Config/EditorPreferences.h"
#include "Selection/SelectionSystem.h"
#include "Undo/UndoSystem.h"
#include "Scripting/ScriptReflection.h"
#include "View/ViewStack.h"
#include "View/EditorLayout.h"
#include "View/Widgets/PropertyDrawer.h"
#include "World/World.h"
#include "Physics/PhysicsSystem.h"
#include "Input/PlayerInputSystem.h"
#include "Renderer/Renderers/ImGuiRenderer.h"

#include "View/Panels/MenuBar/MenuBar.h"
#include "View/Panels/MenuBar/StatusBar.h"
#include "View/Panels/World/WorldViewport.h"
#include "View/Panels/World/WorldOutliner.h"
#include "View/Panels/Entity/EntityInspector.h"
#include "View/Panels/Project/ContentBrowser.h"
#include "View/Panels/Project/ProjectSettings.h"
#include "View/Panels/Project/Preferences.h"
#include "View/Panels/Project/AssetImportDialog.h"
#include "View/Panels/Console/Console.h"
#include "View/Panels/Input/InputActionsEditor.h"

#include <Editor.Reflection.generated.h>
#include <Editor.Reflection.types.inl>

namespace GEditor {

class GleamEditor : public Gleam::Application
{
public:

	GleamEditor(const Gleam::Project& project)
        : Gleam::Application(project)
	{
		auto assetManager = AddSubsystem<EAssetManager>(Gleam::Globals::ProjectContentDirectory);
		auto editorConfig = Gleam::Globals::Engine->AddSubsystem<EditorConfigSystem>();
		const auto& preferences = editorConfig->Register<EditorPreferences>();

		auto worldManager = GetSubsystem<Gleam::WorldManager>();
		mEditWorld = worldManager->GetActiveWorld();
		mEditWorld->GetSystem<Gleam::PhysicsSystem>()->Enabled = false;
		mEditWorld->GetSystem<Gleam::PlayerInputSystem>()->Enabled = false;
		mEditWorld->AddSubsystem<UndoSystem>();
		mEditWorld->AddSubsystem<SelectionSystem>();
		AddSubsystem<EWorldManager>(mEditWorld);
		AddSubsystem<EditorInputSystem>();

		const auto iniPath = EditorConfigSystem::UserDirectory(Gleam::Globals::ProjectName, Gleam::Globals::ProjectDirectory) / "Editor.ini";
		const bool hasLayout = Gleam::Filesystem::Exists(iniPath);
		auto viewStack = AddSubsystem<ViewStack>(iniPath, Gleam::Math::Clamp(preferences.uiScale, 0.75f, 2.0f));
		if (hasLayout == false)
		{
			viewStack->GetImGuiRenderer()->ApplyLayout(EditorLayout);
		}
		PropertyDrawer::SetFonts(&viewStack->GetFonts());
		viewStack->AddView<MenuBar>(mEditWorld);
		viewStack->AddView<StatusBar>();
		viewStack->AddView<WorldViewport>(mEditWorld);
		viewStack->AddView<WorldOutliner>(mEditWorld);
		viewStack->AddView<EntityInspector>(mEditWorld);
		viewStack->AddView<ContentBrowser>(assetManager);
		viewStack->AddView<Console>();
		viewStack->AddView<ProjectSettings>();
		viewStack->AddView<Preferences>();
		viewStack->AddView<AssetImportDialog>(assetManager);
		viewStack->AddView<InputActionsEditor>(assetManager);
	}
    
	~GleamEditor()
	{
		RemoveSubsystem<ViewStack>();
		RemoveSubsystem<EditorInputSystem>();
		mEditWorld->RemoveSubsystem<SelectionSystem>();
		mEditWorld->RemoveSubsystem<UndoSystem>();
		RemoveSubsystem<EWorldManager>();
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
	GLEAM_FOR_EACH_EDITOR_REFLECTED_CLASS(REGISTER_TYPE_LIFECYCLE)

	auto projectFile = GetProjectFile(cli);
	if (projectFile.Empty())
	{
		return new GEditor::GleamLauncher();
	}

	auto project = Gleam::Project::Load(projectFile);
	auto scriptReflection = Gleam::Globals::Engine->AddSubsystem<GEditor::ScriptReflection>(project);
	if (scriptReflection->Refresh({}) == false)
	{
		Gleam::Globals::Engine->RemoveSubsystem<GEditor::ScriptReflection>();
		return new GEditor::GleamLauncher();
	}
	return new GEditor::GleamEditor(project);
}
