#pragma once
#include "Array.h"

#include <entt/core/type_info.hpp>
#include <EASTL/any.h>

namespace Gleam {

class AnyArray
{
	struct Element
	{
		uint32_t hash;
		eastl::any object;
	};
	
	using Container = TArray<Element>;

public:

	class iterator
	{
	public:

		iterator(Container::iterator it)
			: it(it) {}

		iterator& operator++()
		{
			++it;
			return *this;
		}

		iterator operator++(int)
		{
			iterator copy = *this;
			++it;
			return copy;
		}

		bool operator==(const iterator& other) const
		{
			return it == other.it;
		}

		bool operator!=(const iterator& other) const
		{
			return it != other.it;
		}

		eastl::any& operator*()
		{
			return it->object;
		}

	private:

		Container::iterator it;

	};
	
	class const_iterator
	{
	public:
		
		const_iterator(typename Container::const_iterator it)
			: it(it) {}
		
		const_iterator& operator++()
		{
			++it;
			return *this;
		}
		
		const_iterator operator++(int)
		{
			const_iterator copy = *this;
			++it;
			return copy;
		}
		
		bool operator==(const const_iterator& other) const
		{
			return it == other.it;
		}
		
		bool operator!=(const const_iterator& other) const
		{
			return it != other.it;
		}
		
		const eastl::any& operator*() const
		{
			return it->object;
		}
		
	private:
		
		typename Container::const_iterator it;
		
	};

	iterator begin()
	{
		return iterator(mData.begin());
	}

	iterator end()
	{
		return iterator(mData.end());
	}
	
	const_iterator begin() const
	{
		return const_iterator(mData.begin());
	}
	
	const_iterator end() const
	{
		return const_iterator(mData.end());
	}
	
	eastl::any& operator[](size_t index)
	{
		return mData[index].object;
	}
	
	const eastl::any& operator[](size_t index) const
	{
		return mData[index].object;
	}
	
	template<class T>
	T& operator[](size_t index)
	{
		return eastl::any_cast<T&>(mData[index].object);
	}

	template<class T>
	const T& operator[](size_t index) const
	{
		return eastl::any_cast<const T&>(mData[index].object);
	}
	
	template<class T, class...Args>
	T& emplace_back(Args&&... args) noexcept
	{
		uint32_t hash = entt::type_hash<T>().value();
		auto it = eastl::find_if(mData.begin(), mData.end(), [hash](const auto& element)
		{
			return element.hash == hash;
		});
		if (it != mData.end())
		{
			return it->object.template emplace<T>(T{std::forward<Args>(args)...});
		}
		auto& element = mData.emplace_back(Element{ hash, eastl::any{} });
		return element.object.template emplace<T>(T{std::forward<Args>(args)...});
	}

	void clear()
	{
		mData.clear();
	}

	template<class T>
	size_t erase()
	{
		uint32_t hash = entt::type_hash<T>().value();
		auto it = eastl::find_if(mData.begin(), mData.end(), [hash](const auto& element)
		{
			return element.hash == hash;
		});
		if (it != mData.end())
		{
			mData.erase(it);
			return 1;
		}
		return 0;
	}

	template<class T>
	T* get()
	{
		uint32_t hash = entt::type_hash<T>().value();
		auto it = eastl::find_if(mData.begin(), mData.end(), [hash](const auto& element)
		{
			return element.hash == hash;
		});
		if (it != mData.end())
		{
			return &(eastl::any_cast<T&>(it->object));
		}
		return nullptr;
	}

	template<class T>
	T& get_unsafe()
	{
		uint32_t hash = entt::type_hash<T>().value();
		auto it = eastl::find_if(mData.begin(), mData.end(), [hash](const auto& element)
		{
			return element.hash == hash;
		});
		return eastl::any_cast<T&>(it->object);
	}
	
	template<class T>
	const T& get_unsafe() const
	{
		uint32_t hash = entt::type_hash<T>().value();
		auto it = eastl::find_if(mData.begin(), mData.end(), [hash](const auto& element)
		{
			return element.hash == hash;
		});
		return eastl::any_cast<const T&>(it->object);
	}
	
	template<class T>
	bool contains() const
	{
		uint32_t hash = entt::type_hash<T>().value();
		auto it = eastl::find_if(mData.begin(), mData.end(), [hash](const auto& element)
		{
			return element.hash == hash;
		});
		return it != mData.end();
	}

	size_t size() const
	{
		return mData.size();
	}

private:

	Container mData;

};

} // namespace Gleam
