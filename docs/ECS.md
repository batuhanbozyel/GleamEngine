# Entity Component System

Gleam stores game state as plain components on entities and processes it in systems. The ECS is built on [EnTT](https://github.com/skypjack/entt) and lives in `Engine/Source/Runtime/src/World/`.

## Table of Contents
* [Overview](#overview)
* [Worlds](#worlds)
* [Entities](#entities)
* [Components](#components)
* [Querying](#querying)
* [Singletons](#singletons)
* [Systems](#systems)
* [World Subsystems](#world-subsystems)
* [Change Tracking](#change-tracking)
* [Frame Order and Time](#frame-order-and-time)
* [Prefabs and References](#prefabs-and-references)

## Overview

| Type | Role |
| --- | --- |
| `World` | An asset (`.gworld`) that owns one `EntityManager`, its component systems and its world subsystems |
| `WorldManager` | Game instance subsystem that loads worlds from the project's build list and tracks the active one |
| `EntityManager` | Creates and destroys entities, adds/gets/removes components, runs queries |
| `Entity` | The component every entity has: name, GUID, active flag, hierarchy and transform |
| `ComponentSystem` | Gameplay logic with update and fixed-update callbacks |
| `WorldSubsystem` | Per-world service (for example `RenderSceneProxy`), optionally ticked |
| `ChangeTracker` | Records when each component was added or last changed |

## Worlds

A project lists its worlds in `WorldConfig`. When the `Application` starts, `WorldManager` opens `startingWorldIndex` and makes it the active world.

```cpp
auto worldManager = Globals::GameInstance->GetSubsystem<WorldManager>();
worldManager->OpenWorld(1);                      // load and activate build index 1
World* world = worldManager->GetActiveWorld();
EntityManager& entityManager = world->GetEntityManager();
```

`LoadWorld(index)` loads a world without activating it; `SetActiveWorld(world)` switches to it.

Every world is created with:

- `RenderSceneProxy` (world subsystem): mirrors `MeshRenderer` components for the renderer.
- `PhysicsSystem` (component system): steps physics, using `WorldDescriptor::gravity`.

## Entities

Each entity carries an `Entity` component holding its name, GUID, active flag, parent/children and transforms. `EntityHandle` (`entt::entity`) is the lightweight id; `InvalidEntity` is the null handle.

```cpp
Entity& player = entityManager.CreateEntity("Player", Guid::NewGuid());
Entity& weapon = entityManager.CreateEntity("Weapon", Guid::NewGuid(), MeshRenderer{ .mesh = swordMesh });

weapon.SetParent(player);         // keeps the local transform, recomputes the world transform
player.SetActive(false);

entityManager.DestroyEntity(player);   // also destroys all children
```

`CreateEntity` with a GUID that already exists returns the existing entity instead of creating a new one.

### Transforms

Transforms live on `Entity`, not in a separate component. `Transform` is `{ position, rotation (Quaternion), scale (uniform float) }`.

```cpp
player.SetTranslation(Float3{ 0.0f, 1.0f, 0.0f });
player.Translate(player.ForwardVector() * speed * dt);
player.Rotate(0.0f, Math::Deg2Rad(90.0f), 0.0f);   // Euler angles in radians
player.SetScale(2.0f);

const Transform& world = player.GetWorldTransform();
const Float3& localPosition = player.GetLocalPosition();
Float4x4 matrix = world;          // Transform converts to a TRS matrix
```

Setters write the local transform and immediately propagate world transforms to every descendant. Each changed entity is marked as a `Transform` change in the change tracker.

## Components

Components are plain structs. Engine components are reflected so they can be serialized, edited in the inspector and added by type hash:

```cpp
GSTRUCT(Health, "6F2B6C1E-9C61-4E43-9A5F-1D9B3A1C2E77", EntityComponent, Serializable, PrettyName("Health"))
{
	GFIELD("0C7D4A60-2C8F-4E54-8D7C-3B4E5F607182", Serializable, PrettyName("Max"))
	float max = 100.0f;

	GFIELD("7A1B2C3D-4E5F-4061-8273-94A5B6C7D8E9", Serializable, PrettyName("Current"))
	float current = 100.0f;
};
```

- `EntityComponent` marks it as a component the editor and serializers recognize.
- `Serializable` on the struct and each field makes it save with the world and prefabs.
- `EditorOnly` keeps a component (and any entity carrying one) out of play-mode copies.
- Register every reflected component in `ScriptingSystem::Initialize` with `RegisterMetaComponent<T>()`. Type-erased add/remove and the play-mode world copy rely on it, and the copy asserts on unregistered components.

Built-in components: `Camera`, `MeshRenderer`, `SkyAtmosphere`, `ReflectionProbe`, `RigidBody`.

### Adding, reading and removing

The same API exists on `EntityManager` (taking a handle) and on `Entity` (acting on itself):

```cpp
entityManager.AddComponent<Health>(player, Health{ .max = 150.0f, .current = 150.0f });
entityManager.SetComponent<Health>(player, Health{});   // add or replace

if (entityManager.HasComponent<Health>(player))
{
	Health& health = entityManager.GetComponent<Health>(player);    // marks Health as changed
	health.current -= damage;
}

const Health& health = eastl::as_const(entityManager).GetComponent<Health>(player);   // read-only, no change mark
entityManager.RemoveComponent<Health>(player);

player.AddComponent<RigidBody>();
```

`AddComponent` asserts if the component already exists; `GetComponent` and `RemoveComponent` assert if it does not.

> The non-const `GetComponent` always marks the component as changed. Read through a `const EntityManager&` or `const Entity&` when you do not intend to write, so change-driven systems do not do extra work.

### Type-erased access

Tools and serializers work with reflection type hashes:

```cpp
void* component = entityManager.FindComponent(entity, Reflection::GetClass<Health>().TypeHash());
entityManager.AddComponent(entity, typeHash);
entityManager.RemoveComponent(entity, typeHash);
entityManager.Visit(entity, [](void* component, const Reflection::ClassDescription& classDesc)
{
	// every reflected component on the entity
});
```

## Querying

`ForEach` iterates every entity that has all listed components. The callback may take the handle first.

```cpp
entityManager.ForEach<Entity, Health>([](Entity& entity, Health& health)
{
	if (health.current <= 0.0f)
	{
		entity.SetActive(false);
	}
});

entityManager.ForEach<Entity, Camera>([](EntityHandle handle, const Entity& entity, const Camera& camera)
{
});

entityManager.ForEach<RigidBody>([](RigidBody& body)
{
}, Exclude<Health>{});

entityManager.ForEach([](EntityHandle handle)
{
	// every entity
});
```

Components received by reference inside `ForEach` are **not** marked as changed. Call `GetChangeTracker().MarkChanged<T>(entity)` yourself if other systems react to the change.

Other lookups: `IsValid(handle)`, `GetEntityCount()`, and `GetEntity(EntityReference)` to resolve a GUID reference.

## Singletons

Singletons are components stored on a hidden entity, for world-wide state:

```cpp
entityManager.SetSingleton<GameRules>(GameRules{ .roundTime = 120.0f });
GameRules& rules = entityManager.GetSingleton<GameRules>();
```

`ForEach` without component types skips the singleton entity.

## Systems

Derive from `ComponentSystem` and override any callbacks you need. Each receives the world's `EntityManager`.

```cpp
class HealthRegenSystem final : public ComponentSystem
{
public:

	float regenPerSecond = 5.0f;

protected:

	virtual void OnUpdate(EntityManager& entityManager) override
	{
		const float dt = static_cast<float>(Timestep::deltaTime);
		entityManager.ForEach<Health>([&](Health& health)
		{
			health.current = Math::Min(health.current + regenPerSecond * dt, health.max);
		});
	}
};

auto regen = world->AddSystem<HealthRegenSystem>();
regen->regenPerSecond = 10.0f;
regen->Enabled = false;            // skipped by World::Update
world->GetSystem<HealthRegenSystem>();
world->RemoveSystem<HealthRegenSystem>();
```

| Callback | When |
| --- | --- |
| `OnCreate` | Once, inside `AddSystem` |
| `OnPreFixedUpdate` / `OnFixedUpdate` / `OnPostFixedUpdate` | Zero or more times per frame, once per fixed step |
| `OnPreUpdate` / `OnUpdate` / `OnPostUpdate` | Once per frame |
| `OnDestroy` | In `RemoveSystem`, or when the world is destroyed (reverse order) |

Systems run in the order they were added. A world allows one system per type.

## World Subsystems

`WorldSubsystem` is for per-world services that are not entity loops: scene proxies, picking, undo, selection. Derive from `TickableWorldSubsystem` to get a `Tick(World*)` call every frame, before fixed updates.

```cpp
class SpawnDirector final : public TickableWorldSubsystem
{
protected:

	virtual void Initialize(World* world) override {}

	virtual void Tick(World* world) override
	{
		auto& entityManager = world->GetEntityManager();
	}

	virtual void Shutdown(World* world) override {}
};

world->AddSubsystem<SpawnDirector>();
world->GetSubsystem<SpawnDirector>();
```

## Change Tracking

Every `EntityManager` owns a `ChangeTracker`. It keeps a global tick that increases on each add or change, and records the `added` and `changed` tick per component per entity. Systems use it to process only what changed since they last looked.

What marks a change:

- `AddComponent` / `SetComponent`: added (and changed).
- Non-const `GetComponent`: changed.
- Transform setters, `SetParent` and parent movement: `Transform` changed on the entity and all descendants.
- `CreateEntity`: `Transform` changed.
- Manual: `GetChangeTracker().MarkChanged<T>(entity)`.

Use a `ChangeCursor` to remember the last tick a system processed:

```cpp
class HealthBarSystem final : public ComponentSystem
{
protected:

	virtual void OnUpdate(EntityManager& entityManager) override
	{
		const Tick since = mCursor.Begin(entityManager.GetChangeTracker());
		eastl::as_const(entityManager).ForEachChanged<Health>(since, [&](EntityHandle entity, const Health& health)
		{
			UpdateHealthBar(entity, health.current / health.max);
		});
		mCursor.Commit();
	}

private:

	ChangeCursor mCursor;
};
```

- `ForEachAdded<T>(since, fn)` visits only components added after `since`.
- For tight loops, fetch a view once: `auto transforms = tracker.GetTicks<Transform>();` then `transforms.IsChanged(entity, since)`.
- Removal is not tracked by ticks. Subscribe instead, and keep the returned `ComponentCallback` alive. It disconnects when destroyed.

```cpp
mHealthRemoved = entityManager.OnComponentRemoved<Health, &HealthBarSystem::OnHealthRemoved>(*this);

void HealthBarSystem::OnHealthRemoved(EntityHandle entity) { ... }
```

`RenderSceneProxy` (meshes) and `PhysicsSystem` (rigid bodies and transforms) are the in-tree examples of this pattern.

## Frame Order and Time

`Application::Run` does, per frame:

1. Events, then input (`InputSystem::Update`).
2. Tickable game instance subsystems.
3. `World::Update()` on the active world:
    1. `Timestep::Step()`, then every tickable world subsystem.
    2. While a fixed step is due: `OnPreFixedUpdate`, `OnFixedUpdate`, `OnPostFixedUpdate` for all enabled systems.
    3. `OnPreUpdate`, `OnUpdate`, `OnPostUpdate` for all enabled systems.
4. `RenderSystem::PreRender(world)` and `Render(world)`.

`Timestep` exposes static, read-only `deltaTime`, `elapsedTime`, `fixedTime` and `alpha` (interpolation factor between fixed steps). It also has the settings `fixedDeltaTime` (default 1/60 s), `maxDeltaTime` (default 0.25 s) and `timeScale`.

## Prefabs and References

`EntityManager::CreateFromPrefab(assetRef)` instantiates a `.prefab` asset (an entity hierarchy with its serialized components) and returns the root `Entity`.

To point at another entity from a component, store an `EntityReference` (a GUID) rather than an `EntityHandle`. Handles are not stable across save/load or play-mode copies; GUIDs are.

```cpp
GSTRUCT(Follow, "1E2D3C4B-5A69-4788-97A6-B5C4D3E2F100", EntityComponent, Serializable)
{
	GFIELD("2F3E4D5C-6B7A-4899-A8B7-C6D5E4F30211", Serializable)
	EntityReference target;
};

if (follow.target)
{
	EntityHandle target = entityManager.GetEntity(follow.target);
}
```
