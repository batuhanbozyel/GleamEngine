#pragma once
#include "Core/Subsystem.h"
#include "Entity.h"
#include "Reflection/Reflection.h"

namespace Gleam {

class ScriptingSystem final : public EngineSubsystem
{
public:

	virtual void Initialize(Engine* engine) override;

	virtual void Shutdown() override;

	template<typename T>
	static void RegisterMetaComponent()
	{
		const auto& classDesc = Reflection::GetClass<T>();
		if (classDesc.HasAttribute<Reflection::Attribute::EntityComponent>())
		{
			entt::meta<T>()
				.type(entt::type_hash<T>::value())
				.template func<&AddComponent<T>, entt::as_ref_t>("AddComponent"_hs)
				.template func<&RemoveComponent<T>>("RemoveComponent"_hs)
				.template func<&HasComponent<T>>("HasComponent"_hs);
		}
	}

private:

	template<typename T>
	static T& AddComponent(Ref<Entity> entity)
	{
		return entity.get().AddComponent<T>();
	}

	template<typename T>
	static void RemoveComponent(Ref<Entity> entity)
	{
		entity.get().RemoveComponent<T>();
	}

	template<typename T>
	static bool HasComponent(Ref<Entity> entity)
	{
		return entity.get().HasComponent<T>();
	}

};

} // namespace Gleam
