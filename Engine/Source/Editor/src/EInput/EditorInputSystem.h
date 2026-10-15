#pragma once
#include "Core/Subsystem.h"
#include "Container/String.h"
#include "Input/PlayerInput.h"

namespace GEditor {

struct EditorGlobalActions
{
	Gleam::InputAction* undo = nullptr;
	Gleam::InputAction* redo = nullptr;
	Gleam::InputAction* play = nullptr;
	Gleam::InputAction* pause = nullptr;
	Gleam::InputAction* step = nullptr;
};

struct EditorViewportActions
{
	Gleam::InputAction* translate = nullptr;
	Gleam::InputAction* rotate = nullptr;
	Gleam::InputAction* scale = nullptr;
	Gleam::InputAction* frame = nullptr;
	Gleam::InputAction* fly = nullptr;
	Gleam::InputAction* pan = nullptr;
	Gleam::InputAction* orbit = nullptr;
	Gleam::InputAction* dolly = nullptr;
	Gleam::InputAction* look = nullptr;
	Gleam::InputAction* zoom = nullptr;
};

struct EditorFlyActions
{
	Gleam::InputAction* move = nullptr;
	Gleam::InputAction* boost = nullptr;
};

struct EditorPanelActions
{
	Gleam::InputAction* rename = nullptr;
	Gleam::InputAction* remove = nullptr;
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

	Gleam::InputActionMap* GetViewportMap() const;

	Gleam::InputActionMap* GetFlyMap() const;

	const EditorGlobalActions& GetGlobalActions() const;

	const EditorViewportActions& GetViewportActions() const;

	const EditorFlyActions& GetFlyActions() const;

	const EditorPanelActions& GetPanelActions() const;

	void RequestPanelActions();

	static Gleam::TString GetShortcutLabel(const Gleam::InputAction* action);

private:

	Gleam::InputUser* mUser = nullptr;

	Gleam::PlayerInput mInput;

	Gleam::InputActionMap* mGlobalMap = nullptr;

	Gleam::InputActionMap* mViewportMap = nullptr;

	Gleam::InputActionMap* mFlyMap = nullptr;

	Gleam::InputActionMap* mPanelMap = nullptr;

	EditorGlobalActions mGlobalActions;

	EditorViewportActions mViewportActions;

	EditorFlyActions mFlyActions;

	EditorPanelActions mPanelActions;

	bool mPanelActionsRequested = false;

};

} // namespace GEditor
