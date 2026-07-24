#pragma once
#include "Subsystem.h"
#include "Container/Hash.h"
#include "Container/Array.h"
#include "Container/Pointer.h"
#include "Container/String.h"
#include "IO/Filesystem.h"
#include "IO/Log.h"

#include <Reflection/Reflection.h>
#ifndef __GLEAM_REFLECTION__
#include <Runtime.Reflection.generated.h>
#endif

#include <functional>

namespace Gleam {

class JSONSerializer;

template<typename T>
concept ConfigType = Reflection::Traits::IsReflected<T>::value;

template<ConfigType T>
using ConfigCallbackFunc = std::function<void(const T&)>;

template<ConfigType T>
using ConfigMutatorFunc = std::function<void(T&)>;

using ConfigCallbackHandle = uint64_t;

class ConfigSystem final : public EngineSubsystem
{
public:
	
	virtual void Initialize(Engine* engine) override;
	
	virtual void Shutdown(Engine* engine) override;
	
	template<ConfigType T>
	const T& Get() const
	{
		return Reflection::Get<T>(GetBlock<T>().data);
	}

	template<ConfigType T>
	void Set(const T& value)
	{
		auto& block = GetBlock<T>();
		memcpy(block.data, &value, sizeof(T));
		FlushToDisk();
		block.Notify();
	}

	template<ConfigType T>
	void Modify(const ConfigMutatorFunc<T>& mutator)
	{
		auto& block = GetBlock<T>();
		mutator(Reflection::Get<T>(block.data));
		FlushToDisk();
		block.Notify();
	}
	
	template<ConfigType T>
	const T& Register()
	{
		const auto& classDesc = Reflection::GetClass<T>();
		if (HasBlock(classDesc))
		{
			auto& block = GetBlock(classDesc);
			block.Notify();
			return Reflection::Get<T>(block.data);
		}
		auto& block = RegisterBlock(classDesc);
		auto& value = Reflection::Get<T>(block.data);
		value = T();
		FlushToDisk();
		block.Notify();
		return value;
	}

	template<ConfigType T>
	ConfigCallbackHandle Subscribe(ConfigCallbackFunc<T>&& callback)
	{
		auto& block = GetBlock<T>();
		ConfigCallbackHandle handle = mNextHandle++;
		block.subscribers.push_back({ handle,
			[callback = std::move(callback)](const void* userData)
			{
				std::invoke(callback, Reflection::Get<T>(userData));
			}
		});
		return handle;
	}

	template<ConfigType T>
	void Unsubscribe(ConfigCallbackHandle handle)
	{
		auto& subs = GetBlock<T>().subscribers;
		for (auto it = subs.begin(); it != subs.end(); ++it)
		{
			if (it->handle == handle)
			{
				subs.erase(it);
				break;
			}
		}
	}

	struct ConfigView
	{
		const Reflection::ClassDescription& desc;
		void* data = nullptr;
	};
	void ForEachConfig(const std::function<void(const ConfigView&)>& visitor);

	void MarkModified(uint32_t typeHash);

private:

	struct ConfigBlock
	{
		struct Subscriber
		{
			ConfigCallbackHandle handle;
			std::function<void(const void*)> fn;
		};
		
		void* data = nullptr;
		uint32_t typeHash = 0;
		TArray<Subscriber> subscribers;
		
		ConfigBlock(const Reflection::ClassDescription& description)
			: typeHash(description.TypeHash())
		{
			data = ::operator new(description.GetSize());
			memset(data, 0, description.GetSize());
		}
		
		~ConfigBlock()
		{
			::operator delete(data);
		}
		
		void Notify() const
		{
			for (const auto& sub : subscribers)
			{
				sub.fn(data);
			}
		}
	};
	
	ConfigBlock& RegisterBlock(const Reflection::ClassDescription& classDesc);

	template<ConfigType T>
	ConfigBlock& GetBlock() const
	{
		GLEAM_ASSERT(HasBlock<T>(), "Config type is not registered!");
		const auto& classDesc = Reflection::GetClass<T>();
		return GetBlock(classDesc);
	}
	
	ConfigBlock& GetBlock(const Reflection::ClassDescription& classDesc) const;
	
	template<ConfigType T>
	bool HasBlock() const
	{
		const auto& classDesc = Reflection::GetClass<T>();
		return HasBlock(classDesc);
	}
	
	bool HasBlock(const Reflection::ClassDescription& classDesc) const;

	void FlushToDisk() const;

	Path ConfigFilePath() const;

	JSONSerializer* mSerializer = nullptr;
	HashMap<uint32_t, ConfigBlock*> mBlocks;
	ConfigCallbackHandle mNextHandle = 1;
};

} // namespace Gleam
