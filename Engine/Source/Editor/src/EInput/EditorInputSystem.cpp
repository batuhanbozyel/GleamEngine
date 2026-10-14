#include "EditorInputSystem.h"

#include "Core/Globals.h"
#include "Core/Engine.h"
#include "Input/InputSystem.h"

#include <imgui.h>

using namespace GEditor;

static Gleam::InputBindingDescriptor KeyBinding(Gleam::Key key, Gleam::EnumFlag<Gleam::ModifierKeys> modifiers = {})
{
	Gleam::InputBindingDescriptor binding;
	binding.type = Gleam::InputBindingType::Control;
	binding.controls.push_back(Gleam::InputControl::Make(key));
	binding.modifiers = modifiers;
	binding.layoutMapped = true;
	return binding;
}

static Gleam::InputBindingDescriptor MouseBinding(Gleam::MouseControl control, Gleam::EnumFlag<Gleam::ModifierKeys> modifiers = {})
{
	Gleam::InputBindingDescriptor binding;
	binding.type = Gleam::InputBindingType::Control;
	binding.controls.push_back(Gleam::InputControl::Make(control));
	binding.modifiers = modifiers;
	return binding;
}

static Gleam::InputBindingDescriptor AxisBinding(std::initializer_list<Gleam::Key> keys)
{
	Gleam::InputBindingDescriptor binding;
	binding.type = Gleam::InputBindingType::Axis3D;
	for (const auto key : keys)
	{
		binding.controls.push_back(Gleam::InputControl::Make(key));
	}
	return binding;
}

static Gleam::InputActionDescriptor MakeAction(const char* name, Gleam::InputValueType valueType, std::initializer_list<Gleam::InputBindingDescriptor> bindings)
{
	Gleam::InputActionDescriptor action;
	action.id = Gleam::Guid::NewGuid();
	action.name = Gleam::TString(name);
	action.valueType = valueType;
	for (const auto& binding : bindings)
	{
		action.bindings.push_back(binding);
	}
	return action;
}

static Gleam::InputActionMapDescriptor MakeMap(const char* name, std::initializer_list<Gleam::InputActionDescriptor> actions)
{
	Gleam::InputActionMapDescriptor map;
	map.id = Gleam::Guid::NewGuid();
	map.name = Gleam::TString(name);
	for (const auto& action : actions)
	{
		map.actions.push_back(action);
	}
	return map;
}

void EditorInputSystem::Initialize(Gleam::Application* app)
{
	using Gleam::Key;
	using Gleam::MouseControl;
	using Gleam::ModifierKeys;
	using Gleam::InputValueType;

	auto inputSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::InputSystem>();
	mUser = inputSystem->CreateUser();
	mUser->PairDevice(inputSystem->GetKeyboard());
	mUser->PairDevice(inputSystem->GetMouse());

	const auto global = MakeMap("Editor.Global", {
		MakeAction("Undo", InputValueType::Bool, { KeyBinding(Key::Z, ModifierKeys::Shortcut) }),
		MakeAction("Redo", InputValueType::Bool, { KeyBinding(Key::Z, ModifierKeys::Shortcut | ModifierKeys::Shift), KeyBinding(Key::Y, ModifierKeys::Shortcut) }),
		MakeAction("Play", InputValueType::Bool, { KeyBinding(Key::P, ModifierKeys::Shortcut) }),
		MakeAction("Pause", InputValueType::Bool, { KeyBinding(Key::P, ModifierKeys::Shortcut | ModifierKeys::Shift) }),
		MakeAction("Step", InputValueType::Bool, { KeyBinding(Key::F10) })
	});

	const auto viewport = MakeMap("Editor.Viewport", {
		MakeAction("Translate", InputValueType::Bool, { KeyBinding(Key::W) }),
		MakeAction("Rotate", InputValueType::Bool, { KeyBinding(Key::E) }),
		MakeAction("Scale", InputValueType::Bool, { KeyBinding(Key::R) }),
		MakeAction("Frame", InputValueType::Bool, { KeyBinding(Key::F) }),
		MakeAction("Fly", InputValueType::Bool, { MouseBinding(MouseControl::Right) }),
		MakeAction("Pan", InputValueType::Bool, { MouseBinding(MouseControl::Middle), MouseBinding(MouseControl::Left, ModifierKeys::Alt | ModifierKeys::Shortcut) }),
		MakeAction("Orbit", InputValueType::Bool, { MouseBinding(MouseControl::Left, ModifierKeys::Alt) }),
		MakeAction("Dolly", InputValueType::Bool, { MouseBinding(MouseControl::Right, ModifierKeys::Alt) }),
		MakeAction("Look", InputValueType::Float2, { MouseBinding(MouseControl::Delta) }),
		MakeAction("Zoom", InputValueType::Float, { MouseBinding(MouseControl::ScrollY) })
	});

	const auto fly = MakeMap("Editor.Fly", {
		MakeAction("Move", InputValueType::Float3, { AxisBinding({ Key::E, Key::Q, Key::A, Key::D, Key::W, Key::S }) }),
		MakeAction("Boost", InputValueType::Bool, { KeyBinding(Key::LeftShift), KeyBinding(Key::RightShift) })
	});

	const auto panel = MakeMap("Editor.Panel", {
#if defined(PLATFORM_MACOS)
		MakeAction("Rename", InputValueType::Bool, { KeyBinding(Key::Enter), KeyBinding(Key::F2) }),
		MakeAction("Delete", InputValueType::Bool, { KeyBinding(Key::Backspace, ModifierKeys::Super), KeyBinding(Key::Delete) })
#else
		MakeAction("Rename", InputValueType::Bool, { KeyBinding(Key::F2) }),
		MakeAction("Delete", InputValueType::Bool, { KeyBinding(Key::Delete) })
#endif
	});

	mGlobalMap = mUser->AddActionMap(global, GlobalPriority);
	mViewportMap = mUser->AddActionMap(viewport, ViewportPriority);
	mFlyMap = mUser->AddActionMap(fly, FlyPriority);
	mFlyMap->SetEnabled(false);
	mPanelMap = mUser->AddActionMap(panel, PanelPriority);
	mPanelMap->SetEnabled(false);

	mGlobalActions = EditorGlobalActions{
		.undo = mGlobalMap->FindAction("Undo"),
		.redo = mGlobalMap->FindAction("Redo"),
		.play = mGlobalMap->FindAction("Play"),
		.pause = mGlobalMap->FindAction("Pause"),
		.step = mGlobalMap->FindAction("Step")
	};

	mViewportActions = EditorViewportActions{
		.translate = mViewportMap->FindAction("Translate"),
		.rotate = mViewportMap->FindAction("Rotate"),
		.scale = mViewportMap->FindAction("Scale"),
		.frame = mViewportMap->FindAction("Frame"),
		.fly = mViewportMap->FindAction("Fly"),
		.pan = mViewportMap->FindAction("Pan"),
		.orbit = mViewportMap->FindAction("Orbit"),
		.dolly = mViewportMap->FindAction("Dolly"),
		.look = mViewportMap->FindAction("Look"),
		.zoom = mViewportMap->FindAction("Zoom")
	};

	mFlyActions = EditorFlyActions{
		.move = mFlyMap->FindAction("Move"),
		.boost = mFlyMap->FindAction("Boost")
	};

	mPanelActions = EditorPanelActions{
		.rename = mPanelMap->FindAction("Rename"),
		.remove = mPanelMap->FindAction("Delete")
	};
}

void EditorInputSystem::Shutdown(Gleam::Application* app)
{
	auto inputSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::InputSystem>();
	inputSystem->DestroyPlayer(mUser);
	mUser = nullptr;
}

void EditorInputSystem::Tick(Gleam::Application* app)
{
	const bool typing = ImGui::GetCurrentContext() and ImGui::GetIO().WantTextInput;
	mGlobalMap->SetEnabled(typing == false);
	mPanelMap->SetEnabled(mPanelActionsRequested and typing == false);
	mPanelActionsRequested = false;
}

Gleam::InputActionMap* EditorInputSystem::GetViewportMap() const
{
	return mViewportMap;
}

Gleam::InputActionMap* EditorInputSystem::GetFlyMap() const
{
	return mFlyMap;
}

const EditorGlobalActions& EditorInputSystem::GetGlobalActions() const
{
	return mGlobalActions;
}

const EditorViewportActions& EditorInputSystem::GetViewportActions() const
{
	return mViewportActions;
}

const EditorFlyActions& EditorInputSystem::GetFlyActions() const
{
	return mFlyActions;
}

const EditorPanelActions& EditorInputSystem::GetPanelActions() const
{
	return mPanelActions;
}

void EditorInputSystem::RequestPanelActions()
{
	mPanelActionsRequested = true;
}

Gleam::TString EditorInputSystem::GetShortcutLabel(const Gleam::InputAction* action)
{
	const auto& bindings = action->GetBindings();
	return bindings.empty() ? Gleam::TString() : bindings.front().GetDisplayName();
}
