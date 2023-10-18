//
//  PolyArray.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 18.03.2023.
//

#pragma once
#include <typeindex>

namespace Gleam {

template<class Base>
class PolyArray
{
    using Container = HashMap<std::type_index, Scope<Base>>;
    
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
            return it->second.get();
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
            return it->second.get();
        }
        
    private:
        
        typename Container::const_iterator it;
        
    };
    
    ~PolyArray()
    {
        clear();
    }
    
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
    T* emplace(Args&&... args) noexcept
    {
        auto& ptr = data[typeid(T)] = CreateScope<T>(std::forward<Args>(args)...);
        return static_cast<T*>(ptr.get());
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
        return static_cast<T*>(it->second.get());
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

