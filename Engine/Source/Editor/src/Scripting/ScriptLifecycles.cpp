//
//  ScriptLifecycles.cpp
//  Editor
//

#include "ScriptLifecycles.h"
#include "ComposedLifecycle.h"
#include "RawVectorLifecycle.h"

#include "Container/String.h"
#include "Serialization/ReflectionUtils.h"

#include <Runtime.Reflection.generated.h>
#include <EASTL/algorithm.h>
#include <cstring>

using namespace GEditor;

static auto TemplateName(const Gleam::Reflection::ClassDescription& classDesc)
{
	const auto qualifiedName = classDesc.ResolveQualifiedName();
	return qualifiedName.substr(0, qualifiedName.find_first_of('<'));
}

class ScriptLifecycleBuilder
{
public:

	ScriptLifecycleBuilder(Gleam::TArray<uint32_t>& types, Gleam::TArray<ComposedLifecycle*>& composed)
		: mTypes(types)
		, mComposed(composed)
	{

	}

	const Gleam::TypeLifecycle& Resolve(Gleam::Reflection::MetaType type, uint32_t typeHash)
	{
		auto registry = Gleam::TypeLifecycleRegistry::GetInstance();
		if (const auto lifecycle = registry->Find(typeHash))
		{
			return *lifecycle;
		}

		switch (type)
		{
			case Gleam::Reflection::MetaType::Primitive:
			{
				const auto size = Gleam::Reflection::GetPrimitive(typeHash).GetSize();
				auto lifecycle = new ComposedLifecycle(size);
				lifecycle->Initialize(size, {});
				Register(typeHash, lifecycle);
				break;
			}
			case Gleam::Reflection::MetaType::Enum:
			{
				const auto enumDesc = Gleam::Reflection::GetEnum(typeHash);
				auto lifecycle = new ComposedLifecycle(enumDesc->GetSize());
				lifecycle->Initialize(enumDesc->GetSize(), {});
				if (const auto cases = enumDesc->Cases(); cases.size() != 0)
				{
					Gleam::ReflectionUtils::WriteEnumValue(lifecycle->GetPrototype(), *enumDesc, cases[0].Value());
				}
				Register(typeHash, lifecycle);
				break;
			}
			case Gleam::Reflection::MetaType::Class:
			{
				BuildClass(*Gleam::Reflection::GetClass(typeHash));
				break;
			}
			default:
			{
				GLEAM_ASSERT(false, "Unsupported meta type for a lifecycle");
				break;
			}
		}
		return *registry->Find(typeHash);
	}

private:

	void BuildClass(const Gleam::Reflection::ClassDescription& classDesc)
	{
		if (TemplateName(classDesc) == TemplateName(Gleam::Reflection::GetClass<Gleam::TArray<uint8_t>>()))
		{
			const auto& element = classDesc.ResolveTemplateParameters()[0];
			Register(classDesc.TypeHash(), new RawVectorLifecycle(Resolve(element.GetType(), element.TypeHash())));
		}
		else
		{
			auto lifecycle = new ComposedLifecycle(classDesc.GetSize());
			Register(classDesc.TypeHash(), lifecycle);

			Gleam::TArray<ComposedLifecycle::Member> members;
			size_t alignment = 1;
			auto addMember = [&](size_t offset, const Gleam::TypeLifecycle& lifecycle)
			{
				members.push_back({ offset, &lifecycle });
				alignment = eastl::max(alignment, lifecycle.GetAlignment());
			};

			for (const auto& base : classDesc.ResolveBaseClasses())
			{
				addMember(base.GetOffset(), Resolve(Gleam::Reflection::MetaType::Class, base.TypeHash()));
			}

			for (const auto& field : classDesc.ResolveFields())
			{
				if (field.GetType() == Gleam::Reflection::MetaType::Array)
				{
					const auto arrayDesc = Gleam::Reflection::GetArray(field.TypeHash());
					const auto& element = Resolve(arrayDesc->ElementType(), arrayDesc->ElementHash());
					for (size_t offset = 0; offset < arrayDesc->GetSize(); offset += element.GetSize())
					{
						addMember(field.GetOffset() + offset, element);
					}
				}
				else
				{
					addMember(field.GetOffset(), Resolve(field.GetType(), field.TypeHash()));
				}
			}

			lifecycle->Initialize(alignment, members);
			ApplyFieldDefaults(classDesc, lifecycle->GetPrototype());
		}
	}

	void ApplyFieldDefaults(const Gleam::Reflection::ClassDescription& classDesc, void* prototype)
	{
		for (const auto& field : classDesc.ResolveFields())
		{
			const auto defaultValue = field.DefaultValue();
			if (defaultValue.size() == 0)
			{
				continue;
			}

			auto target = static_cast<uint8_t*>(prototype) + field.GetOffset();
			if (field.TypeHash() == Gleam::Reflection::TypeHash<Gleam::TString>())
			{
				Gleam::Reflection::Get<Gleam::TString>(target).assign(reinterpret_cast<const char*>(defaultValue.data()), defaultValue.size());
			}
			else if (field.GetType() != Gleam::Reflection::MetaType::Class or Resolve(field.GetType(), field.TypeHash()).IsTrivial())
			{
				std::memcpy(target, defaultValue.data(), defaultValue.size());
			}
		}
	}

	void Register(uint32_t typeHash, ComposedLifecycle* lifecycle)
	{
		Gleam::TypeLifecycleRegistry::GetInstance()->Register(typeHash, lifecycle);
		mTypes.push_back(typeHash);
		mComposed.push_back(lifecycle);
	}

	void Register(uint32_t typeHash, RawVectorLifecycle* lifecycle)
	{
		Gleam::TypeLifecycleRegistry::GetInstance()->Register(typeHash, static_cast<Gleam::VectorLifecycle*>(lifecycle));
		mTypes.push_back(typeHash);
	}

	Gleam::TArray<uint32_t>& mTypes;

	Gleam::TArray<ComposedLifecycle*>& mComposed;
};

void ScriptLifecycles::RegisterAll()
{
	ScriptLifecycleBuilder builder(mTypes, mComposed);
	for (const auto& classDesc : Gleam::Reflection::IDatabase::GetInstance()->GetClasses())
	{
		builder.Resolve(Gleam::Reflection::MetaType::Class, classDesc.TypeHash());
	}
}

void ScriptLifecycles::UnregisterAll()
{
	for (auto lifecycle : mComposed)
	{
		lifecycle->ReleasePrototype();
	}

	auto registry = Gleam::TypeLifecycleRegistry::GetInstance();
	for (const auto typeHash : mTypes)
	{
		registry->Unregister(typeHash);
	}
	mTypes.clear();
	mComposed.clear();
}
