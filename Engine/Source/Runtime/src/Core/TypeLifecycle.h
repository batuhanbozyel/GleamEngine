#pragma once
#include "Container/Hash.h"
#include "IO/Log.h"

#include <Reflection/Reflection.h>
#include <EASTL/vector.h>

#include <new>
#include <utility>
#include <type_traits>

namespace Gleam {

class TypeLifecycle
{
public:

	virtual ~TypeLifecycle() = default;

	// Construct, Copy and Move build into uninitialized memory, Destruct destroys in place
	virtual void Construct(void* obj) const = 0;

	virtual void Destruct(void* obj) const = 0;

	virtual void Copy(void* obj, const void* source) const = 0;

	virtual void Move(void* obj, void* source) const = 0;

	size_t GetSize() const
	{
		return mSize;
	}

	size_t GetAlignment() const
	{
		return mAlignment;
	}

	bool IsTrivial() const
	{
		return mTrivial;
	}

protected:

	size_t mSize = 0;
	size_t mAlignment = 0;
	bool mTrivial = false;
};

class VectorLifecycle : public TypeLifecycle
{
public:

	virtual size_t GetElementSize() const = 0;

	virtual void* Data(void* obj) const = 0;

	// Destroys every element, then default constructs count elements
	virtual void Reset(void* obj, size_t count) const = 0;
};

template<typename T>
class CompiledLifecycle final : public TypeLifecycle
{
public:

	CompiledLifecycle()
	{
		mSize = sizeof(T);
		mAlignment = alignof(T);
		mTrivial = std::is_trivially_copyable_v<T>;
	}

	virtual void Construct(void* obj) const override
	{
		if constexpr (std::is_default_constructible_v<T>)
		{
			new (obj) T();
		}
		else
		{
			GLEAM_ASSERT(false, "Type is not default constructible");
		}
	}

	virtual void Destruct(void* obj) const override
	{
		if constexpr (std::is_destructible_v<T>)
		{
			static_cast<T*>(obj)->~T();
		}
		else
		{
			GLEAM_ASSERT(false, "Type is not destructible");
		}
	}

	virtual void Copy(void* obj, const void* source) const override
	{
		if constexpr (std::is_copy_constructible_v<T>)
		{
			new (obj) T(*static_cast<const T*>(source));
		}
		else
		{
			GLEAM_ASSERT(false, "Type is not copy constructible");
		}
	}

	virtual void Move(void* obj, void* source) const override
	{
		if constexpr (std::is_move_constructible_v<T>)
		{
			new (obj) T(std::move(*static_cast<T*>(source)));
		}
		else
		{
			GLEAM_ASSERT(false, "Type is not move constructible");
		}
	}
};

template<typename T>
class CompiledVectorLifecycle final : public VectorLifecycle
{
public:

	CompiledVectorLifecycle()
	{
		mSize = sizeof(T);
		mAlignment = alignof(T);
		mTrivial = false;
	}

	virtual void Construct(void* obj) const override
	{
		new (obj) T();
	}

	virtual void Destruct(void* obj) const override
	{
		static_cast<T*>(obj)->~T();
	}

	virtual void Copy(void* obj, const void* source) const override
	{
		new (obj) T(*static_cast<const T*>(source));
	}

	virtual void Move(void* obj, void* source) const override
	{
		new (obj) T(std::move(*static_cast<T*>(source)));
	}

	virtual size_t GetElementSize() const override
	{
		return sizeof(typename T::value_type);
	}

	virtual void* Data(void* obj) const override
	{
		return static_cast<T*>(obj)->data();
	}

	virtual void Reset(void* obj, size_t count) const override
	{
		auto& vector = *static_cast<T*>(obj);
		vector.clear();
		vector.resize(count);
	}
};

class TypeLifecycleRegistry final
{
public:

	static TypeLifecycleRegistry* GetInstance()
	{
		GLEAM_ASSERT(sInstance != nullptr, "Type lifecycle registry is not initialized");
		return sInstance;
	}

	void Initialize();

	void Shutdown();

	template<typename T>
	void Register()
	{
		const auto typeHash = Reflection::TypeHash<T>();
		if constexpr (Reflection::Traits::IsInstanceOfTemplate<T, eastl::vector>::value)
		{
			auto lifecycle = new CompiledVectorLifecycle<T>();
			mLifecycles[typeHash] = lifecycle;
			mVectors[typeHash] = lifecycle;
		}
		else
		{
			mLifecycles[typeHash] = new CompiledLifecycle<T>();
		}
	}

	void Register(uint32_t typeHash, TypeLifecycle* lifecycle)
	{
		mLifecycles[typeHash] = lifecycle;
	}

	void Register(uint32_t typeHash, VectorLifecycle* lifecycle)
	{
		mLifecycles[typeHash] = lifecycle;
		mVectors[typeHash] = lifecycle;
	}

	const TypeLifecycle* Find(uint32_t typeHash) const;

	const VectorLifecycle* FindVector(uint32_t typeHash) const;

private:

	HashMap<uint32_t, TypeLifecycle*> mLifecycles;

	HashMap<uint32_t, const VectorLifecycle*> mVectors;

	static inline TypeLifecycleRegistry* sInstance = nullptr;
};

#define REGISTER_TYPE_LIFECYCLE(Type) ::Gleam::TypeLifecycleRegistry::GetInstance()->Register<Type>();

} // namespace Gleam
