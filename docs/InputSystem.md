# Input System

Gameplay and editor code read input through **actions**, not raw keys. An action is bound to device controls and lives in an **action map**. Maps come from an **Input Actions** asset assigned to a `PlayerInput` component, and each `PlayerInput` reads from the devices paired with one **player**. Code lives in `Engine/Source/Runtime/src/Input/`.

## Table of Contents
* [Overview](#overview)
* [Frame Flow](#frame-flow)
* [Devices](#devices)
* [PlayerInput Component](#playerinput-component)
* [Players and Users](#players-and-users)
* [Action Maps](#action-maps)
* [Actions](#actions)
* [Bindings](#bindings)
* [Defining Actions](#defining-actions)
* [Rebinding](#rebinding)
* [Cursor](#cursor)
* [Examples](#examples)

## Overview

| Type | Role |
| --- | --- |
| `InputSystem` | Engine subsystem: owns devices and users, receives SDL events, updates every action map each frame |
| `InputDevice` | `Keyboard`, `Mouse`, `Gamepad`: raw per-frame control state |
| `InputUser` | A player or a non-player user: player index, paired devices, binding overrides, rebinding |
| `PlayerInput` | Entity component: an Input Actions asset and a player index; owns the action maps built from the asset |
| `PlayerInputManager` | Entity component: join behavior and player limit for the world |
| `InputActionMap` | A named group of actions with a priority; enabled or disabled as a group |
| `InputAction` | One logical input (Jump, Move, Undo): value, phase, callbacks |
| `InputBindingDescriptor` | Which controls drive an action, plus modifiers, processors and interactions |
| `InputActionAssetDescriptor` | Maps and actions authored in the editor's Input Actions panel |

## Frame Flow

1. `EventSystem` forwards SDL events to `InputSystem::ProcessEvent`, which writes each device's *pending* state. Keyboard, mouse and gamepad events are reported as handled.
2. `Application` calls `InputSystem::Update(world)` with the active world, right after `EventSystem::Update()` and before the world updates.
3. Every device commits: pending state becomes current. Press and release counts are kept, so a press and release inside one frame still registers as a tap.
4. If world input is enabled:
   1. The first `PlayerInputManager` in the world decides the join behavior. In `JoinOnButtonPress` mode, unpaired devices that were pressed create new players.
   2. Every `PlayerInput` component builds its maps if its asset changed, and resolves its player from `playerIndex`. In `Manual` mode a missing player is created.
   3. `PlayerJoinedEvent` fires for players created in the previous step.
5. If the OS keyboard layout changed, layout-mapped bindings are re-resolved.
6. Every map, from registered `PlayerInput`s (the editor's) and from the world's components, is sorted by priority (highest first). Active maps are updated in that order against one shared consumption set.
7. Pending interactive rebinds advance.
8. Users destroyed during steps 6 and 7 are released.

Action state and callbacks are therefore settled before any `ComponentSystem` or world subsystem runs, and stay stable for the rest of the frame.

In the editor, world input is off in edit mode and on during play (`InputSystem::SetWorldInputEnabled`), so `PlayerInput` components in the edit world never build maps or consume keys.

## Devices

The keyboard and mouse exist from startup; gamepads are created on connect. A gamepad that reconnects (same SDL GUID) gets its old `InputDevice` back, so pairings and cached pointers survive a cable pull.

Poll a device directly for debug keys and tools:

```cpp
auto input = Globals::Engine->GetSubsystem<InputSystem>();

if (input->GetKeyboard()->WasPressedThisFrame(Key::F5))
{
	ReloadLevel();
}

Float2 delta = input->GetMouse()->GetDelta();
for (auto gamepad : input->GetGamepads())
{
	Float2 stick = gamepad->GetLeftStick();
	gamepad->SetRumble(0.5f, 0.5f, 200);
}
```

| Device | Buttons | Analog controls |
| --- | --- | --- |
| `Keyboard` | every `Key` (physical scancodes) | none |
| `Mouse` | `Left`, `Right`, `Middle`, `Back`, `Forward` | `Position`, `Delta`, `Scroll` (Float2); `DeltaX/Y`, `ScrollX/Y` (Float) |
| `Gamepad` | `South`/`East`/`West`/`North`, shoulders, stick presses, D-pad, `Start`/`Select`/`Guide`, triggers (pressed at 0.5) | `LeftStick`, `RightStick` (Float2); `LeftStickX/Y`, `RightStickX/Y`, `LeftTrigger`, `RightTrigger` (Float) |

Every device answers the same generic queries: `ReadControl(code)` returns a `Float3`, and `WasControlPressedThisFrame` / `WasControlReleasedThisFrame` report edges. Buttons read 0 or 1.

`InputControl` names one control as `{ device type, code }`. Build it with `InputControl::Make(Key::Space)`, `InputControl::Make(MouseControl::Delta)` or `InputControl::Make(GamepadControl::South)`. `GetName()` returns its display name.

### Events

Subscribe through `EventDispatcher`:

```cpp
EventDispatcher<InputDeviceConnectedEvent>::Subscribe([](InputDeviceConnectedEvent e)
{
	GLEAM_INFO("Connected: {}", e.GetDevice()->GetName());
});
```

| Event | Payload |
| --- | --- |
| `InputDeviceConnectedEvent` / `InputDeviceDisconnectedEvent` | `GetDevice()` |
| `PlayerJoinedEvent` / `PlayerLeftEvent` | `GetPlayer()` (an `InputUser*`) |

## PlayerInput Component

Add **Player Input** to an entity in the inspector and assign an Input Actions asset:

| Field | Meaning |
| --- | --- |
| `actions` | The Input Actions asset. Every map in it is built for this component. |
| `playerIndex` | Which player's devices and binding overrides the maps read from. |

At runtime, look maps and actions up on the component:

```cpp
entityManager.ForEach<PlayerInput>([](const PlayerInput& input)
{
	InputAction* jump = input.FindAction("Gameplay/Jump");   // "Map/Action"
	InputAction* any = input.FindAction("Jump");             // first match across all maps
	InputActionMap* menu = input.FindActionMap("Menu");
	InputUser* player = input.GetUser();                    // nullptr until the player exists
});
```

Maps are built on the first input update after the component appears, and rebuilt whenever `actions` points to a different asset. Map and action pointers stay valid until then, even when other entities are added or removed, so they can be cached and callbacks can be registered on them. A rebuild drops the old maps and their callbacks.

Copying the component (for example into the play world) copies only `actions` and `playerIndex`; the copy builds its own maps.

### Several entities

Each `PlayerInput` builds its own copies of the asset's maps.

| Setup | Result |
| --- | --- |
| Two entities, same player, same asset (a character and its camera rig) | Both read identical values. Copies of the same map do not hide input from each other. |
| Same player, different assets or maps (a UI entity and a character) | Maps stack on that player; priority and consumption decide who sees a key. |
| Different players | Separate devices and separate binding overrides: local co-op. |

## Players and Users

An `InputUser` reads only from its **paired devices** and stores binding overrides. There are two kinds:

| Created by | Player index | In `GetPlayers()` | Join/leave events | Typical use |
| --- | --- | --- | --- | --- |
| `PlayerInput` components and joins | 0, 1, ... | yes | yes | Gameplay players |
| `CreateUser()` | `InputUser::InvalidPlayerIndex` | no | no | Editor, debug or UI input that is not a player |

`DestroyUser(user)` removes one; `DestroyPlayers()` removes all players and keeps users. Called from an action or rebind callback, both take effect at the end of the input update. The editor destroys all players when play mode ends, before the play world is torn down.

### Joining

Add a **Player Input Manager** component to any entity to choose the mode. Without one, the world behaves as `Manual`.

| Field | Meaning |
| --- | --- |
| `joinBehavior` | `Manual` or `JoinOnButtonPress` |
| `maxPlayers` | `JoinOnButtonPress` stops creating players at this count |

- **`Manual`**: a player is created when a `PlayerInput` references a `playerIndex` that does not exist yet. The **first** player is paired with every existing device and automatically pairs gamepads that connect later. Later players start with no devices and log a warning; pair them yourself or use `JoinOnButtonPress`.
- **`JoinOnButtonPress`**: a button press (or a stick pushed past 0.5) on an unpaired device creates a player with the lowest free index. A gamepad joins on its own; a keyboard or mouse press pairs both the keyboard and the mouse with the new player. Listen to `PlayerJoinedEvent` and create an entity whose `PlayerInput::playerIndex` is the new player's index.

### Pairing

```cpp
InputUser* player = input->GetPlayer(0);
player->UnpairAllDevices();                  // also stops auto-pairing new gamepads
player->PairDevice(input->GetGamepads()[0]);
player->IsPaired(device);
player->GetPairedDevices();
```

A device may be paired with several players; each reads it independently. `player->SetEnabled(false)` deactivates every map that reads from that player on the next update, canceling and resetting their actions. The editor uses this to mute players while the game viewport is unfocused.

### Maps outside a world

Code that is not part of a world (the editor) owns a `PlayerInput` directly, builds it from a descriptor and registers it with a user:

```cpp
mUser = input->CreateUser();
mUser->PairDevice(input->GetKeyboard());
mUser->PairDevice(input->GetMouse());

mInput.Build(descriptor);              // InputActionAssetDescriptor
input->AddPlayerInput(&mInput, mUser);

// on shutdown
input->RemovePlayerInput(&mInput);
input->DestroyUser(mUser);
```

Registered `PlayerInput`s update whether or not world input is enabled, and share consumption with the world's maps.

## Action Maps

Higher-priority maps see input first. A control they use is hidden from every lower-priority map for that frame. A map's priority is set in the asset.

```cpp
InputActionMap* menu = input.FindActionMap("Menu");
menu->SetEnabled(false);
menu->IsEnabled();
menu->GetPriority();
menu->FindAction("Back");
```

Every map in the asset starts enabled. A map is active only when it is enabled **and** its player exists and is enabled. Disabling a map cancels any started or performed action and zeroes its state. Re-enabling syncs the held state, so a key that is already down does not fire a fresh press.

### Priority and consumption

Each frame, every active map is sorted by priority, highest first, and updated in that order. When a binding actuates (non-zero value, or pressed this frame), the controls it used are **consumed** for that device. Lower-priority maps then read those controls as idle.

- Use this to layer input: a pause menu at priority 100 takes Escape before gameplay at 0 sees it.
- Consumption is shared across all players and users. A high-priority editor map bound to `W` blocks a game map bound to `W` on the same keyboard.
- Copies of the **same** map (same map GUID, on different entities) do not consume from each other, so they all see the same input.
- Maps with equal priority keep their order: registered `PlayerInput`s first, then world components.

### Modifier ordering

Within one map, bindings with **more modifiers are evaluated first**. `Ctrl+Shift+Z` therefore consumes `Z` before `Ctrl+Z` is checked, so Redo does not also trigger Undo. A binding whose modifiers are not all held is skipped and consumes nothing.

## Actions

An action has a value type (`Bool`, `Float`, `Float2`, `Float3`) and is read either by polling or through phase callbacks.

### Polling

```cpp
InputAction* jump = input.FindAction("Gameplay/Jump");
InputAction* move = input.FindAction("Gameplay/Move");

if (jump->WasPerformedThisFrame()) { ... }   // honors interactions (Hold, Tap, ...)
if (jump->WasPressedThisFrame()) { ... }     // raw press edge, ignores interactions
bool held = jump->IsPressed();
Float2 direction = move->ReadValue<Float2>();          // bool, float, Float2, Float3
const InputDevice* device = move->GetActiveDevice();   // last device that drove it
```

`ReadValue<bool>()` is `IsPressed()`. `ReadValue<float>()` returns `x`; `ReadValue<Float2>()` returns `x, y`.

### Callbacks

```cpp
InputCallbackHandle handle = jump->OnPerformed([](const InputActionContext& context)
{
	// context.action, context.phase, context.device, context.time (seconds)
});
jump->OnStarted([](const InputActionContext& context) { ... });
jump->OnCanceled([](const InputActionContext& context) { ... });
jump->RemoveCallback(handle);
```

Callbacks fire inside `InputSystem::Update()`, before the world updates. Adding or removing callbacks, creating or destroying entities (including ones with a `PlayerInput`) and destroying players are all safe from inside a callback. Maps of a `PlayerInput` destroyed mid-update are skipped for the rest of that update.

### Phases

`Disabled` → `Waiting` → `Started` → `Performed` → `Canceled`, then back to `Waiting` on the next frame. `GetPhase()` returns the current phase.

- **Value actions** (non-`Bool` with no interaction): Started + Performed when the value becomes non-zero, Performed again on every value change, Canceled when it returns to zero. Use these for sticks, mouse delta and WASD movement.
- **Button actions** (`Bool`, or any action with an interaction): driven by the active interaction below. A press means magnitude ≥ the interaction's `pressPoint` (default 0.5).

### Interactions

| Interaction | Started | Performed | Canceled | Settings |
| --- | --- | --- | --- | --- |
| `Press` (default) | on press | on press | on release | `pressPoint` |
| `Release` | on press | on release | when disabled while held | `pressPoint` |
| `Hold` | on press | once held for `duration` | on release | `duration` (s), `pressPoint` |
| `Tap` | on press | released within `duration` | held past `duration` | `duration` (s), `pressPoint` |
| `MultiTap` | on first press | after `tapCount` taps, each within `duration` | a tap or a gap exceeds `duration` | `tapCount`, `duration` (s), `pressPoint` |

Only the **first** interaction in a list is used. A binding's interaction overrides the action's, so one action can use `Press` on a gamepad and `Hold` on the keyboard.

## Bindings

A binding (`InputBindingDescriptor`) lists one or more controls. Each control is read from whichever paired device of that type gives the strongest value. An action can have any number of bindings: each frame the strongest one sets the value, and press/release edges from all of them are merged.

### Binding types

| Type | `controls`, in order | Value |
| --- | --- | --- |
| `Control` | 1 control | that control's raw value |
| `Axis1D` | Negative, Positive | x = Positive − Negative |
| `Axis2D` | Up, Down, Left, Right | x = Right − Left, y = Up − Down |
| `Axis3D` | Up, Down, Left, Right, Forward, Backward | adds z = Forward − Backward |

Composite parts use each control's magnitude, so a trigger or a stick axis can be a part too. `InputBindingDescriptor::GetPartName(type, part)` returns the part label for UIs.

### Modifiers

`modifiers` is an `EnumFlag<ModifierKeys>`. Every flag that is set must be held on a paired keyboard.

| Flag | Keys |
| --- | --- |
| `Shift` / `Control` / `Alt` / `Super` | left or right variant |
| `Shortcut` | Cmd on macOS, Ctrl elsewhere; use this for cross-platform shortcuts |

### Layout mapping

Keyboard controls are **physical scancodes** by default: `Key::W` is the key in the W position of a US layout, which suits movement. Set `layoutMapped = true` to bind by the **letter** instead (A–Z only), which suits mnemonic shortcuts such as `Ctrl+Z`. Layout-mapped bindings are re-resolved automatically when the OS layout changes.

### Processors

Processors run in list order: first those on the binding, then those on the action.

| Processor | Uses | Effect |
| --- | --- | --- |
| `DeadZone` | `min` (0.125), `max` (0.925) | Radial: magnitude below `min` → 0; `min..max` is rescaled to `0..1` |
| `Scale` | `factor` (Float3) | Per-component multiply |
| `Invert` | none | Negates the value |
| `Normalize` | none | Scales to unit length (zero stays zero) |
| `Clamp` | `min`, `max` | Per-component clamp |

Magnitude is `|x|` for `Bool`/`Float`, the length of `(x, y)` for `Float2`, and the full length for `Float3`.

## Defining Actions

Author game input as an **Input Actions** asset in the editor. Build descriptors in code only for engine or editor tooling that must exist without assets.

### As an asset

1. In the Content Browser, create an Input Actions asset from the context menu.
2. Edit it in the **Input Actions** panel: maps (name, priority) → actions → bindings, with value type, modifiers, processors and interactions per entry.
3. Assign it to the `actions` field of a **Player Input** component.

Maps and actions carry a `Guid`. Binding overrides are keyed by these GUIDs, so renaming a map or action in the asset keeps players' saved rebinds.

### In code

The descriptors are plain reflected structs: `InputActionAssetDescriptor` → `InputActionMapDescriptor` → `InputActionDescriptor` → `InputBindingDescriptor`. Build a `PlayerInput` from one with `PlayerInput::Build` and register it as shown in [Maps outside a world](#maps-outside-a-world).

```cpp
InputBindingDescriptor jumpKey;
jumpKey.controls.push_back(InputControl::Make(Key::Space));

InputBindingDescriptor jumpButton;
jumpButton.controls.push_back(InputControl::Make(GamepadControl::South));

InputActionDescriptor jump;
jump.id = Guid("5B0C6D1E-2F3A-4B5C-8D9E-0F1A2B3C4D5E");
jump.name = "Jump";
jump.valueType = InputValueType::Bool;
jump.bindings = { jumpKey, jumpButton };

InputBindingDescriptor wasd;
wasd.type = InputBindingType::Axis2D;
wasd.controls = { InputControl::Make(Key::W), InputControl::Make(Key::S),
                  InputControl::Make(Key::A), InputControl::Make(Key::D) };
wasd.processors.push_back(InputProcessor{ .type = InputProcessorType::Normalize });

InputBindingDescriptor stick;
stick.controls.push_back(InputControl::Make(GamepadControl::LeftStick));
stick.processors.push_back(InputProcessor{ .type = InputProcessorType::DeadZone });

InputActionDescriptor move;
move.id = Guid("6C1D7E2F-3A4B-4C5D-9E0F-1A2B3C4D5E6F");
move.name = "Move";
move.valueType = InputValueType::Float2;
move.bindings = { wasd, stick };

InputActionMapDescriptor gameplay;
gameplay.id = Guid("7D2E8F3A-4B5C-4D6E-8F1A-2B3C4D5E6F70");
gameplay.name = "Gameplay";
gameplay.priority = 0;
gameplay.actions = { jump, move };

InputActionAssetDescriptor actions;
actions.maps.push_back(gameplay);

mInput.Build(actions);
```

Use fixed GUIDs for maps that support rebinding. `Guid::NewGuid()` changes every run, so overrides saved against it will not load again.

## Rebinding

Rebinds are stored per player (`InputUser`) as overrides that replace one control of one binding; the authored descriptor is never modified. An override is addressed by `(map, action, binding index, part index)`. `part` is 0 for a `Control` binding, or the composite part index (for `Axis2D`: 0 Up, 1 Down, 2 Left, 3 Right).

Overrides apply to every map with a matching GUID that reads from that player, on every entity, starting with the next input update.

### Interactive rebind

```cpp
InputUser* player = input.GetUser();
player->StartInteractiveRebind(jump, 0, 0, [](bool completed)
{
	// completed == false: canceled by Escape, CancelInteractiveRebind or a new rebind
});
player->IsRebinding();
player->CancelInteractiveRebind();
```

- The first frame is skipped, so the click or key that opened the rebind UI is not captured.
- It then takes the first button pressed, or a stick pushed past 0.5, on the player's **paired** devices.
- **Escape cancels** the rebind, so Escape cannot be assigned this way; use `ApplyBindingOverride` for that.
- While the rebind runs, every map reading from that player is inactive, so the pressed key (or Escape) does not also trigger gameplay or menu actions.
- The rebind remembers the action by GUID, so it survives the map being rebuilt.

### Direct overrides and persistence

```cpp
player->ApplyBindingOverride(jump, 0, 0, InputControl::Make(Key::F));

TString json = player->SaveBindingOverrides();
player->LoadBindingOverrides(json);     // replaces all current overrides
player->ClearBindingOverrides();        // back to the authored bindings
```

Overrides are kept even when no map uses them yet; they apply automatically when a map with a matching GUID is built for that player. `InputAction::GetBindings()` returns the bindings with overrides applied, and `InputBindingDescriptor::GetDisplayName()` formats one for UI (for example `Ctrl+Z`).

## Cursor

`InputSystem::HideCursor()` hides the cursor, switches the window to relative mouse mode (unbounded `Mouse::Delta`) and remembers where the cursor was. `ShowCursor()` restores it to that position. `CursorVisible()` reports the current state.

## Examples

### Single-player controller

Put **Player Input** (with the game's Input Actions asset) and `CharacterMotor` on the player entity. No manager is needed: player 0 is created and paired with every device.

```cpp
class PlayerControllerSystem final : public ComponentSystem
{
protected:

	virtual void OnUpdate(EntityManager& entityManager) override
	{
		const float dt = static_cast<float>(Timestep::deltaTime);
		entityManager.ForEach<Entity, PlayerInput, CharacterMotor>([dt](Entity& entity, const PlayerInput& input, CharacterMotor& motor)
		{
			const auto move = input.FindAction("Gameplay/Move");
			const auto look = input.FindAction("Gameplay/Look");
			const auto jump = input.FindAction("Gameplay/Jump");
			if (move and look and jump)
			{
				const Float2 direction = move->ReadValue<Float2>();
				entity.Rotate(0.0f, look->ReadValue<Float2>().x * motor.lookSpeed * dt, 0.0f);   // radians
				entity.Translate((entity.ForwardVector() * direction.y + entity.RightVector() * direction.x) * motor.speed * dt);
				if (jump->WasPerformedThisFrame())
				{
					motor.Jump();
				}
			}
		});
	}
};
```

### Pause menu layered over gameplay

Give the asset a `Gameplay` map at priority 0 and a `Menu` map at priority 100. Every map starts enabled, so the system sets both from the menu state every frame; `SetEnabled` does nothing when the state is unchanged.

```cpp
entityManager.ForEach<PlayerInput, PauseMenu>([](const PlayerInput& input, PauseMenu& pause)
{
	const auto gameplay = input.FindActionMap("Gameplay");
	const auto menu = input.FindActionMap("Menu");
	if (gameplay and menu)
	{
		if (gameplay->FindAction("Pause")->WasPerformedThisFrame())
		{
			pause.open = true;
			Globals::Engine->GetSubsystem<InputSystem>()->ShowCursor();
		}
		else if (pause.open and menu->FindAction("Back")->WasPerformedThisFrame())
		{
			pause.open = false;
			Globals::Engine->GetSubsystem<InputSystem>()->HideCursor();
		}

		gameplay->SetEnabled(pause.open == false);
		menu->SetEnabled(pause.open);
	}
});
```

While the menu is open it sees Escape and the confirm buttons first, so they never reach gameplay.

### Local co-op

Add a **Player Input Manager** with `joinBehavior = JoinOnButtonPress` to the world, and create a player entity for each join:

```cpp
EventDispatcher<PlayerJoinedEvent>::Subscribe([](PlayerJoinedEvent e)
{
	auto world = Globals::GameInstance->GetSubsystem<WorldManager>()->GetActiveWorld();
	auto& entity = world->GetEntityManager().CreateEntity("Player", Guid::NewGuid());

	auto& input = entity.AddComponent<PlayerInput>();
	input.actions = kPlayerActions;
	input.playerIndex = e.GetPlayer()->GetPlayerIndex();

	entity.AddComponent<CharacterMotor>();
});

EventDispatcher<InputDeviceDisconnectedEvent>::Subscribe([](InputDeviceDisconnectedEvent e)
{
	ShowReconnectPrompt(e.GetDevice());
});
```

The new entity's maps are built on the next input update. Each player only reads its own gamepad (or the keyboard and mouse), so the same asset works for every player, and the controller system above needs no changes.

### Settings screen with rebinding

```cpp
void ControlsScreen::DrawBinding(const PlayerInput& input, InputAction* action, uint32_t binding, uint32_t part)
{
	InputUser* player = input.GetUser();
	const TString label = action->GetBindings()[binding].GetDisplayName();

	if (player->IsRebinding())
	{
		DrawText("Press a key... (Esc to cancel)");
	}
	else if (DrawButton(label))
	{
		player->StartInteractiveRebind(action, binding, part, [player](bool completed)
		{
			if (completed)
			{
				SaveSettings("bindings", player->SaveBindingOverrides());
			}
		});
	}
}

void ControlsScreen::OnOpen(const PlayerInput& input)
{
	input.GetUser()->LoadBindingOverrides(LoadSettings("bindings"));
}
```

### Reference implementation

`Engine/Source/Editor/src/EInput/EditorInputSystem.cpp` is the in-tree example of input outside a world. It creates one **user** (not a player) paired with the keyboard and mouse, builds a `PlayerInput` from four maps defined in code, and registers it with `AddPlayerInput`:

| Map | Priority | Contents | Enabled |
| --- | --- | --- | --- |
| `Editor.Global` | 100 | Undo/Redo/Play/Pause with `Shortcut` modifiers, Step | unless typing |
| `Editor.Panel` | 70 | Rename, Delete (platform-specific keys) | while a panel is focused |
| `Editor.Fly` | 60 | Move as an `Axis3D` binding on E/Q/A/D/W/S, Boost on either Shift | while flying |
| `Editor.Viewport` | 50 | gizmo keys, Fly/Pan/Orbit/Dolly mouse buttons with Alt variants, Look (`Float2` delta), Zoom (`ScrollY`) | in edit mode |

`WorldViewport.cpp` reads them with `ReadValue<>` and `WasPressedThisFrame()`.
