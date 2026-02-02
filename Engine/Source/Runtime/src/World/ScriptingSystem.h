#pragma once
#include "Core/Subsystem.h"
#include "Container/Pointer.h"
#include "Entity.h"

#include <entt/meta/policy.hpp>
#include <entt/meta/factory.hpp>

namespace Gleam {

class ScriptingSystem final : public EngineSubsystem
{
public:

	virtual void Initialize(Engine* engine) override;

	virtual void Shutdown(Engine* engine) override;

	template<typename T>
	static void RegisterMetaComponent()
	{
		const auto& classDesc = Reflection::GetClass<T>();
		if (classDesc.template HasAttribute<Reflection::Attribute::EntityComponent>())
		{
			entt::meta_factory<T>()
				.type(classDesc.TypeHash())
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
