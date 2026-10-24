#include "EditorInputSystem.h"

#include "Core/Globals.h"
#include "Core/Engine.h"
#include "Input/InputSystem.h"

#include <imgui.h>

using namespace GEditor;

void EditorInputSystem::Initialize(Gleam::Application* app)
{
	using Gleam::Key;
	using Gleam::MouseControl;
	using Gleam::ModifierKeys;
	using Gleam::InputValueType;
	using Gleam::InputBindingType;
	using Gleam::InputActionDescriptor;
	using Gleam::InputBindingDescriptor;
	using Gleam::InputActionMapDescriptor;
	using Gleam::InputActionAssetDescriptor;

	auto inputSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::InputSystem>();
	mUser = inputSystem->CreateUser();
	mUser->PairDevice(inputSystem->GetKeyboard());
	mUser->PairDevice(inputSystem->GetMouse());

	InputActionAssetDescriptor actions = InputActionAssetDescriptor::Make({
		InputActionMapDescriptor::Make("Editor.Global", GlobalPriority,
		{
			InputActionDescriptor::Make("Undo", InputValueType::Bool, { InputBindingDescriptor::MakeKeyBinding(Key::Z, ModifierKeys::Shortcut) }),
			InputActionDescriptor::Make("Redo", InputValueType::Bool, { InputBindingDescriptor::MakeKeyBinding(Key::Z, ModifierKeys::Shortcut | ModifierKeys::Shift), InputBindingDescriptor::MakeKeyBinding(Key::Y, ModifierKeys::Shortcut) }),
			InputActionDescriptor::Make("Play", InputValueType::Bool, { InputBindingDescriptor::MakeKeyBinding(Key::P, ModifierKeys::Shortcut) }),
			InputActionDescriptor::Make("Pause", InputValueType::Bool, { InputBindingDescriptor::MakeKeyBinding(Key::P, ModifierKeys::Shortcut | ModifierKeys::Shift) }),
			InputActionDescriptor::Make("Step", InputValueType::Bool, { InputBindingDescriptor::MakeKeyBinding(Key::F10) })
		}),
		InputActionMapDescriptor::Make("Editor.Viewport", ViewportPriority,
		{
			InputActionDescriptor::Make("Translate", InputValueType::Bool, { InputBindingDescriptor::MakeKeyBinding(Key::W) }),
			InputActionDescriptor::Make("Rotate", InputValueType::Bool, { InputBindingDescriptor::MakeKeyBinding(Key::E) }),
			InputActionDescriptor::Make("Scale", InputValueType::Bool, { InputBindingDescriptor::MakeKeyBinding(Key::R) }),
			InputActionDescriptor::Make("Frame", InputValueType::Bool, { InputBindingDescriptor::MakeKeyBinding(Key::F) }),
			InputActionDescriptor::Make("Fly", InputValueType::Bool, { InputBindingDescriptor::MakeMouseBinding(MouseControl::Right) }),
			InputActionDescriptor::Make("Pan", InputValueType::Bool, { InputBindingDescriptor::MakeMouseBinding(MouseControl::Middle), InputBindingDescriptor::MakeMouseBinding(MouseControl::Left, ModifierKeys::Alt | ModifierKeys::Shortcut) }),
			InputActionDescriptor::Make("Orbit", InputValueType::Bool, { InputBindingDescriptor::MakeMouseBinding(MouseControl::Left, ModifierKeys::Alt) }),
			InputActionDescriptor::Make("Dolly", InputValueType::Bool, { InputBindingDescriptor::MakeMouseBinding(MouseControl::Right, ModifierKeys::Alt) }),
			InputActionDescriptor::Make("Look", InputValueType::Float2, { InputBindingDescriptor::MakeMouseBinding(MouseControl::Delta) }),
			InputActionDescriptor::Make("Zoom", InputValueType::Float, { InputBindingDescriptor::MakeMouseBinding(MouseControl::ScrollY) })
		}),
		InputActionMapDescriptor::Make("Editor.Fly", FlyPriority,
		{
			InputActionDescriptor::Make("Move", InputValueType::Float3, { InputBindingDescriptor::MakeAxisBinding(InputBindingType::Axis3D, { Key::E, Key::Q, Key::A, Key::D, Key::W, Key::S }) }),
			InputActionDescriptor::Make("Boost", InputValueType::Bool, { InputBindingDescriptor::MakeKeyBinding(Key::LeftShift), InputBindingDescriptor::MakeKeyBinding(Key::RightShift) })
		}),
		InputActionMapDescriptor::Make("Editor.Panel", PanelPriority,
		{
#if defined(PLATFORM_MACOS)
			InputActionDescriptor::Make("Rename", InputValueType::Bool, { InputBindingDescriptor::MakeKeyBinding(Key::Enter), InputBindingDescriptor::MakeKeyBinding(Key::F2) }),
			InputActionDescriptor::Make("Delete", InputValueType::Bool, { InputBindingDescriptor::MakeKeyBinding(Key::Backspace, ModifierKeys::Super), InputBindingDescriptor::MakeKeyBinding(Key::Delete) })
#else
			InputActionDescriptor::Make("Rename", InputValueType::Bool, { InputBindingDescriptor::MakeKeyBinding(Key::F2) }),
			InputActionDescriptor::Make("Delete", InputValueType::Bool, { InputBindingDescriptor::MakeKeyBinding(Key::Delete) })
#endif
		})
	});
	mActions = Gleam::CreateScope<Gleam::InputActionAsset>(actions);
	mStates = Gleam::InputActionStates(mActions.get());

	mGlobalMap = mActions->FindActionMap("Editor.Global");
	mViewportMap = mActions->FindActionMap("Editor.Viewport");

	mFlyMap = mActions->FindActionMap("Editor.Fly");
	mStates.SetActionMapEnabled(mFlyMap, false);

	mPanelMap = mActions->FindActionMap("Editor.Panel");
	mStates.SetActionMapEnabled(mPanelMap, false);

	mGlobalActions = EditorGlobalActions{
		.undo = mActions->FindAction("Editor.Global/Undo"),
		.redo = mActions->FindAction("Editor.Global/Redo"),
		.play = mActions->FindAction("Editor.Global/Play"),
		.pause = mActions->FindAction("Editor.Global/Pause"),
		.step = mActions->FindAction("Editor.Global/Step")
	};

	mViewportActions = EditorViewportActions{
		.translate = mActions->FindAction("Editor.Viewport/Translate"),
		.rotate = mActions->FindAction("Editor.Viewport/Rotate"),
		.scale = mActions->FindAction("Editor.Viewport/Scale"),
		.frame = mActions->FindAction("Editor.Viewport/Frame"),
		.fly = mActions->FindAction("Editor.Viewport/Fly"),
		.pan = mActions->FindAction("Editor.Viewport/Pan"),
		.orbit = mActions->FindAction("Editor.Viewport/Orbit"),
		.dolly = mActions->FindAction("Editor.Viewport/Dolly"),
		.look = mActions->FindAction("Editor.Viewport/Look"),
		.zoom = mActions->FindAction("Editor.Viewport/Zoom")
	};

	mFlyActions = EditorFlyActions{
		.move = mActions->FindAction("Editor.Fly/Move"),
		.boost = mActions->FindAction("Editor.Fly/Boost")
	};

	mPanelActions = EditorPanelActions{
		.rename = mActions->FindAction("Editor.Panel/Rename"),
		.remove = mActions->FindAction("Editor.Panel/Delete")
	};
}

void EditorInputSystem::Shutdown(Gleam::Application* app)
{
	auto inputSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::InputSystem>();
	inputSystem->DestroyUser(mUser);
	mUser = nullptr;
	mStates = Gleam::InputActionStates();
	mActions.reset();
}

void EditorInputSystem::Tick(Gleam::Application* app)
{
	const bool typing = ImGui::GetCurrentContext() and ImGui::GetIO().WantTextInput;
	mStates.SetActionMapEnabled(mGlobalMap, typing == false);
	mStates.SetActionMapEnabled(mPanelMap, mPanelActionsRequested and typing == false);
	mPanelActionsRequested = false;

	auto inputSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::InputSystem>();
	inputSystem->UpdateActions(mStates, mUser);
}

Gleam::InputActionMapHandle EditorInputSystem::GetViewportMap() const
{
	return mViewportMap;
}

Gleam::InputActionMapHandle EditorInputSystem::GetFlyMap() const
{
	return mFlyMap;
}

void EditorInputSystem::SetActionMapEnabled(Gleam::InputActionMapHandle map, bool enabled)
{
	mStates.SetActionMapEnabled(map, enabled);
}

const Gleam::InputActionState& EditorInputSystem::GetAction(Gleam::InputActionHandle action) const
{
	return mStates.GetAction(action);
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

Gleam::TString EditorInputSystem::GetShortcutLabel(Gleam::InputActionHandle action) const
{
	const auto& bindings = mActions->GetAction(action).bindings;
	return bindings.empty() ? Gleam::TString() : bindings.front().GetDisplayName();
}

Gleam::InputUser* EditorInputSystem::GetUser() const
{
	return mUser;
}
