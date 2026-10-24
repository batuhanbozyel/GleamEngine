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
| `InputSystem` | Engine subsystem: owns devices and users, receives SDL events, evaluates action maps against paired devices |
| `InputDevice` | `Keyboard`, `Mouse`, `Gamepad`: raw per-frame control state |
| `InputUser` | A player or a non-player user: player index, paired devices, binding overrides, rebinding |
| `PlayerInput` | Entity component: an Input Actions asset and a player index. Plain data; holds no runtime state |
| `PlayerInputManager` | Required world singleton: join behavior and player limit |
| `PlayerInputSystem` | World `ComponentSystem`: builds action state for every `PlayerInput`, joins players, updates every map in priority order |
| `InputActionAsset` | Immutable, shared layout of an `InputActionAssetDescriptor`: maps, actions, flattened bindings, lookups by name or GUID |
| `InputActionStates` | Per-`PlayerInput` runtime state: map enable flags, action phases and values, bindings with overrides applied |
| `InputActionMapHandle` / `InputActionHandle` | Indices into an asset. The same handle is valid for every `InputActionStates` built from that asset |

The asset is shared and read-only, while each `PlayerInput` gets its own `InputActionStates`. Gameplay code looks up handles on the asset once and reads state through the handles.

## Frame Flow

1. `EventSystem` forwards SDL events to `InputSystem::ProcessEvent`, which writes each device's *pending* state. Keyboard, mouse and gamepad events are reported as handled.
2. `Application` calls `InputSystem::Update()` right after `EventSystem::Update()`:
   1. Pending interactive rebinds advance.
   2. Users destroyed from a rebind callback are released.
   3. Every device commits: pending state becomes current. Press and release counts are kept, so a press and release inside one frame still registers as a tap.
   4. The consumption set is cleared.
3. Tickable game instance subsystems run. The editor's `EditorInputSystem` evaluates its own maps here (see [Maps outside a world](#maps-outside-a-world)).
4. `World::Update()` runs fixed steps first, then `OnPreUpdate` for every system. In its `OnPreUpdate`, `PlayerInputSystem`:
   1. Builds `InputActionStates` for every `PlayerInput` that was added or whose `actions` changed, and drops state for removed components.
   2. If world input is enabled, joins players from the `PlayerInputManager` settings and publishes `PlayerJoinedEvent`.
   3. Prepares each `PlayerInput`: decides whether its player is active and re-resolves bindings if that player's overrides or the OS keyboard layout changed.
   4. Collects every active map from **all** `PlayerInput`s, sorts them by priority (highest first) and updates them in that order against one shared consumption set.

Read actions from `OnUpdate` or `OnPostUpdate`, where the values are settled for the frame. Fixed steps run before `OnPreUpdate`, so `OnFixedUpdate` sees the previous frame's action state.

In the editor, world input is off in edit mode and on during play (`InputSystem::SetWorldInputEnabled`). Edit-world `PlayerInput`s still build their state, but every map stays inactive and consumes nothing.

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

Polling a device ignores consumption, so a debug key read this way also reaches any action bound to it.

| Device | Buttons | Analog controls |
| --- | --- | --- |
| `Keyboard` | every `Key` (physical scancodes) | none |
| `Mouse` | `Left`, `Right`, `Middle`, `Back`, `Forward` | `Position`, `Delta`, `Scroll` (Float2); `DeltaX/Y`, `ScrollX/Y` (Float) |
| `Gamepad` | `South`/`East`/`West`/`North`, shoulders, stick presses, D-pad, `Start`/`Select`/`Guide`, triggers (pressed at 0.5) | `LeftStick`, `RightStick` (Float2); `LeftStickX/Y`, `RightStickX/Y`, `LeftTrigger`, `RightTrigger` (Float) |

Every device answers the same generic queries: `ReadControl(code)` returns a `Float3`, and `WasControlPressedThisFrame` / `WasControlReleasedThisFrame` report edges. Buttons read 0 or 1.

`InputControl` names one control as `{ device type, code }`. Build it with `InputControl::Make(Key::Space)`, `InputControl::Make(MouseControl::Delta)` or `InputControl::Make(GamepadControl::South)`. `GetName()` returns its display name.

`InputUtils::FindActuatedControl(devices, control)` returns the first button pressed this frame (or a stick pushed past 0.5) on the given devices. Rebinding, joining and the editor's binding Listen button are built on it.

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

The component is plain data. Its runtime state lives in `PlayerInputSystem`, keyed by entity:

```cpp
auto playerInput = world->GetSystem<PlayerInputSystem>();

const InputActionStates* states = playerInput->GetActionStates(entity);   // nullptr until built, or if the asset failed to load
if (states)
{
	const InputActionAsset& asset = *states->asset;
	InputActionHandle jump = asset.FindAction("Gameplay/Jump");   // "Map/Action"
	InputActionHandle any = asset.FindAction("Jump");             // first match across all maps
	InputActionMapHandle menu = asset.FindActionMap("Menu");

	const InputActionState& state = states->GetAction(jump);
	playerInput->SetActionMapEnabled(entity, menu, false);
}
```

Handles are indices into the asset. Look them up once and reuse them for every entity that uses the same asset. Always check `IsValid()` on handles found by name.

State is built in the first `OnPreUpdate` after the component appears, so it is readable from `OnUpdate` in the same frame. It is rebuilt whenever `actions` points to a different asset, which resets every map's enable flag and every action's phase. Changing only `playerIndex` keeps the state and switches it to the new player on the same update. Don't keep the `InputActionStates` pointer across frames: adding or removing any `PlayerInput` can move it. Call `GetActionStates` each update instead.

Saving the asset in the Input Actions panel reloads it in place on the next frame, including during play. Every `PlayerInput` using it rebuilds its state on that update, with the same effect as reassigning the asset: map enable flags and action phases reset, binding overrides are kept. Handles cached from the asset may point at different actions afterwards; code that caches them should compare `asset->GetVersion()` and look them up again.

### Several entities

Each `PlayerInput` gets its own `InputActionStates` for the asset's maps.

| Setup | Result |
| --- | --- |
| Two entities, same player, same asset (a character and its camera rig) | Both read identical values. Copies of the same map (same map GUID) do not hide input from each other. |
| Same player, different assets or maps (a UI entity and a character) | Maps stack on that player; priority and consumption decide who sees a key, regardless of which entity was created first. |
| Different players | Separate devices and separate binding overrides: local co-op. |

## Players and Users

An `InputUser` reads only from its **paired devices** and stores binding overrides. There are two kinds:

| Created by | Player index | In `GetPlayers()` | Join/leave events | Typical use |
| --- | --- | --- | --- | --- |
| `PlayerInputSystem` joins | 0, 1, ... | yes | yes | Gameplay players |
| `InputSystem::CreateUser()` | `InputUser::InvalidPlayerIndex` | no | no | Editor, debug or UI input that is not a player |

Get a player with `InputSystem::GetPlayer(index)`; it returns `nullptr` until that player has joined. `DestroyUser(user)` removes one, and `DestroyPlayers()` removes all players and keeps users. Called from a rebind callback, both take effect at the end of `InputSystem::Update()`. The editor destroys all players when play mode ends, before the play world is torn down.

### Joining

Every world has a **Player Input Manager** singleton. The launcher adds it to new worlds, and `PlayerInputSystem` requires it.

| Field | Meaning |
| --- | --- |
| `joinBehavior` | `Manual` or `JoinOnButtonPress` |
| `maxPlayers` | `JoinOnButtonPress` stops creating players at this count |

- **`Manual`**: a player is created when a `PlayerInput` references a `playerIndex` that does not exist yet. The **first** player is paired with every existing device and automatically pairs gamepads that connect later. Later players start with no devices and log a warning; pair them yourself or use `JoinOnButtonPress`.
- **`JoinOnButtonPress`**: a button press (or a stick pushed past 0.5) on a device not paired with any player creates a player with the lowest free index. A gamepad joins on its own; a keyboard or mouse press pairs both the keyboard and the mouse with the new player. Listen to `PlayerJoinedEvent` and create an entity whose `PlayerInput::playerIndex` is the new player's index.

Joining only happens while world input is enabled.

### Pairing

```cpp
InputUser* player = input->GetPlayer(0);
player->UnpairAllDevices();                  // also stops auto-pairing new gamepads
player->PairDevice(input->GetGamepads()[0]);
player->IsPaired(device);
player->GetPairedDevices();
```

A device may be paired with several players; each reads it independently. `player->SetEnabled(false)` deactivates every map that reads from that player on the next update, which resets their actions. The editor uses this to mute players while the game viewport is unfocused.

### Maps outside a world

Code that is not part of a world (the editor) owns the asset and state itself, creates a user, and updates the state every frame from a tickable game instance subsystem:

```cpp
// Initialize
mUser = input->CreateUser();
mUser->PairDevice(input->GetKeyboard());
mUser->PairDevice(input->GetMouse());

mActions = CreateScope<InputActionAsset>(descriptor);   // InputActionAssetDescriptor
mStates = InputActionStates(mActions.get());

// Tick
input->UpdateActions(mStates, mUser);

// Shutdown
input->DestroyUser(mUser);
mStates = InputActionStates();
mActions.reset();
```

`UpdateActions` updates that one state's maps in priority order. It shares the frame's consumption set with the world's maps, and because game instance subsystems tick before the world, these maps see input before any `PlayerInput`, whatever their priority. They update whether or not world input is enabled.

To merge several states into one priority order yourself, do what `PlayerInputSystem` does: call `InputSystem::PrepareActions(states, user)` for each (it returns whether the user is active), collect the active maps, sort them by priority, and call `UpdateActionMap(states, map, *user, input->GetTime())` for each.

## Action Maps

Higher-priority maps see input first. A control they use is hidden from every lower-priority map with a different GUID for the rest of that frame. A map's priority is set in the asset.

```cpp
const InputActionAsset& asset = *states->asset;
InputActionMapHandle menu = asset.FindActionMap("Menu");

playerInput->SetActionMapEnabled(entity, menu, false);
states->maps[menu.value].enabled;            // requested state
states->maps[menu.value].active;             // enabled and the player is active
asset.GetActionMap(menu).priority;
```

Every map starts enabled. A map is active only when it is enabled **and** its player exists, is enabled and is not rebinding. Disabling a map moves its actions to `Disabled` and zeroes their state. Re-enabling syncs the held state, so a key that is already down does not fire a fresh press.

### Priority and consumption

Each frame, every active map from every `PlayerInput` is sorted by priority, highest first, and updated in that order. When a binding actuates (non-zero value, or pressed this frame), the controls it used are **consumed** on that device for that map. Maps with a different GUID then read those controls as idle.

- Use this to layer input: a pause menu at priority 100 takes Escape before gameplay at 0 sees it, even if the menu entity was spawned after the player.
- Consumption is shared across all players and users. An editor map bound to `W` blocks a game map bound to `W` on the same keyboard.
- Copies of the **same** map (same map GUID, on different entities) do not consume from each other, so they all see the same input.
- Maps with equal priority keep a stable order: maps of the same asset in asset order, then `PlayerInput`s in the order their state was built.

### Modifier ordering

Within one map, bindings with **more modifiers are evaluated first**, and a binding that actuates hides its controls from the remaining bindings of that map. `Ctrl+Shift+Z` therefore takes `Z` before `Ctrl+Z` is checked, so Redo does not also trigger Undo. A binding whose modifiers are not all held is skipped and hides nothing.

## Actions

An action has a value type (`Bool`, `Float`, `Float2`, `Float3`) and is read by polling its `InputActionState`. There are no callbacks: systems check the frame flags in `OnUpdate`.

```cpp
const InputActionState& jump = states->GetAction(jumpHandle);
const InputActionState& move = states->GetAction(moveHandle);

if (jump.performedThisFrame) { ... }   // honors interactions (Hold, Tap, ...)
if (jump.pressedThisFrame) { ... }     // raw press edge, ignores interactions
if (jump.releasedThisFrame) { ... }
bool held = jump.pressed;
Float2 direction = move.ReadValue<Float2>();      // bool, float, Float2, Float3
const InputDevice* device = move.activeDevice;    // last device that drove it
InputActionPhase phase = jump.phase;
```

`ReadValue<bool>()` is `pressed`. `ReadValue<float>()` returns `x`; `ReadValue<Float2>()` returns `x, y`. `value` and `magnitude` are the processed value and its magnitude.

### Phases

`Disabled` → `Waiting` → `Started` → `Performed` → `Canceled`, then back to `Waiting` on the next frame.

- **Value actions** (non-`Bool` with no interaction): Started + Performed when the value becomes non-zero, Performed again on every value change, Canceled when it returns to zero. Use these for sticks, mouse delta and WASD movement.
- **Button actions** (`Bool`, or any action with an interaction): driven by the active interaction below. A press means magnitude ≥ the interaction's `pressPoint` (default 0.5).

`Release`, `Tap` and `MultiTap` return to `Waiting` in the same frame they perform, so check `performedThisFrame` rather than `phase == Performed`.

### Interactions

| Interaction | Started | Performed | Canceled | Settings |
| --- | --- | --- | --- | --- |
| `Press` (default) | on press | on press | on release | `pressPoint` |
| `Release` | on press | on release | never; disabling resets it | `pressPoint` |
| `Hold` | on press | once held for `duration` | on release | `duration` (s), `pressPoint` |
| `Tap` | on press | released within `duration` | held past `duration` | `duration` (s), `pressPoint` |
| `MultiTap` | on first press | after `tapCount` taps, each within `duration` | a tap or a gap exceeds `duration` | `tapCount`, `duration` (s), `pressPoint` |

Only the **first** interaction in a list is used. A binding's interaction overrides the action's, so one action can use `Press` on a gamepad and `Hold` on the keyboard. Durations are measured in real time (`InputSystem::GetTime()`), independent of `Timestep::timeScale`.

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

`modifiers` is an `EnumFlag<ModifierKeys>`. Every flag that is set must be held on a paired keyboard. A binding without modifiers does not require them to be released, so `S` still fires while Ctrl is held unless a `Ctrl+S` binding in the same or a higher map takes it first.

| Flag | Keys |
| --- | --- |
| `Shift` / `Control` / `Alt` / `Super` | left or right variant |
| `Shortcut` | Cmd on macOS, Ctrl elsewhere; use this for cross-platform shortcuts |

### Layout mapping

Keyboard controls are **physical scancodes** by default: `Key::W` is the key in the W position of a US layout, which suits movement. Set `layoutMapped = true` to bind by the **letter** instead (A–Z only), which suits mnemonic shortcuts such as `Ctrl+Z`. Layout-mapped bindings are re-resolved automatically when the OS layout changes. `InputBindingDescriptor::MakeKeyBinding` sets it.

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
2. Edit it in the **Input Actions** panel: maps (name, priority) → actions → bindings, with value type, modifiers, processors and interactions per entry. **Listen** captures the next button press on any device; Escape cancels.
3. Assign it to the `actions` field of a **Player Input** component.

Maps and actions carry a `Guid`. Binding overrides and consumption are keyed by these GUIDs, so renaming a map or action in the asset keeps players' saved rebinds.

Saving reloads the loaded asset in place on the next frame (see [PlayerInput Component](#playerinput-component)).

### In code

The descriptors are plain reflected structs: `InputActionAssetDescriptor` → `InputActionMapDescriptor` → `InputActionDescriptor` → `InputBindingDescriptor`. Construct an `InputActionAsset` from one and drive it as shown in [Maps outside a world](#maps-outside-a-world).

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

mActions = CreateScope<InputActionAsset>(actions);
mStates = InputActionStates(mActions.get());
```

The `Make` helpers (`InputActionAssetDescriptor::Make`, `InputActionMapDescriptor::Make`, `InputActionDescriptor::Make`, `InputBindingDescriptor::MakeKeyBinding` / `MakeMouseBinding` / `MakeAxisBinding`) shorten this, but they assign `Guid::NewGuid()`. That changes every run, so overrides saved against those maps will not load again. Use fixed GUIDs for maps that support rebinding.

## Rebinding

Rebinds are stored per player (`InputUser`) as overrides that replace one control of one binding; the asset is never modified. An override is addressed by `(map, action, binding index, part index)`. `part` is 0 for a `Control` binding, or the composite part index (for `Axis2D`: 0 Up, 1 Down, 2 Left, 3 Right).

Overrides apply to every `PlayerInput` that reads from that player and whose asset has a map and action with matching GUIDs, starting with the next update.

### Interactive rebind

```cpp
InputUser* player = input->GetPlayer(playerInput.playerIndex);
player->StartInteractiveRebind(*states->asset, jumpHandle, 0, 0, [](bool completed)
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
- The rebind remembers the action by GUID, so it survives the state being rebuilt.

### Direct overrides and persistence

```cpp
player->ApplyBindingOverride(*states->asset, jumpHandle, 0, 0, InputControl::Make(Key::F));

TString json = player->SaveBindingOverrides();
player->LoadBindingOverrides(json);     // replaces all current overrides
player->ClearBindingOverrides();        // back to the authored bindings
```

Overrides are kept even when no map uses them yet; they apply automatically when state is built for a matching asset on that player. `InputActionStates::bindings` holds every binding with overrides applied, indexed through the action's layout. `GetDisplayName()` formats one for UI (for example `Ctrl+Z`):

```cpp
const InputActionLayout& layout = states->asset->GetActionLayout(jumpHandle);
for (uint32_t i = 0; i < layout.bindingCount; ++i)
{
	TString label = states->bindings[layout.firstBinding + i].GetDisplayName();
}
```

## Cursor

`InputSystem::HideCursor()` hides the cursor, switches the window to relative mouse mode (unbounded `Mouse::Delta`) and remembers where the cursor was. `ShowCursor()` restores it to that position. `CursorVisible()` reports the current state.

## Examples

### Single-player controller

Put **Player Input** (with the game's Input Actions asset) and `CharacterMotor` on the player entity. With `joinBehavior = Manual`, player 0 is created and paired with every device.

```cpp
class PlayerControllerSystem final : public ComponentSystem
{
public:

	PlayerControllerSystem(PlayerInputSystem* playerInput)
		: mPlayerInput(playerInput)
	{

	}

protected:

	virtual void OnUpdate(EntityManager& entityManager) override
	{
		const float dt = static_cast<float>(Timestep::deltaTime);
		entityManager.ForEach<Entity, PlayerInput, CharacterMotor>([this, dt](EntityHandle handle, Entity& entity, const PlayerInput& input, CharacterMotor& motor)
		{
			const auto states = mPlayerInput->GetActionStates(handle);
			if (states == nullptr)
			{
				return;
			}

			const auto& asset = *states->asset;
			const auto move = asset.FindAction("Gameplay/Move");
			const auto look = asset.FindAction("Gameplay/Look");
			const auto jump = asset.FindAction("Gameplay/Jump");
			if (move.IsValid() and look.IsValid() and jump.IsValid())
			{
				const Float2 direction = states->GetAction(move).ReadValue<Float2>();
				entity.Rotate(0.0f, states->GetAction(look).ReadValue<Float2>().x * motor.lookSpeed * dt, 0.0f);   // radians
				entity.Translate((entity.ForwardVector() * direction.y + entity.RightVector() * direction.x) * motor.speed * dt);
				if (states->GetAction(jump).performedThisFrame)
				{
					motor.Jump();
				}
			}
		});
	}

private:

	PlayerInputSystem* mPlayerInput;

};

world->AddSystem<PlayerControllerSystem>(world->GetSystem<PlayerInputSystem>());
```

`FindAction` by name is a linear search. A real system would cache the handles per asset.

### Pause menu layered over gameplay

Give the asset a `Gameplay` map at priority 0 and a `Menu` map at priority 100. Every map starts enabled, so the system sets both from the menu state every frame; `SetActionMapEnabled` does nothing when the state is unchanged.

```cpp
entityManager.ForEach<PlayerInput, PauseMenu>([this](EntityHandle handle, const PlayerInput& input, PauseMenu& pause)
{
	const auto states = mPlayerInput->GetActionStates(handle);
	if (states == nullptr)
	{
		return;
	}

	const auto& asset = *states->asset;
	const auto gameplay = asset.FindActionMap("Gameplay");
	const auto menu = asset.FindActionMap("Menu");
	if (gameplay.IsValid() and menu.IsValid())
	{
		auto input = Globals::Engine->GetSubsystem<InputSystem>();
		if (states->GetAction(asset.FindAction("Gameplay/Pause")).performedThisFrame)
		{
			pause.open = true;
			input->ShowCursor();
		}
		else if (pause.open and states->GetAction(asset.FindAction("Menu/Back")).performedThisFrame)
		{
			pause.open = false;
			input->HideCursor();
		}

		mPlayerInput->SetActionMapEnabled(handle, gameplay, pause.open == false);
		mPlayerInput->SetActionMapEnabled(handle, menu, pause.open);
	}
});
```

While the menu is open it sees Escape and the confirm buttons first, so they never reach gameplay. The menu could also live on a separate UI entity with its own asset; priority still decides who sees the key first.

### Local co-op

Set the world's **Player Input Manager** to `joinBehavior = JoinOnButtonPress`, and create a player entity for each join:

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

The event fires inside `PlayerInputSystem::OnPreUpdate`; the new entity's state is built on the next update. Each player only reads its own gamepad (or the keyboard and mouse), so the same asset works for every player, and the controller system above needs no changes.

### Settings screen with rebinding

```cpp
void ControlsScreen::DrawBinding(const InputActionStates& states, InputUser* player, InputActionHandle action, uint32_t binding, uint32_t part)
{
	const auto& layout = states.asset->GetActionLayout(action);
	const TString label = states.bindings[layout.firstBinding + binding].GetDisplayName();

	if (player->IsRebinding())
	{
		DrawText("Press a key... (Esc to cancel)");
	}
	else if (DrawButton(label))
	{
		player->StartInteractiveRebind(*states.asset, action, binding, part, [player](bool completed)
		{
			if (completed)
			{
				SaveSettings("bindings", player->SaveBindingOverrides());
			}
		});
	}
}

void ControlsScreen::OnOpen(InputUser* player)
{
	player->LoadBindingOverrides(LoadSettings("bindings"));
}
```

### Reference implementation

`Engine/Source/Editor/src/EInput/EditorInputSystem.cpp` is the in-tree example of input outside a world. It creates one **user** (not a player) paired with the keyboard and mouse, builds an `InputActionAsset` from four maps defined in code, and calls `UpdateActions` from its `Tick`:

| Map | Priority | Contents | Enabled |
| --- | --- | --- | --- |
| `Editor.Global` | 100 | Undo/Redo/Play/Pause with `Shortcut` modifiers, Step | unless typing |
| `Editor.Panel` | 70 | Rename, Delete (platform-specific keys) | while a panel requests it |
| `Editor.Fly` | 60 | Move as an `Axis3D` binding on E/Q/A/D/W/S, Boost on either Shift | while flying |
| `Editor.Viewport` | 50 | gizmo keys, Fly/Pan/Orbit/Dolly mouse buttons with Alt variants, Look (`Float2` delta), Zoom (`ScrollY`) | in edit mode, unless typing |

`WorldViewport.cpp` reads them through `EditorInputSystem::GetAction(handle)`.
