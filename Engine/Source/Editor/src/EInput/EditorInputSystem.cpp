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
	mActionSet.Build(actions);
	mActionSet.SetUser(mUser);
	inputSystem->AddActionSet(&mActionSet);

	mGlobalMap = mActionSet.FindActionMap("Editor.Global");
	mViewportMap = mActionSet.FindActionMap("Editor.Viewport");

	mFlyMap = mActionSet.FindActionMap("Editor.Fly");
	mFlyMap->SetEnabled(false);

	mPanelMap = mActionSet.FindActionMap("Editor.Panel");
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
	inputSystem->RemoveActionSet(&mActionSet);
	inputSystem->DestroyUser(mUser);
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
