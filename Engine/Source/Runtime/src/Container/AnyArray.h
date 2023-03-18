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
	T& emplace(Args&&... args)
	{
        auto& obj = data[typeid(T)] = std::make_any<T>(std::forward<Args>(args)...);
        return std::any_cast<T&>(obj);
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
	T* get()
	{
		auto it = data.find(typeid(T));
		if (it == data.end()) return nullptr;
		return &(std::any_cast<T&>(it->second));
	}

	template<class T>
	T& get_unsafe()
	{
		auto it = data.find(typeid(T));
		return std::any_cast<T&>(it->second);
	}
	
	template<class T>
	bool contains() const
	{
		return data.contains(typeid(T));
	}

	size_t size() const
	{
		return data.size();
	}

private:

	Container data;

};

} // namespace Gleam
