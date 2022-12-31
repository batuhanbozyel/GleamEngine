#pragma once
#include <any>
#include <typeindex>

namespace Gleam {

class AnyArray
{
	using Container = HashMap<std::type_index, std::any>;

public:

	class iterator
	{
	public:

		iterator(Container::iterator it)
			: it(it) {}

		iterator& operator++()
		{
			++it;
			return *it;
		}

		iterator operator++(int)
		{
			iterator copy = *this;
			++it;
			return copy;
		}

		iterator& operator--()
		{
			--it;
			return *it;
		}

		iterator operator--(int)
		{
			iterator copy = *this;
			--it;
			return copy;
		}

		bool operator==(const iterator& other) const
		{
			return it == other.it;
		}

    	bool operator!=(const iterator& other) const
		{
			return !(*this == other);
		}

		std::any& operator*() const
		{
			return it->second;
		}

	private:

		Container::iterator it;

	};

	iterator begin()
	{
		return iterator(data.begin());
	}

	iterator end()
	{
		return iterator(data.end());
	}

	template<class T, class...Args>
	T* emplace(Args&&... args)
	{
		auto it = data.find(typeid(T));
		if (it == data.end())
		{
			it = data.emplace(std::make_pair(typeid(T), T{std::forward<Args>(args)...}));
		} 
		else 
		{
			data[typeid(T)] = T{ std::forward<Args>(args)... };
		}
		return std::any_cast<T>(&(it->second));
	}

	template<class T, class...Args>
	const T* emplace(Args&&... args)
	{
		auto it = data.find(typeid(T));
		if (it == data.end())
		{
			it = data.emplace(std::make_pair(typeid(T), T{std::forward<Args>(args)...}));
		} 
		else 
		{
			data[typeid(T)] = T{ std::forward<Args>(args)... };
		}
		return std::any_cast<T>(&(it->second));
	}

	template<class T, class...Args>
	T& emplace(Args&&... args)
	{
		auto it = data.find(typeid(T));
		if (it == data.end())
		{
			it = data.emplace(std::make_pair(typeid(T), T{std::forward<Args>(args)...}));
		} 
		else 
		{
			data[typeid(T)] = T{ std::forward<Args>(args)... };
		}
		return std::any_cast<T&>(it->second);
	}

	template<class T, class...Args>
	const T& emplace(Args&&... args)
	{
		auto it = data.find(typeid(T));
		if (it == data.end())
		{
			it = data.emplace(std::make_pair(typeid(T), T{std::forward<Args>(args)...}));
		} 
		else 
		{
			data[typeid(T)] = T{ std::forward<Args>(args)... };
		}
		return std::any_cast<const T&>(it->second);
	}

	void clear()
	{
		data.clear();
	}

	template<class T>
	size_t erase()
	{
		return data.erase(typeid(T));
	}

	template<class T>
	T* get() const
	{
		auto it = data.find(typeid(T));
		if (it == data.end()) return nullptr;
		return std::any_cast<T>(&(it->second));
	}

	template<class T>
	const T* get() const
	{
		auto it = data.find(typeid(T));
		if (it == data.end()) return nullptr;
		return std::any_cast<T>(&(it->second));
	}

	template<class T>
	T& get() const
	{
		auto it = data.find(typeid(T));
		return std::any_cast<T&>(it->second);
	}

	template<class T>
	const T& get() const
	{
		auto it = data.find(typeid(T));
		return std::any_cast<const T&>(it->second);
	}
	
	template<class T>
	bool contains() const
	{
		data.contains(typeid(T));
	}

private:

	Container data;

};

} // namespace Gleam