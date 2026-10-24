#pragma once
#include "Core/Subsystem.h"
#include "Container/String.h"
#include "Container/Pointer.h"
#include "Input/InputActionState.h"

namespace Gleam {
class InputUser;
} // namespace Gleam

namespace GEditor {

struct EditorGlobalActions
{
	Gleam::InputActionHandle undo;
	Gleam::InputActionHandle redo;
	Gleam::InputActionHandle play;
	Gleam::InputActionHandle pause;
	Gleam::InputActionHandle step;
};

struct EditorViewportActions
{
	Gleam::InputActionHandle translate;
	Gleam::InputActionHandle rotate;
	Gleam::InputActionHandle scale;
	Gleam::InputActionHandle frame;
	Gleam::InputActionHandle fly;
	Gleam::InputActionHandle pan;
	Gleam::InputActionHandle orbit;
	Gleam::InputActionHandle dolly;
	Gleam::InputActionHandle look;
	Gleam::InputActionHandle zoom;
};

struct EditorFlyActions
{
	Gleam::InputActionHandle move;
	Gleam::InputActionHandle boost;
};

struct EditorPanelActions
{
	Gleam::InputActionHandle rename;
	Gleam::InputActionHandle remove;
};

class EditorInputSystem final : public Gleam::TickableGameInstanceSubsystem
{
public:

	static constexpr int32_t GlobalPriority = 100;

	static constexpr int32_t PanelPriority = 70;

	static constexpr int32_t FlyPriority = 60;

	static constexpr int32_t ViewportPriority = 50;

	virtual void Initialize(Gleam::Application* app) override;

	virtual void Shutdown(Gleam::Application* app) override;

	virtual void Tick(Gleam::Application* app) override;

	Gleam::InputActionMapHandle GetViewportMap() const;

	Gleam::InputActionMapHandle GetFlyMap() const;

	void SetActionMapEnabled(Gleam::InputActionMapHandle map, bool enabled);

	const Gleam::InputActionState& GetAction(Gleam::InputActionHandle action) const;

	const EditorGlobalActions& GetGlobalActions() const;

	const EditorViewportActions& GetViewportActions() const;

	const EditorFlyActions& GetFlyActions() const;

	const EditorPanelActions& GetPanelActions() const;

	void RequestPanelActions();

	Gleam::TString GetShortcutLabel(Gleam::InputActionHandle action) const;

	Gleam::InputUser* GetUser() const;

private:

	void ApplyLetterShortcuts();

	Gleam::InputUser* mUser = nullptr;

	Gleam::Scope<Gleam::InputActionAsset> mActions;

	Gleam::InputActionStates mStates;

	Gleam::InputActionMapHandle mGlobalMap;

	Gleam::InputActionMapHandle mViewportMap;

	Gleam::InputActionMapHandle mFlyMap;

	Gleam::InputActionMapHandle mPanelMap;

	EditorGlobalActions mGlobalActions;

	EditorViewportActions mViewportActions;

	EditorFlyActions mFlyActions;

	EditorPanelActions mPanelActions;

	bool mPanelActionsRequested = false;

};

} // namespace GEditor
