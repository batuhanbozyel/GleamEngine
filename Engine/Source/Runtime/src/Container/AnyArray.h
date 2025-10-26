#pragma once
#include "Hash.h"

#include <entt/core/type_info.hpp>
#include <EASTL/any.h>

namespace Gleam {

class AnyArray
{
	using Container = HashMap<uint32_t, eastl::any>;

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
			return it->second;
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
            return it->second;
        }
        
    private:
        
        typename Container::const_iterator it;
        
    };

	iterator begin()
	{
		return iterator(data.begin());
	}

	iterator end()
	{
		return iterator(data.end());
	}
    
    const_iterator begin() const
    {
        return const_iterator(data.begin());
    }
    
    const_iterator end() const
    {
        return const_iterator(data.end());
    }
    
	template<class T, class...Args>
	T& emplace(Args&&... args) noexcept
	{
        return data[entt::type_hash<T>().value()].emplace<T>(T{std::forward<Args>(args)...});
	}
    
    template<class T>
    T& emplace(const T& obj)
    {
        return data[entt::type_hash<T>().value()].emplace<T>(obj);
    }

	void clear()
	{
		data.clear();
	}

	template<class T>
	size_t erase()
	{
		return data.erase(entt::type_hash<T>().value());
	}

	template<class T>
	T* get()
	{
		auto it = data.find(entt::type_hash<T>().value());
		if (it == data.end()) return nullptr;
		return &(eastl::any_cast<T&>(it->second));
	}

	template<class T>
	T& get_unsafe()
	{
		auto it = data.find(entt::type_hash<T>().value());
		return eastl::any_cast<T&>(it->second);
	}
    
    template<class T>
    const T& get_unsafe() const
    {
        auto it = data.find(entt::type_hash<T>().value());
        return eastl::any_cast<const T&>(it->second);
    }
	
	template<class T>
	bool contains() const
	{
		return data.find(entt::type_hash<T>().value()) != data.end();
	}

	size_t size() const
	{
		return data.size();
	}

private:

	Container data;

};

} // namespace Gleam
