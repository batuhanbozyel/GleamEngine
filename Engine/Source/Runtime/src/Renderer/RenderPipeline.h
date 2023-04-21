//
//  RenderPipeline.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#pragma once
#include "Renderer.h"

namespace Gleam {

class RendererContext;

template <typename T>
concept RendererType = std::is_base_of<IRenderer, T>::value;

class RenderPipeline
{
    friend class RendererContext;
    
    using Container = TArray<Scope<IRenderer>>;
    
public:
    
    template<RendererType T>
    T* AddRenderer()
    {
        GLEAM_ASSERT(!HasRenderer<T>(), "Render pipeline already has the renderer!");
        const auto& renderer = mRenderers.emplace_back(CreateScope<T>());
        renderer->OnCreate(mRendererContext);
        return static_cast<T*>(renderer.get());
    }
    
    template<RendererType T>
    void RemoveRenderer()
    {
        GLEAM_ASSERT(HasRenderer<T>(), "Render pipeline does not have the renderer!");
        auto it = std::remove_if(mRenderers.begin(), mRenderers.end(), [](const Scope<IRenderer>& ptr)
        {
            const auto& renderer = *ptr;
            return typeid(renderer) == typeid(T);
        });
        
        if (it != mRenderers.end()) mRenderers.erase(it);
    }
    
    template<RendererType T>
    T* GetRenderer()
    {
        GLEAM_ASSERT(HasRenderer<T>(), "Render pipeline does not have the renderer!");
        auto it = std::find_if(mRenderers.begin(), mRenderers.end(), [](const Scope<IRenderer>& ptr)
        {
            const auto& renderer = *ptr;
            return typeid(renderer) == typeid(T);
        });
        return static_cast<T*>(it->get());
    }
    
    template<RendererType T>
    bool HasRenderer()
    {
        auto it = std::find_if(mRenderers.begin(), mRenderers.end(), [](const Scope<IRenderer>& ptr)
        {
            const auto& renderer = *ptr;
            return typeid(renderer) == typeid(T);
        });
        return it != mRenderers.end();
    }
    
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
        
        IRenderer* operator*() const
        {
            return it->get();
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
        
        const IRenderer* operator*() const
        {
            return it->get();
        }
        
    private:
        
        typename Container::const_iterator it;
        
    };
    
    iterator begin()
    {
        return iterator(mRenderers.begin());
    }
    
    iterator end()
    {
        return iterator(mRenderers.end());
    }
    
    const_iterator begin() const
    {
        return const_iterator(mRenderers.begin());
    }
    
    const_iterator end() const
    {
        return const_iterator(mRenderers.end());
    }
    
private:
    
    TArray<Scope<IRenderer>> mRenderers;
    
    // initialized and set when a Game instance is created
    static inline RendererContext* mRendererContext = nullptr;
    
};

} // Gleam
