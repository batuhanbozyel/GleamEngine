//
//  PolyArray.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 18.03.2023.
//

#pragma once
#include "Array.h"
#include "Pointer.h"

#include <entt/core/type_info.hpp>

namespace Gleam {

template<class Base>
class PolyArray
{
	struct Element
	{
		uint32_t hash;
		Scope<Base> object;
	};
	
	using Container = TArray<Element>;
	
public:
	
	class iterator
	{
	public:
		
		iterator(typename Container::iterator it)
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
		
		Base* operator*() const
		{
			return it->object.get();
		}
		
	private:
		
		typename Container::iterator it;
		
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
		
		const Base* operator*() const
		{
			return it->object.get();
		}
		
	private:
		
		typename Container::const_iterator it;
		
	};
	
	PolyArray() = default;
	PolyArray(PolyArray&&) = default;
	PolyArray& operator=(PolyArray&&) noexcept = default;
	
	~PolyArray()
	{
		clear();
	}
	
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
	
	Base* operator[](size_t index)
	{
		return mData[index].object.get();
	}

	const Base* operator[](size_t index) const
	{
		return mData[index].object.get();
	}
	
	template<class T, class...Args>
	T* emplace_back(Args&&... args) noexcept
	{
		uint32_t hash = entt::type_hash<T>().value();
		auto it = eastl::find_if(mData.begin(), mData.end(), [hash](const auto& element)
		{
			return element.hash == hash;
		});
		if (it != mData.end())
		{
			it->object = CreateScope<T>(std::forward<Args>(args)...);
			return static_cast<T*>(it->object.get());
		}
		auto& element = mData.emplace_back(Element{ hash, CreateScope<T>(std::forward<Args>(args)...) });
		return static_cast<T*>(element.object.get());
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
	T* get() const
	{
		uint32_t hash = entt::type_hash<T>().value();
		auto it = eastl::find_if(mData.begin(), mData.end(), [hash](const auto& element)
		{
			return element.hash == hash;
		});
		if (it != mData.end())
		{
			return static_cast<T*>(it->object.get());
		}
		return nullptr;
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
