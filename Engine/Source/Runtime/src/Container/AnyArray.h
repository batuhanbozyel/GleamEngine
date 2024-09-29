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

		std::any& operator*()
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
        
        const std::any& operator*() const
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
        return data[typeid(T)].emplace<T>(T{std::forward<Args>(args)...});
	}
    
    template<class T>
    T& emplace(const T& obj)
    {
        return data[typeid(T)].emplace<T>(obj);
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
    const T& get_unsafe() const
    {
        auto it = data.find(typeid(T));
        return std::any_cast<const T&>(it->second);
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
