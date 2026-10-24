# Input System

Gameplay reads input through **actions**, not raw keys. Actions are grouped into **action maps** and authored in an **Input Actions** asset. A `PlayerInput` component points an entity at an asset and at a **player**, and the player decides which devices that entity reads from. Code lives in `Engine/Source/Runtime/src/Input/`.

## Table of Contents
* [Overview](#overview)
* [Frame Flow](#frame-flow)
* [Reading Actions](#reading-actions)
* [Players](#players)
* [Action Maps](#action-maps)
* [Actions](#actions)
* [Bindings](#bindings)
* [Rebinding](#rebinding)
* [Devices](#devices)
* [Cursor](#cursor)
* [Examples](#examples)

## Overview

| Type | Role |
| --- | --- |
| `PlayerInput` | Entity component: an Input Actions asset and a player index. Plain data |
| `PlayerInputManager` | Required world singleton: join behavior and player limit |
| `PlayerInputSystem` | World system that builds and updates action state for every `PlayerInput`, and joins players |
| `InputActionAsset` | The loaded, read-only asset: maps, actions, bindings, lookups by name |
| `InputActionStates` | One `PlayerInput`'s runtime state: map enable flags, action values and phases, bindings with the player's overrides applied |
| `InputActionMapHandle` / `InputActionHandle` | Indices into an asset, valid for every `InputActionStates` built from it |
| `InputUser` | A player: paired devices, binding overrides, interactive rebinding |
| `InputSystem` | Engine subsystem: devices, players, cursor |

The asset is shared; each `PlayerInput` gets its own state. Look handles up on the asset and read state through them.

## Frame Flow

1. At the start of the frame, every device commits the input received since the last frame. A press and release inside one frame still counts as a tap.
2. During `World::Update`, `PlayerInputSystem::OnPreUpdate`:
   1. Builds state for every `PlayerInput` that was added or whose `actions` changed.
   2. Joins players according to the `PlayerInputManager` and publishes `PlayerJoinedEvent`.
   3. Updates every active map of every `PlayerInput`, highest priority first (see [Priority and consumption](#priority-and-consumption)).
3. Your systems read action state in `OnUpdate` or `OnPostUpdate`.

Fixed steps run before `OnPreUpdate`, so `OnFixedUpdate` sees the previous frame's action state. Read input in `OnUpdate` and hand it to fixed-step code if it needs it.

## Reading Actions

Add **Player Input** to an entity and assign an Input Actions asset:

| Field | Meaning |
| --- | --- |
| `actions` | The Input Actions asset. Every map in it is built for this entity. |
| `playerIndex` | Which player's devices and binding overrides it reads. |

State lives in `PlayerInputSystem`, keyed by entity:

```cpp
auto playerInput = world->GetSystem<PlayerInputSystem>();

const InputActionStates* states = playerInput->GetActionStates(entity);   // nullptr until built, or if the asset failed to load
if (states)
{
	const InputActionAsset& asset = *states->asset;
	InputActionHandle jump = asset.FindAction("Gameplay/Jump");   // "Map/Action"
	InputActionHandle any = asset.FindAction("Jump");             // first match across all maps
	InputActionMapHandle menu = asset.FindActionMap("Menu");

	if (jump.IsValid() and states->GetAction(jump).performedThisFrame)
	{
		...
	}
	playerInput->SetActionMapEnabled(entity, menu, false);
}
```

- State is built in the first `OnPreUpdate` after the component appears, so it is readable from `OnUpdate` in the same frame.
- Call `GetActionStates` every update. Adding or removing any `PlayerInput` can move the state, so don't keep the pointer across frames.
- Handles found by name may be invalid; check `IsValid()`. `FindAction` is a linear search, so cache handles per asset.
- Assigning a different asset rebuilds the state: map enable flags and action phases reset. Changing only `playerIndex` keeps the state and switches players on the same update.
- An asset can be reloaded in place while the game runs. Every `PlayerInput` using it rebuilds its state as if the asset had been reassigned, and cached handles may then point at different actions. If you cache handles, store `asset->GetVersion()` with them and look them up again when it changes.

### Several entities

| Setup | Result |
| --- | --- |
| Same player, same asset (a character and its camera rig) | Both read identical values. Copies of the same map don't hide input from each other. |
| Same player, different assets (a UI entity and a character) | Maps stack on that player; priority decides who sees a key, regardless of which entity was created first. |
| Different players | Separate devices and separate rebinds: local co-op. |

## Players

A player (`InputUser`) reads only from its **paired devices** and stores its binding overrides. `InputSystem::GetPlayer(index)` returns it, or `nullptr` until that player has joined; `GetPlayers()` lists them.

### Joining

Every world has a **Player Input Manager** singleton:

| Field | Meaning |
| --- | --- |
| `joinBehavior` | `Manual` or `JoinOnButtonPress` |
| `maxPlayers` | `JoinOnButtonPress` stops creating players at this count |

- **`Manual`**: a player is created when a `PlayerInput` references a `playerIndex` that doesn't exist yet. The **first** player is paired with every device and also gets gamepads that connect later. Later players start with no devices and log a warning; pair them yourself or use `JoinOnButtonPress`.
- **`JoinOnButtonPress`**: pressing a button (or pushing a stick past 0.5) on a device no player owns creates a player with the lowest free index. A gamepad joins on its own; a keyboard or mouse press pairs both with the new player. Handle `PlayerJoinedEvent` and create an entity whose `playerIndex` is the new player's index.

`InputSystem::DestroyUser(player)` removes one player and `DestroyPlayers()` removes all of them; both publish `PlayerLeftEvent`.

### Pairing

```cpp
auto input = Globals::Engine->GetSubsystem<InputSystem>();

InputUser* player = input->GetPlayer(0);
player->UnpairAllDevices();                  // also stops auto-pairing new gamepads
player->PairDevice(input->GetGamepads()[0]);
player->IsPaired(device);
player->GetPairedDevices();
player->SetEnabled(false);                   // every map reading from this player goes inactive
```

A device may be paired with several players; each reads it independently.

## Action Maps

```cpp
playerInput->SetActionMapEnabled(entity, menu, false);
states->maps[menu.value].enabled;            // requested state
states->maps[menu.value].active;             // enabled, and the player exists, is enabled and isn't rebinding
asset.GetActionMap(menu).priority;
```

Every map starts enabled. Disabling a map moves its actions to `Disabled` and zeroes them. Re-enabling it doesn't fire a fresh press for a key that is already held.

### Priority and consumption

Each frame, the active maps of **all** `PlayerInput`s are sorted by priority, highest first, and updated in that order. When a binding actuates (non-zero value, or pressed this frame), the controls that actuated are **consumed** for the rest of the frame: lower-priority maps read them as idle.

- Use this to layer input. A pause menu at priority 100 takes Escape before gameplay at 0 sees it, even if the menu entity was created after the player.
- Copies of the **same** map on different entities don't consume from each other, so they all see the same input.
- Maps with equal priority keep a stable order: maps of one asset in asset order, then entities in the order their state was built.

### Modifier ordering

Within one map, bindings with **more modifiers are evaluated first**, and controls they consume are hidden from the rest of that map. `Ctrl+Shift+Z` therefore takes `Z` before `Ctrl+Z` is checked, so Redo doesn't also trigger Undo. A binding whose modifiers aren't all held is skipped and consumes nothing.

## Actions

An action has a value type (`Bool`, `Float`, `Float2`, `Float3`) and is polled through its `InputActionState`. There are no callbacks.

```cpp
const InputActionState& jump = states->GetAction(jumpHandle);
const InputActionState& move = states->GetAction(moveHandle);

if (jump.performedThisFrame) { ... }   // honors interactions (Hold, Tap, ...)
if (jump.pressedThisFrame) { ... }     // raw press edge, ignores interactions
if (jump.releasedThisFrame) { ... }
bool held = jump.pressed;
Float2 direction = move.ReadValue<Float2>();      // bool, float, Float2, Float3
const InputDevice* device = move.activeDevice;    // last device that drove it, e.g. for button prompts
InputActionPhase phase = jump.phase;
```

`ReadValue<bool>()` is `pressed`, `ReadValue<float>()` is `x`, `ReadValue<Float2>()` is `x, y`. `value` and `magnitude` hold the processed value and its magnitude.

### Phases

`Disabled` → `Waiting` → `Started` → `Performed` → `Canceled`, then back to `Waiting` on the next frame.

- **Value actions** (non-`Bool`, no interaction): Started and Performed when the value becomes non-zero, Performed again on every change, Canceled when it returns to zero. Use these for sticks, mouse delta and movement.
- **Button actions** (`Bool`, or any action with an interaction): driven by the interaction below. A press means magnitude ≥ the interaction's `pressPoint` (default 0.5).

`Release`, `Tap` and `MultiTap` return to `Waiting` in the frame they perform, so check `performedThisFrame` rather than `phase == Performed`.

### Interactions

| Interaction | Started | Performed | Canceled | Settings |
| --- | --- | --- | --- | --- |
| `Press` (default) | on press | on press | on release | `pressPoint` |
| `Release` | on press | on release | never; disabling resets it | `pressPoint` |
| `Hold` | on press | once held for `duration` | on release | `duration` (s), `pressPoint` |
| `Tap` | on press | released within `duration` | held past `duration` | `duration` (s), `pressPoint` |
| `MultiTap` | on first press | after `tapCount` taps, each within `duration` | a tap or a gap exceeds `duration` | `tapCount`, `duration` (s), `pressPoint` |

Only the **first** interaction in a list is used. A binding's interaction overrides the action's, so one action can use `Press` on a gamepad and `Hold` on the keyboard.

Durations are real time, unaffected by `Timestep::timeScale`, so a hold feels the same in slow motion or while paused. Press and release times come from the OS event timestamps, so a frame hitch doesn't turn a quick tap into a long one.

## Bindings

A binding lists one or more controls. Each control is read from whichever paired device of that type gives the strongest value. An action can have any number of bindings: the strongest one sets the value each frame, and press/release edges from all of them count.

### Binding types

| Type | Controls, in order | Value |
| --- | --- | --- |
| `Control` | 1 control | that control's value |
| `Axis1D` | Negative, Positive | x = Positive − Negative |
| `Axis2D` | Up, Down, Left, Right | x = Right − Left, y = Up − Down |
| `Axis3D` | Up, Down, Left, Right, Forward, Backward | adds z = Forward − Backward |

Composite parts use each control's magnitude, so a trigger or a stick axis can be a part.

### Modifiers

Every modifier set on a binding must be held. A binding without modifiers doesn't require them to be released, so `S` still fires while Ctrl is held unless a `Ctrl+S` binding takes it first.

| Modifier | Keys |
| --- | --- |
| `Shift` / `Control` / `Alt` / `Super` | left or right variant |
| `Shortcut` | Cmd on macOS, Ctrl elsewhere |

### Keyboard layouts

Keyboard bindings are physical keys: `W` is the key in the W position of a US layout, so WASD keeps its shape on AZERTY. Display names (`InputControl::GetName`, `GetDisplayName`) show how that key is labelled on the player's current layout, so the same bindings read "Z / Q / S / D" on AZERTY. Handle `KeymapChangedEvent` to refresh prompts when the player switches layouts.

### Processors

Processors run in order: those on the binding, then those on the action.

| Processor | Settings | Effect |
| --- | --- | --- |
| `DeadZone` | `min` (0.125), `max` (0.925) | Radial: magnitude below `min` → 0, `min..max` rescaled to `0..1` |
| `Scale` | `factor` (Float3) | Per-component multiply |
| `Invert` | none | Negates the value |
| `Normalize` | none | Scales to unit length (zero stays zero) |
| `Clamp` | `min`, `max` | Per-component clamp |

Magnitude is `|x|` for `Bool`/`Float`, the length of `(x, y)` for `Float2`, and the full length for `Float3`.

## Rebinding

Rebinds are stored per player as overrides; the asset is never modified. An override replaces one control of one binding: pass the action, the binding's index on that action and the part index. `part` is 0 for a `Control` binding, or the composite part (for `Axis2D`: 0 Up, 1 Down, 2 Left, 3 Right). An override always names a physical key. Overrides are stored by binding GUID, so reordering or inserting bindings in the asset keeps saved rebinds.

Overrides apply to every `PlayerInput` of that player whose asset contains the action, from the next update. Maps and actions are matched by GUID, so renaming them in the asset keeps players' saved rebinds.

### Interactive rebind

```cpp
InputUser* player = input->GetPlayer(playerIndex);
player->StartInteractiveRebind(*states->asset, jumpHandle, 0, 0, [](bool completed)
{
	// completed == false: canceled by Escape, CancelInteractiveRebind or a new rebind
});
player->IsRebinding();
player->CancelInteractiveRebind();
```

- The first frame is skipped, so the click or key that opened the rebind UI isn't captured.
- It then takes the first button pressed, or a stick pushed past 0.5, on the player's paired devices.
- **Escape cancels**, so Escape can't be assigned this way; use `ApplyBindingOverride` for that.
- While the rebind runs, every map of that player is inactive, so the key doesn't also trigger gameplay or menu actions.

### Direct overrides and persistence

```cpp
player->ApplyBindingOverride(*states->asset, jumpHandle, 0, 0, InputControl::Make(Key::F));

TString json = player->SaveBindingOverrides();
player->LoadBindingOverrides(json);     // replaces all current overrides
player->ClearBindingOverrides();        // back to the authored bindings
```

Overrides are kept even when no map uses them yet. `InputActionStates::bindings` holds every binding with overrides applied; `GetDisplayName()` formats one for UI (for example `Ctrl+Z`):

```cpp
const InputActionLayout& layout = states->asset->GetActionLayout(jumpHandle);
for (uint32_t i = 0; i < layout.bindingCount; ++i)
{
	TString label = states->bindings[layout.firstBinding + i].GetDisplayName();
}
```

## Devices

Actions cover gameplay. Poll devices directly only for debug keys, and remember that polling ignores consumption:

```cpp
auto input = Globals::Engine->GetSubsystem<InputSystem>();

if (input->GetKeyboard()->WasPressedThisFrame(Key::F5))
{
	ReloadLevel();
}

Float2 delta = input->GetMouse()->GetDelta();
for (auto gamepad : input->GetGamepads())
{
	gamepad->SetRumble(0.5f, 0.5f, 200);
}
```

| Device | Buttons | Analog controls |
| --- | --- | --- |
| `Keyboard` | every `Key` (physical) | none |
| `Mouse` | `Left`, `Right`, `Middle`, `Back`, `Forward` | `Position`, `Delta`, `Scroll` (Float2); `DeltaX/Y`, `ScrollX/Y` (Float) |
| `Gamepad` | `South`/`East`/`West`/`North`, shoulders, stick presses, D-pad, `Start`/`Select`/`Guide`, triggers (pressed at 0.5) | `LeftStick`, `RightStick` (Float2); `LeftStickX/Y`, `RightStickX/Y`, `LeftTrigger`, `RightTrigger` (Float) |

A gamepad that reconnects gets its old `InputDevice` back, so pairings survive a cable pull. `InputControl::Make(Key::Space)` / `Make(MouseControl::Delta)` / `Make(GamepadControl::South)` names a control; `GetName()` returns its display name.

### Events

```cpp
EventDispatcher<InputDeviceDisconnectedEvent>::Subscribe([](InputDeviceDisconnectedEvent e)
{
	ShowReconnectPrompt(e.GetDevice());
});
```

| Event | Payload |
| --- | --- |
| `InputDeviceConnectedEvent` / `InputDeviceDisconnectedEvent` | `GetDevice()` |
| `PlayerJoinedEvent` / `PlayerLeftEvent` | `GetPlayer()` |
| `KeymapChangedEvent` | none; the OS keyboard layout changed |

## Cursor

`InputSystem::HideCursor()` hides the cursor and switches to relative mouse mode (unbounded `Mouse::Delta`), remembering where the cursor was. `ShowCursor()` restores it there. `CursorVisible()` reports the current state.

## Examples

### Single-player controller

The player entity has **Player Input** and a game-defined `CharacterMotor` component. With `joinBehavior = Manual`, player 0 is created and paired with every device.

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
				entity.Rotate(0.0f, states->GetAction(look).ReadValue<Float2>().x * motor.lookSpeed * dt, 0.0f);
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

### Pause menu layered over gameplay

The asset has a `Gameplay` map at priority 0 and a `Menu` map at priority 100. `SetActionMapEnabled` does nothing when the state is unchanged, so the system can set both every frame.

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

While the menu is open it sees Escape and the confirm buttons first, so they never reach gameplay. The menu could also live on its own UI entity with a separate asset; priority still decides who sees the key first.

### Local co-op

Set the world's **Player Input Manager** to `JoinOnButtonPress` and create a player entity for each join:

```cpp
EventDispatcher<PlayerJoinedEvent>::Subscribe([this](PlayerJoinedEvent e)
{
	auto world = Globals::GameInstance->GetSubsystem<WorldManager>()->GetActiveWorld();
	auto& entity = world->GetEntityManager().CreateEntity("Player", Guid::NewGuid());

	auto& input = entity.AddComponent<PlayerInput>();
	input.actions = mPlayerActions;
	input.playerIndex = e.GetPlayer()->GetPlayerIndex();

	entity.AddComponent<CharacterMotor>();
});
```

The event fires during `PlayerInputSystem::OnPreUpdate`, and the new entity's state is built on the next update. Each player reads only its own gamepad (or the keyboard and mouse), so one asset serves every player and the controller system above needs no changes.

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
